#-----------------------------------------------------------------------------
# TsukinoEngine の CMake ビルド（Windows / macOS / iOS / Android 共通）
#
# エンジン単体で開くと、エンジンのライブラリと Sandbox を作る。
#   cmake -S . -B build                          （Windows: Visual Studio、macOS: -G Xcode）
#   cmake --build build --config Debug
# iOS は -G Xcode -DCMAKE_SYSTEM_NAME=iOS、Android は NDK の toolchain ファイルを渡す
# （どちらもシェーダーの変換に、ビルドしている OS 用の shaderc が要る。TSUKINO_SHADERC を参照）。
#
# ゲームからは add_subdirectory でこのフォルダを取り込み、Tsukino::Engine をリンクする。
# 実行ファイルの設定（アセットの同梱・シェーダーの変換）は cmake/Tsukino.cmake の関数で行う。
#
# Windows のゲームのビルドは、今は premake（premake5.lua・Tools/premake/tsukino.lua）でも作れる。
#-----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.24)
project(TsukinoEngine LANGUAGES C CXX)

set(TSUKINO_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "TsukinoEngine source folder")
list(APPEND CMAKE_MODULE_PATH "${TSUKINO_ENGINE_ROOT}/cmake")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(TSUKINO_IS_TOP_LEVEL ON)
else()
	set(TSUKINO_IS_TOP_LEVEL OFF)
endif()

option(TSUKINO_BUILD_SANDBOX "Build Tsukino.Sandbox" ${TSUKINO_IS_TOP_LEVEL})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

include(TsukinoShaders)
include(Tsukino)

#-----------------------------------------------------------------------------
# 外部のライブラリ（External/ の submodule）
#-----------------------------------------------------------------------------
include(TsukinoThirdParty)

#-----------------------------------------------------------------------------
# エンジンのモジュール（依存は上から下へ一方向）
#-----------------------------------------------------------------------------
tsukino_add_module(Tsukino.Core
	PCH
	PUBLIC_LINKS cereal_headers hlslpp_headers entt_headers SDL3::SDL3-static
)
target_compile_definitions(Tsukino.Core PRIVATE "TSUKINO_ENGINE_ROOT=\"${TSUKINO_ENGINE_ROOT}\"")
if(WIN32)
	target_link_libraries(Tsukino.Core PRIVATE dwmapi)    # Window の ClickThrough（DwmExtendFrameIntoClientArea。Windows だけ）
endif()

tsukino_add_module(Tsukino.GraphicsCommon
	PCH
	PUBLIC_LINKS Tsukino.Core
)

if(TARGET tsukino_assimp)
	tsukino_add_module(Tsukino.Engine
		PCH
		PUBLIC_LINKS Tsukino.GraphicsCommon
		PRIVATE_LINKS bimg_encode bimg_decode bimg bx tsukino_assimp
	)
else()
	# Assimp が無いビルド（スマホなど）は、モデルの取り込みを外す（開発機で作った .tsm を同梱して読む）
	tsukino_add_module(Tsukino.Engine
		PCH
		EXCLUDE "ModelImporter\\.cpp$"
		PUBLIC_LINKS Tsukino.GraphicsCommon
		PRIVATE_LINKS bimg_encode bimg_decode bimg bx
	)
	target_compile_definitions(Tsukino.Engine PRIVATE TSUKINO_WITHOUT_MODEL_IMPORTER)
endif()

tsukino_add_module(Tsukino.Renderer
	PCH
	PUBLIC_LINKS Tsukino.Engine effekseer_renderer_common
	PRIVATE_LINKS bgfx bimg bx
)
target_include_directories(Tsukino.Renderer PRIVATE
	"${TSUKINO_ENGINE_ROOT}/Tsukino.Renderer/internal"    # Renderer の中だけで使うヘッダ（描画 API の型を含む）
	"${TSUKINO_ENGINE_ROOT}/External/bgfx/3rdparty/stb"   # stb_truetype（DynamicFontAtlas.cpp だけが実装ごと取り込む）
)
if(WIN32)
	target_link_libraries(Tsukino.Renderer PRIVATE dxgi)    # 使う GPU を選ぶ（GraphicsContext）
endif()
# 組み込みシェーダーは、この OS の形式だけを作る
tsukino_add_shaders(Tsukino.Renderer "${TSUKINO_ENGINE_ROOT}/Tsukino.BuiltIn/Assets/Shaders/bgfx")
tsukino_add_shaders(Tsukino.Renderer "${TSUKINO_ENGINE_ROOT}/Tsukino.BuiltIn/Assets/Shaders/bgfx/Effekseer")

tsukino_add_module(Tsukino.Physics
	PUBLIC_LINKS Tsukino.GraphicsCommon jolt
)

tsukino_add_module(Tsukino.Audio
	PUBLIC_LINKS Tsukino.Engine
	PRIVATE_LINKS miniaudio
)

tsukino_add_module(Tsukino.BuiltIn
	PCH
	PUBLIC_LINKS Tsukino.Engine Tsukino.Audio Tsukino.Renderer Tsukino.Physics
)

tsukino_add_module(Tsukino.EngineIntegration
	PCH
	PUBLIC_LINKS Tsukino.BuiltIn
)
if(MSVC)
	# 組み込みコンポーネントのシリアライズを全部取り込むため、既定のセクション数を超える
	target_compile_options(Tsukino.EngineIntegration PRIVATE /bigobj)
endif()

# ゲームがリンクする名前
add_library(Tsukino::Engine ALIAS Tsukino.EngineIntegration)

#-----------------------------------------------------------------------------
# Sandbox（エンジン単体で開いたときだけ）
#-----------------------------------------------------------------------------
if(TSUKINO_BUILD_SANDBOX)
	file(GLOB_RECURSE sandbox_sources CONFIGURE_DEPENDS
		"${TSUKINO_ENGINE_ROOT}/Tsukino.Sandbox/src/*.cpp"
		"${TSUKINO_ENGINE_ROOT}/Tsukino.Sandbox/include/*.hpp"
	)
	tsukino_add_app(Tsukino.Sandbox
		SOURCES ${sandbox_sources}
		ASSET_DIRS "${TSUKINO_ENGINE_ROOT}/Tsukino.Sandbox/Assets"
	)
	target_include_directories(Tsukino.Sandbox PRIVATE "${TSUKINO_ENGINE_ROOT}/Tsukino.Sandbox/include")
	target_precompile_headers(Tsukino.Sandbox PRIVATE "${TSUKINO_ENGINE_ROOT}/Tsukino.Sandbox/pch.h")
	tsukino_add_shaders(Tsukino.Sandbox "${TSUKINO_ENGINE_ROOT}/Tsukino.Sandbox/Assets/Shaders" OPTIONAL)
	if(MSVC)
		set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT Tsukino.Sandbox)
	endif()
endif()
