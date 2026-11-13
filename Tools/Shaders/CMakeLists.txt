#-----------------------------------------------------------------------------
# 組み込みシェーダーを bgfx の形式へ変換する CMake のプロジェクト
#
# Tsukino.BuiltIn/Assets/Shaders/bgfx/ の .sc を、bgfx.cmake の bgfx_compile_shaders で
# 変換する。ファイル名で種類を見分ける（*.vs.sc = 頂点、*.ps.sc = ピクセル）。
# 共通部分（*.sh）と varying.def.sc が変わったときは、全部を変換し直す。
#
# 出力は Tsukino.BuiltIn/Assets/Shaders/bgfx/<形式>/<名前>.sc.bin。
# 形式のフォルダ名は bgfx.cmake の決まり（s_5_0 → dxbc、spirv、metal、essl …）。
#
# Windows の premake からは tsukino_bgfx_shader_commands()（Tools/premake/tsukino.lua）が呼ぶ。
# スマホ・Web・Mac の CMake からも、このファイルをそのまま使う想定。
#
#   TSUKINO_SHADERC  : shaderc.exe のパス（必須）
#   TSUKINO_SHADER_PROFILES : 変換する形式（既定は Windows の s_5_0 だけ）
#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(TsukinoShaders LANGUAGES NONE)

set(TSUKINO_SHADERC "" CACHE FILEPATH "Path to bgfx shaderc")
set(TSUKINO_SHADER_PROFILES "s_5_0" CACHE STRING "shaderc profiles to build (s_5_0, spirv, metal, 300_es, ...)")

if(NOT EXISTS "${TSUKINO_SHADERC}")
	message(FATAL_ERROR "TSUKINO_SHADERC does not point to shaderc: '${TSUKINO_SHADERC}'")
endif()

get_filename_component(TSUKINO_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(SHADER_DIR "${TSUKINO_ENGINE_ROOT}/Tsukino.BuiltIn/Assets/Shaders/bgfx")

# bgfx.cmake の関数は bgfx::shaderc という名前のターゲットを使う。外で作った exe をその名前で見せる
add_executable(bgfx::shaderc IMPORTED)
set_target_properties(bgfx::shaderc PROPERTIES IMPORTED_LOCATION "${TSUKINO_SHADERC}")

# bgfx_shader.sh・shaderlib.sh の場所
set(BGFX_SHADER_INCLUDE_PATH "${TSUKINO_ENGINE_ROOT}/External/bgfx/src")
include("${TSUKINO_ENGINE_ROOT}/External/bgfx.cmake/cmake/bgfxToolUtils.cmake")

file(GLOB VERTEX_SHADERS CONFIGURE_DEPENDS "${SHADER_DIR}/*.vs.sc")
file(GLOB PIXEL_SHADERS CONFIGURE_DEPENDS "${SHADER_DIR}/*.ps.sc")
file(GLOB SHADER_HEADERS CONFIGURE_DEPENDS "${SHADER_DIR}/*.sh")

bgfx_compile_shaders(
	TYPE VERTEX
	SHADERS ${VERTEX_SHADERS}
	VARYING_DEF "${SHADER_DIR}/varying.def.sc"
	OUTPUT_DIR "${SHADER_DIR}"
	OUT_FILES_VAR VERTEX_OUTPUTS
	INCLUDE_DIRS "${SHADER_DIR}"
	DEPENDS ${SHADER_HEADERS}
	PROFILES ${TSUKINO_SHADER_PROFILES}
)

bgfx_compile_shaders(
	TYPE FRAGMENT
	SHADERS ${PIXEL_SHADERS}
	VARYING_DEF "${SHADER_DIR}/varying.def.sc"
	OUTPUT_DIR "${SHADER_DIR}"
	OUT_FILES_VAR PIXEL_OUTPUTS
	INCLUDE_DIRS "${SHADER_DIR}"
	DEPENDS ${SHADER_HEADERS}
	PROFILES ${TSUKINO_SHADER_PROFILES}
)

add_custom_target(tsukino_shaders ALL DEPENDS ${VERTEX_OUTPUTS} ${PIXEL_OUTPUTS})
