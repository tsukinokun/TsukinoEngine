#-----------------------------------------------------------------------------
# 外部のライブラリ（External/ の submodule）を CMake のターゲットにする
#
#   SDL3::SDL3-static          ウィンドウ・入力・ファイル・OS の層
#   bgfx / bimg* / bx / shaderc 描画（shaderc はビルドしている OS 用。クロスビルドでは作らない）
#   jolt                       物理
#   effekseer / effekseer_renderer_common  エフェクト
#   miniaudio                  音
#   cereal_headers / hlslpp_headers / entt_headers  ヘッダだけのライブラリ
#   tsukino_assimp             モデルの取り込み（見つかったときだけ）
#-----------------------------------------------------------------------------
include_guard(GLOBAL)

set(TSUKINO_EXTERNAL "${TSUKINO_ENGINE_ROOT}/External")

#-----------------------------------------------------------------------------
# ヘッダだけのライブラリ
#-----------------------------------------------------------------------------
add_library(cereal_headers INTERFACE)
target_include_directories(cereal_headers SYSTEM INTERFACE "${TSUKINO_EXTERNAL}/cereal/include")

add_library(hlslpp_headers INTERFACE)
target_include_directories(hlslpp_headers SYSTEM INTERFACE "${TSUKINO_EXTERNAL}/hlslpp/include")

add_library(entt_headers INTERFACE)
target_include_directories(entt_headers SYSTEM INTERFACE "${TSUKINO_EXTERNAL}/entt/single_include")

#-----------------------------------------------------------------------------
# SDL3（静的ライブラリ）
#-----------------------------------------------------------------------------
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${TSUKINO_EXTERNAL}/SDL" "${CMAKE_BINARY_DIR}/External/SDL" EXCLUDE_FROM_ALL)

#-----------------------------------------------------------------------------
# bgfx・bimg・bx（bgfx.cmake の手順。ソースは submodule の External/bgfx・bimg・bx）
#
# shaderc はビルドしている OS で動く道具なので、クロスビルド（iOS・Android）では作らない。
# その場合は、Windows か macOS で作った shaderc を TSUKINO_SHADERC で渡す。
# テクスチャの上限は既定の 4096 では足りない（ChronoSort はカードの画像を 4,000 枚以上読む）ので広げる。
#-----------------------------------------------------------------------------
set(BX_DIR "${TSUKINO_EXTERNAL}/bx" CACHE PATH "" FORCE)
set(BIMG_DIR "${TSUKINO_EXTERNAL}/bimg" CACHE PATH "" FORCE)
set(BGFX_DIR "${TSUKINO_EXTERNAL}/bgfx" CACHE PATH "" FORCE)
if(CMAKE_CROSSCOMPILING)
	set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
else()
	set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
	set(BGFX_BUILD_TOOLS_SHADER ON CACHE BOOL "" FORCE)
	set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
	set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
	set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
endif()
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_CONFIG_MAX_TEXTURES 16384 CACHE STRING "" FORCE)
add_subdirectory("${TSUKINO_EXTERNAL}/bgfx.cmake" "${CMAKE_BINARY_DIR}/External/bgfx" EXCLUDE_FROM_ALL)

#-----------------------------------------------------------------------------
# Jolt Physics（premake の JoltPhysics プロジェクトと同じ作り）
#
# JPH_DEBUG_RENDERER は Jolt の型の中身を変えるので、Jolt を include するすべての場所で
# 同じでなければならない。PUBLIC にして、リンクする側へ同じ定義を渡す（Debug だけ）
#-----------------------------------------------------------------------------
file(GLOB_RECURSE jolt_sources CONFIGURE_DEPENDS "${TSUKINO_EXTERNAL}/JoltPhysics/Jolt/*.cpp")
add_library(jolt STATIC ${jolt_sources})
target_include_directories(jolt SYSTEM PUBLIC "${TSUKINO_EXTERNAL}/JoltPhysics")
target_compile_definitions(jolt PUBLIC "$<$<CONFIG:Debug>:JPH_DEBUG_RENDERER>")
target_compile_features(jolt PUBLIC cxx_std_17)
if(MSVC)
	target_compile_options(jolt PRIVATE /FIJolt/Jolt.h /W0)
else()
	target_compile_options(jolt PRIVATE -include Jolt/Jolt.h -w)
endif()

#-----------------------------------------------------------------------------
# Effekseer（ランタイムと、描画の共通部分。描画の bgfx 版は Tsukino.Renderer にある）
#-----------------------------------------------------------------------------
set(effekseer_root "${TSUKINO_EXTERNAL}/Effekseer/Dev/Cpp")
file(GLOB_RECURSE effekseer_sources CONFIGURE_DEPENDS "${effekseer_root}/Effekseer/Effekseer/*.cpp")
add_library(effekseer STATIC ${effekseer_sources})
target_include_directories(effekseer SYSTEM PUBLIC
	"${effekseer_root}"
	"${effekseer_root}/Effekseer"
	"${effekseer_root}/3rdParty"
)
if(MSVC)
	target_compile_options(effekseer PRIVATE /W0)
	target_compile_definitions(effekseer PRIVATE WIN32 _WIN32_WINNT=0x0A00)
else()
	target_compile_options(effekseer PRIVATE -w)
endif()

file(GLOB_RECURSE effekseer_common_sources CONFIGURE_DEPENDS "${effekseer_root}/EffekseerRendererCommon/EffekseerRendererCommon/*.cpp")
add_library(effekseer_renderer_common STATIC ${effekseer_common_sources})
target_include_directories(effekseer_renderer_common SYSTEM PUBLIC
	"${effekseer_root}/EffekseerRendererCommon"
	"${effekseer_root}/EffekseerMaterialCompiler"
)
target_link_libraries(effekseer_renderer_common PUBLIC effekseer)
if(MSVC)
	target_compile_options(effekseer_renderer_common PRIVATE /W0)
	target_compile_definitions(effekseer_renderer_common PRIVATE WIN32 _WIN32_WINNT=0x0A00)
else()
	target_compile_options(effekseer_renderer_common PRIVATE -w)
endif()

#-----------------------------------------------------------------------------
# miniaudio（C の1ファイル。使わない機能を外す）
#-----------------------------------------------------------------------------
add_library(miniaudio STATIC "${TSUKINO_EXTERNAL}/miniaudio/miniaudio.c")
target_include_directories(miniaudio SYSTEM PUBLIC "${TSUKINO_EXTERNAL}/miniaudio")
target_compile_definitions(miniaudio PUBLIC MA_NO_ENCODING MA_NO_GENERATION)
if(MSVC)
	target_compile_options(miniaudio PRIVATE /W0)
else()
	target_compile_options(miniaudio PRIVATE -w)
endif()
if(APPLE)
	# Core Audio のフレームワークを実行時に読まず、直接リンクする（iOS では必須）
	target_compile_definitions(miniaudio PRIVATE MA_NO_RUNTIME_LINKING)
	target_link_libraries(miniaudio PRIVATE "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox")
	if(NOT IOS)
		target_link_libraries(miniaudio PRIVATE "-framework AudioUnit")
	else()
		target_link_libraries(miniaudio PRIVATE "-framework AVFoundation")
	endif()
elseif(ANDROID)
	target_link_libraries(miniaudio PRIVATE OpenSLES log)
elseif(UNIX)
	target_link_libraries(miniaudio PRIVATE dl pthread m)
endif()

#-----------------------------------------------------------------------------
# Assimp（モデルの取り込み。開発機だけで使う道具）
#
#   1. CMake のパッケージ（vcpkg・Homebrew の assimp など）があればそれを使う
#   2. Windows では、premake が NuGet から復元した AssimpCpp（.build/packages）を使う
#   3. どちらも無ければ取り込みを外す（スマホのビルドはこれ。.fbx / .obj は開発機で .tsm にして同梱する）
#-----------------------------------------------------------------------------
option(TSUKINO_WITH_ASSIMP "Build the model importer when Assimp is available" ON)
if(TSUKINO_WITH_ASSIMP AND NOT IOS AND NOT ANDROID)
	find_package(assimp CONFIG QUIET)
	if(TARGET assimp::assimp)
		add_library(tsukino_assimp INTERFACE)
		target_link_libraries(tsukino_assimp INTERFACE assimp::assimp)
	elseif(WIN32 AND EXISTS "${TSUKINO_ENGINE_ROOT}/.build/packages/AssimpCpp.5.0.1.6/build/native/include")
		set(assimp_nuget "${TSUKINO_ENGINE_ROOT}/.build/packages/AssimpCpp.5.0.1.6/build/native")
		add_library(tsukino_assimp INTERFACE)
		target_include_directories(tsukino_assimp SYSTEM INTERFACE "${assimp_nuget}/include")
		target_link_libraries(tsukino_assimp INTERFACE
			"$<IF:$<CONFIG:Debug>,${assimp_nuget}/lib/Debug/assimp-vc142-mtd.lib,${assimp_nuget}/lib/Release/assimp-vc142-mt.lib>"
		)
		# 実行ファイルの隣へ DLL を写すために覚えておく（tsukino_add_app が使う）
		set(TSUKINO_ASSIMP_RUNTIME_DLL
			"$<IF:$<CONFIG:Debug>,${assimp_nuget}/bin/Debug/assimp-vc142-mtd.dll,${assimp_nuget}/bin/Release/assimp-vc142-mt.dll>"
			CACHE INTERNAL "")
	endif()
endif()
if(TARGET tsukino_assimp)
	message(STATUS "TsukinoEngine: model importer enabled (Assimp found)")
else()
	message(STATUS "TsukinoEngine: model importer disabled (no Assimp); ship .tsm caches instead")
endif()
