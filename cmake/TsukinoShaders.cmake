#-----------------------------------------------------------------------------
# bgfx のシェーダー（.sc）を変換するための CMake の関数
#
# エンジンの CMake ビルド（tsukino_add_shaders）と、premake から呼ぶ変換用のプロジェクト
# （Tools/Shaders/CMakeLists.txt）の両方が使う。
#
# 形式ごとに、shaderc へ渡す対象の OS を決めて呼ぶ（ビルドしている OS には依らない）。
# そのため、Windows で Metal 向けを作ることも、Mac で Direct3D 向けを作ることもできる。
# （bgfx.cmake の bgfx_compile_shaders は、ビルドしている OS を対象にしてしまうので使わない）
#-----------------------------------------------------------------------------
include_guard(GLOBAL)

set(TSUKINO_SHADER_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." CACHE INTERNAL "")
get_filename_component(TSUKINO_SHADER_ENGINE_ROOT "${TSUKINO_SHADER_ENGINE_ROOT}" ABSOLUTE)

#-----------------------------------------------------------------------------
# 形式から、shaderc の対象の OS と出力のフォルダ名を決める
#   PROFILE      : s_5_0（Direct3D 11）・metal（macOS / iOS）・spirv（Vulkan）・300_es（OpenGL ES）
#   OUT_PLATFORM : shaderc の --platform
#   OUT_FOLDER   : 出力のフォルダ名（ShaderLoader::GetBinaryDirectoryName と揃える）
#-----------------------------------------------------------------------------
function(tsukino_shader_profile_info PROFILE OUT_PLATFORM OUT_FOLDER)
	if(PROFILE STREQUAL "s_5_0")
		set(platform "windows")
		set(folder "dxbc")
	elseif(PROFILE STREQUAL "metal")
		set(platform "osx")
		set(folder "metal")
	elseif(PROFILE STREQUAL "spirv")
		set(platform "linux")
		set(folder "spirv")
	elseif(PROFILE STREQUAL "300_es")
		set(platform "android")
		set(folder "essl")
	else()
		message(FATAL_ERROR "Unsupported shader profile: ${PROFILE}")
	endif()
	set(${OUT_PLATFORM} "${platform}" PARENT_SCOPE)
	set(${OUT_FOLDER} "${folder}" PARENT_SCOPE)
endfunction()

#-----------------------------------------------------------------------------
# 1つのフォルダの .sc を変換するコマンドを作る
#   OUT_VAR  : 出力のファイルの一覧を返す変数
#   DIR      : .sc のフォルダ（*.vs.sc = 頂点、*.ps.sc = ピクセル）。出力は <DIR>/<形式>/<名前>.sc.bin
#   SHADERC  : shaderc のパス（ジェネレーター式でもよい）
#   PROFILES : 作る形式の一覧
#   DEPENDS  : 変換の前に要るもの（shaderc をビルドするターゲットなど。省略可）
#
# 入出力の宣言（varying.def.sc）はフォルダにあればそれを、無ければ組み込みのものを使う。
# #include はフォルダ → 組み込みの Shaders/bgfx の順に探す。
# 共通部分（*.sh）と varying.def.sc が変わったときは、全部を変換し直す。
#-----------------------------------------------------------------------------
function(tsukino_compile_shader_folder OUT_VAR DIR SHADERC)
	cmake_parse_arguments(ARG "" "" "PROFILES;DEPENDS" ${ARGN})

	set(builtin_dir "${TSUKINO_SHADER_ENGINE_ROOT}/Tsukino.BuiltIn/Assets/Shaders/bgfx")
	set(bgfx_include_dir "${TSUKINO_SHADER_ENGINE_ROOT}/External/bgfx/src")    # bgfx_shader.sh・shaderlib.sh の場所

	file(TO_CMAKE_PATH "${DIR}" shader_dir)
	get_filename_component(shader_dir "${shader_dir}" ABSOLUTE)
	if(NOT IS_DIRECTORY "${shader_dir}")
		message(FATAL_ERROR "Shader folder does not exist: '${DIR}'")
	endif()

	if(EXISTS "${shader_dir}/varying.def.sc")
		set(varying_def "${shader_dir}/varying.def.sc")
	else()
		set(varying_def "${builtin_dir}/varying.def.sc")
	endif()

	file(GLOB vertex_shaders CONFIGURE_DEPENDS "${shader_dir}/*.vs.sc")
	file(GLOB pixel_shaders CONFIGURE_DEPENDS "${shader_dir}/*.ps.sc")
	file(GLOB shader_headers CONFIGURE_DEPENDS "${shader_dir}/*.sh" "${builtin_dir}/*.sh")
	list(REMOVE_DUPLICATES shader_headers)

	set(outputs "")
	foreach(shader ${vertex_shaders} ${pixel_shaders})
		get_filename_component(shader_name "${shader}" NAME)
		if(shader_name MATCHES "\\.vs\\.sc$")
			set(type "vertex")
		else()
			set(type "fragment")
		endif()

		foreach(profile ${ARG_PROFILES})
			tsukino_shader_profile_info(${profile} platform folder)
			set(output "${shader_dir}/${folder}/${shader_name}.bin")
			add_custom_command(
				OUTPUT "${output}"
				COMMAND "${CMAKE_COMMAND}" -E make_directory "${shader_dir}/${folder}"
				COMMAND "${SHADERC}"
					-f "${shader}" -o "${output}"
					--type ${type} --platform ${platform} --profile ${profile}
					-O 3 --Werror
					-i "${bgfx_include_dir}" -i "${shader_dir}" -i "${builtin_dir}"
					--varyingdef "${varying_def}"
				MAIN_DEPENDENCY "${shader}"
				DEPENDS ${shader_headers} "${varying_def}" ${ARG_DEPENDS}
				COMMENT "shaderc ${profile} ${shader_name}"
				VERBATIM
			)
			list(APPEND outputs "${output}")
		endforeach()
	endforeach()

	set(${OUT_VAR} ${outputs} PARENT_SCOPE)
endfunction()
