#-----------------------------------------------------------------------------
# シェーダーを bgfx の形式へ変換する CMake のプロジェクト
#
# 1つのフォルダ（TSUKINO_SHADER_DIR）の .sc を、bgfx.cmake の bgfx_compile_shaders で
# 変換する。ファイル名で種類を見分ける（*.vs.sc = 頂点、*.ps.sc = ピクセル）。
# 共通部分（*.sh）と varying.def.sc が変わったときは、全部を変換し直す。
# エンジンの組み込みシェーダーも、ゲームのシェーダーも、フォルダごとにこれを使う。
#
# 出力は <TSUKINO_SHADER_DIR>/<形式>/<名前>.sc.bin。
# 形式のフォルダ名は bgfx.cmake の決まり（s_5_0 → dxbc、spirv、metal、essl …）で、
# 実行時に読む側（ShaderLoader::ToBinaryPath）と揃える。
#
# 入出力の宣言（varying.def.sc）はフォルダにあればそれを、無ければ組み込みのものを使う。
# #include はフォルダ → 組み込みの Shaders/bgfx の順に探す。
#
# Windows の premake からは tsukino_shaders()（Tools/premake/tsukino.lua）が呼ぶ。
# スマホ・Mac の CMake からも、このファイルをそのまま使う想定。
#
#   TSUKINO_SHADERC         : shaderc.exe のパス（必須）
#   TSUKINO_SHADER_DIR      : 変換する .sc のフォルダ（既定は組み込みの Tsukino.BuiltIn/Assets/Shaders/bgfx）
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
set(BUILTIN_SHADER_DIR "${TSUKINO_ENGINE_ROOT}/Tsukino.BuiltIn/Assets/Shaders/bgfx")

set(TSUKINO_SHADER_DIR "${BUILTIN_SHADER_DIR}" CACHE PATH "Folder of the .sc files to compile")
# premake からは \ 区切りで渡るので、CMake の形（/ 区切りの絶対パス）に直す
file(TO_CMAKE_PATH "${TSUKINO_SHADER_DIR}" SHADER_DIR)
get_filename_component(SHADER_DIR "${SHADER_DIR}" ABSOLUTE)
if(NOT IS_DIRECTORY "${SHADER_DIR}")
	message(FATAL_ERROR "TSUKINO_SHADER_DIR is not a folder: '${TSUKINO_SHADER_DIR}'")
endif()

if(EXISTS "${SHADER_DIR}/varying.def.sc")
	set(VARYING_DEF "${VARYING_DEF}")
else()
	set(VARYING_DEF "${BUILTIN_SHADER_DIR}/varying.def.sc")
endif()

# bgfx.cmake の関数は bgfx::shaderc という名前のターゲットを使う。外で作った exe をその名前で見せる
add_executable(bgfx::shaderc IMPORTED)
set_target_properties(bgfx::shaderc PROPERTIES IMPORTED_LOCATION "${TSUKINO_SHADERC}")

# bgfx_shader.sh・shaderlib.sh の場所
set(BGFX_SHADER_INCLUDE_PATH "${TSUKINO_ENGINE_ROOT}/External/bgfx/src")
include("${TSUKINO_ENGINE_ROOT}/External/bgfx.cmake/cmake/bgfxToolUtils.cmake")

file(GLOB VERTEX_SHADERS CONFIGURE_DEPENDS "${SHADER_DIR}/*.vs.sc")
file(GLOB PIXEL_SHADERS CONFIGURE_DEPENDS "${SHADER_DIR}/*.ps.sc")
file(GLOB SHADER_HEADERS CONFIGURE_DEPENDS "${SHADER_DIR}/*.sh" "${BUILTIN_SHADER_DIR}/*.sh")
list(REMOVE_DUPLICATES SHADER_HEADERS)

bgfx_compile_shaders(
	TYPE VERTEX
	SHADERS ${VERTEX_SHADERS}
	VARYING_DEF "${VARYING_DEF}"
	OUTPUT_DIR "${SHADER_DIR}"
	OUT_FILES_VAR VERTEX_OUTPUTS
	INCLUDE_DIRS "${SHADER_DIR}" "${BUILTIN_SHADER_DIR}"
	DEPENDS ${SHADER_HEADERS} "${VARYING_DEF}"
	PROFILES ${TSUKINO_SHADER_PROFILES}
)

bgfx_compile_shaders(
	TYPE FRAGMENT
	SHADERS ${PIXEL_SHADERS}
	VARYING_DEF "${VARYING_DEF}"
	OUTPUT_DIR "${SHADER_DIR}"
	OUT_FILES_VAR PIXEL_OUTPUTS
	INCLUDE_DIRS "${SHADER_DIR}" "${BUILTIN_SHADER_DIR}"
	DEPENDS ${SHADER_HEADERS} "${VARYING_DEF}"
	PROFILES ${TSUKINO_SHADER_PROFILES}
)

add_custom_target(tsukino_shaders ALL DEPENDS ${VERTEX_OUTPUTS} ${PIXEL_OUTPUTS})
