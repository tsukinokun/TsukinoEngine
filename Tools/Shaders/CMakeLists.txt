#-----------------------------------------------------------------------------
# シェーダーを bgfx の形式へ変換する CMake のプロジェクト（premake のビルドから使う）
#
# 1つのフォルダ（TSUKINO_SHADER_DIR）の .sc を変換する。中身は cmake/TsukinoShaders.cmake の
# tsukino_compile_shader_folder（エンジンの CMake ビルドと同じ関数）。
#
# Windows の premake からは tsukino_shaders()（Tools/premake/tsukino.lua）が呼ぶ。
# CMake のビルドでは使わない（tsukino_add_shaders() がその場で同じ変換を足す）。
#
#   TSUKINO_SHADERC         : shaderc のパス（必須。ビルドしている OS で動くもの）
#   TSUKINO_SHADER_DIR      : 変換する .sc のフォルダ（既定は組み込みの Tsukino.BuiltIn/Assets/Shaders/bgfx）
#   TSUKINO_SHADER_PROFILES : 変換する形式（既定は Windows の s_5_0 だけ）
#                             s_5_0（Direct3D 11）・metal（macOS / iOS）・spirv（Vulkan）・300_es（OpenGL ES）
#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(TsukinoShaders LANGUAGES NONE)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/TsukinoShaders.cmake")

set(TSUKINO_SHADERC "" CACHE FILEPATH "Path to bgfx shaderc")
set(TSUKINO_SHADER_PROFILES "s_5_0" CACHE STRING "shaderc profiles to build (s_5_0, metal, spirv, 300_es)")
set(TSUKINO_SHADER_DIR "${TSUKINO_SHADER_ENGINE_ROOT}/Tsukino.BuiltIn/Assets/Shaders/bgfx" CACHE PATH "Folder of the .sc files to compile")

if(NOT EXISTS "${TSUKINO_SHADERC}")
	message(FATAL_ERROR "TSUKINO_SHADERC does not point to shaderc: '${TSUKINO_SHADERC}'")
endif()

tsukino_compile_shader_folder(OUTPUTS "${TSUKINO_SHADER_DIR}" "${TSUKINO_SHADERC}" PROFILES ${TSUKINO_SHADER_PROFILES})
add_custom_target(tsukino_shaders ALL DEPENDS ${OUTPUTS})
