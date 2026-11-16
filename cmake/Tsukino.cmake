#-----------------------------------------------------------------------------
# TsukinoEngine の CMake の関数（エンジン自身と、エンジンを使うゲームの両方が使う）
#
#   tsukino_target_defaults(<target>)                 コンパイルの共通設定
#   tsukino_add_module(<name> ...)                     エンジンのモジュール（静的ライブラリ）を作る
#   tsukino_add_shaders(<target> <dir> [OPTIONAL])     フォルダの .sc を、この OS の形式へビルド時に変換する
#   tsukino_add_app(<name> SOURCES ... ASSET_DIRS ...) 実行ファイル（スマホでは共有ライブラリ）を作る
#-----------------------------------------------------------------------------
include_guard(GLOBAL)

#-----------------------------------------------------------------------------
# この OS で読むシェーダーの形式（ShaderLoader::GetBinaryDirectoryName と揃える）
#-----------------------------------------------------------------------------
if(WIN32)
	set(tsukino_default_profiles "s_5_0")
elseif(APPLE)
	set(tsukino_default_profiles "metal")
else()
	set(tsukino_default_profiles "spirv")    # Android（Vulkan）
endif()
set(TSUKINO_SHADER_PROFILES "${tsukino_default_profiles}" CACHE STRING "Shader formats to build (s_5_0, metal, spirv, 300_es)")
set(TSUKINO_SHADERC "" CACHE FILEPATH "shaderc built for this host (required when cross-compiling for iOS / Android)")

#-----------------------------------------------------------------------------
# コンパイルの共通設定
#-----------------------------------------------------------------------------
function(tsukino_target_defaults TARGET)
	target_compile_features(${TARGET} PUBLIC cxx_std_20)
	if(MSVC)
		target_compile_options(${TARGET} PRIVATE /utf-8 /permissive- /MP)
	else()
		# エンジンは _DEBUG で Debug の動き（アセットの場所など）を切り替える。MSVC 以外は自分で付ける
		target_compile_definitions(${TARGET} PRIVATE "$<$<CONFIG:Debug>:_DEBUG>")
	endif()
endfunction()

#-----------------------------------------------------------------------------
# エンジンのモジュール（静的ライブラリ）を作る
#   tsukino_add_module(<name> [PCH] [EXCLUDE <regex>...] [PUBLIC_LINKS ...] [PRIVATE_LINKS ...])
# <name>/src・include・internal の中を全部ビルドし、include を公開する。
# PCH は <name>/pch.h を各ファイルの先頭に取り込む（premake の forceincludes と同じ）
#-----------------------------------------------------------------------------
function(tsukino_add_module NAME)
	cmake_parse_arguments(ARG "PCH" "" "EXCLUDE;PUBLIC_LINKS;PRIVATE_LINKS" ${ARGN})

	set(dir "${TSUKINO_ENGINE_ROOT}/${NAME}")
	file(GLOB_RECURSE sources CONFIGURE_DEPENDS
		"${dir}/src/*.cpp" "${dir}/src/*.hpp" "${dir}/include/*.hpp" "${dir}/internal/*.hpp")
	foreach(pattern ${ARG_EXCLUDE})
		list(FILTER sources EXCLUDE REGEX "${pattern}")
	endforeach()

	add_library(${NAME} STATIC ${sources})
	target_include_directories(${NAME} PUBLIC "${dir}/include")
	target_link_libraries(${NAME} PUBLIC ${ARG_PUBLIC_LINKS} PRIVATE ${ARG_PRIVATE_LINKS})
	if(ARG_PCH)
		target_precompile_headers(${NAME} PRIVATE "${dir}/pch.h")
	endif()
	tsukino_target_defaults(${NAME})
endfunction()

#-----------------------------------------------------------------------------
# フォルダの .sc を、この OS の形式へビルド時に変換する（<dir>/<形式>/<名前>.sc.bin）
#   tsukino_add_shaders(<target> <dir> [OPTIONAL])
# OPTIONAL を付けると、フォルダが無いときは何もしない。
# 実行時は AssetManager::Load("<dir>/Water.ps.sc") のように .sc のパスで読む
#-----------------------------------------------------------------------------
function(tsukino_add_shaders TARGET DIR)
	cmake_parse_arguments(ARG "OPTIONAL" "" "" ${ARGN})
	if(NOT IS_DIRECTORY "${DIR}")
		if(ARG_OPTIONAL)
			return()
		endif()
		message(FATAL_ERROR "tsukino_add_shaders: folder not found: ${DIR}")
	endif()

	if(TSUKINO_SHADERC)
		set(shaderc "${TSUKINO_SHADERC}")
		set(depends "")
	elseif(TARGET shaderc)
		set(shaderc "$<TARGET_FILE:shaderc>")
		set(depends shaderc)
	else()
		message(FATAL_ERROR "tsukino_add_shaders: no shaderc. Set TSUKINO_SHADERC to a shaderc built for this host.")
	endif()

	tsukino_compile_shader_folder(outputs "${DIR}" "${shaderc}" PROFILES ${TSUKINO_SHADER_PROFILES} DEPENDS ${depends})

	string(MAKE_C_IDENTIFIER "${DIR}" id)
	string(MD5 id "${id}")
	string(SUBSTRING "${id}" 0 8 id)
	set(shader_target "${TARGET}_shaders_${id}")
	add_custom_target(${shader_target} DEPENDS ${outputs})
	set_target_properties(${shader_target} PROPERTIES FOLDER "Shaders")
	add_dependencies(${TARGET} ${shader_target})
endfunction()

#-----------------------------------------------------------------------------
# 実行ファイルを作る（Android では SDL が読み込む共有ライブラリ libmain.so）
#   tsukino_add_app(<name> SOURCES ... [ASSET_DIRS <dir>...] [WORKING_DIR <dir>] [BUNDLE_ID <id>])
#
# アセットは WORKING_DIR（既定はこの CMakeLists.txt のフォルダ）からの相対パスで読む。
#   Debug   : 作業フォルダを WORKING_DIR にして、ソースのアセットをそのまま読む（エンジンの組み込みアセットも
#             ソースの場所から読む）
#   Release : ASSET_DIRS と組み込みアセットを、実行ファイルの隣（macOS / iOS は .app の中）へ同じ相対パスで写す
# Android のアセットは、Gradle のプロジェクトが APK へ入れる（このフォルダの相対パスのまま assets/ へ）
#-----------------------------------------------------------------------------
function(tsukino_add_app NAME)
	cmake_parse_arguments(ARG "" "WORKING_DIR;BUNDLE_ID" "SOURCES;ASSET_DIRS" ${ARGN})
	if(NOT ARG_WORKING_DIR)
		set(ARG_WORKING_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	endif()
	if(NOT ARG_BUNDLE_ID)
		string(MAKE_C_IDENTIFIER "${NAME}" bundle_name)
		set(ARG_BUNDLE_ID "jp.tsukino.${bundle_name}")
	endif()

	if(ANDROID)
		add_library(${NAME} SHARED ${ARG_SOURCES})
		set_target_properties(${NAME} PROPERTIES OUTPUT_NAME main)    # SDL の Java 側が libmain.so を読む
	elseif(APPLE)
		add_executable(${NAME} MACOSX_BUNDLE ${ARG_SOURCES})
		set_target_properties(${NAME} PROPERTIES
			MACOSX_BUNDLE_GUI_IDENTIFIER "${ARG_BUNDLE_ID}"
			MACOSX_BUNDLE_BUNDLE_NAME "${NAME}"
			XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${ARG_BUNDLE_ID}")
		if(IOS)
			# 横持ち・全画面（iPhone と iPad）
			set_target_properties(${NAME} PROPERTIES
				XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
				XCODE_ATTRIBUTE_INFOPLIST_KEY_UIRequiresFullScreen "YES"
				XCODE_ATTRIBUTE_INFOPLIST_KEY_UISupportedInterfaceOrientations "UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight")
		endif()
	elseif(WIN32)
		add_executable(${NAME} WIN32 ${ARG_SOURCES})    # SDL_main.h が WinMain を用意する
	else()
		add_executable(${NAME} ${ARG_SOURCES})
	endif()

	target_link_libraries(${NAME} PRIVATE Tsukino::Engine)
	tsukino_target_defaults(${NAME})

	if(MSVC)
		set_target_properties(${NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${ARG_WORKING_DIR}")
	endif()
	if(CMAKE_GENERATOR STREQUAL "Xcode")
		set_target_properties(${NAME} PROPERTIES
			XCODE_SCHEME_WORKING_DIRECTORY "${ARG_WORKING_DIR}"
			XCODE_GENERATE_SCHEME ON)
	endif()

	#-------------------------------------------------------------------------
	# Release で同梱するアセット（実行ファイルの隣、macOS は Contents/Resources、iOS は .app の直下）
	#-------------------------------------------------------------------------
	if(NOT ANDROID)
		if(APPLE AND NOT IOS)
			set(payload_dir "$<TARGET_BUNDLE_CONTENT_DIR:${NAME}>/Resources")
		elseif(IOS)
			set(payload_dir "$<TARGET_BUNDLE_DIR:${NAME}>")
		else()
			set(payload_dir "$<TARGET_FILE_DIR:${NAME}>")
		endif()

		set(copy_dirs "${TSUKINO_ENGINE_ROOT}/Tsukino.BuiltIn/Assets" ${ARG_ASSET_DIRS})
		foreach(dir ${copy_dirs})
			if(dir STREQUAL "${TSUKINO_ENGINE_ROOT}/Tsukino.BuiltIn/Assets")
				set(relative "Tsukino.BuiltIn/Assets")    # エンジンのアセットは、エンジンのルートからの相対
			else()
				file(RELATIVE_PATH relative "${ARG_WORKING_DIR}" "${dir}")
			endif()
			add_custom_command(TARGET ${NAME} POST_BUILD
				COMMAND "${CMAKE_COMMAND}" -E "$<IF:$<CONFIG:Debug>,true,copy_directory>" "${dir}" "${payload_dir}/${relative}"
				VERBATIM)
		endforeach()
	endif()

	# Windows の NuGet 版 Assimp は DLL なので、実行ファイルの隣へ写す
	if(WIN32 AND TSUKINO_ASSIMP_RUNTIME_DLL)
		add_custom_command(TARGET ${NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${TSUKINO_ASSIMP_RUNTIME_DLL}" "$<TARGET_FILE_DIR:${NAME}>"
			VERBATIM)
	endif()
endfunction()
