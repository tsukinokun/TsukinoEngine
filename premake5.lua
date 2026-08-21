-- このファイルが実際に置かれているディレクトリ（Engineルート）
local ROOT_ENGINE_ROOT     = path.getdirectory(_SCRIPT)
-- TsukinoEngine自身がルートスクリプト（単体ビルド）かどうか
local ROOT_IS_SAME_AS_ROOT = (path.getabsolute(ROOT_ENGINE_ROOT) == path.getabsolute(_MAIN_SCRIPT_DIR))

-- 単体ビルド時のみworkspaceを宣言する。
-- 外部の親premake5.lua（サブモジュールとして取り込んだ側）からinclude()された場合は、
-- 親側が既にworkspaceを宣言している前提のためここではスキップする。
if ROOT_IS_SAME_AS_ROOT then
workspace "TsukinoEngine"                   -- ソリューション名
    architecture "x64"                      -- アーキテクチャ
    configurations { "Debug", "Release" }   -- ビルド構成

    startproject "Tsukino.Sandbox"          -- スタートアッププロジェクト
    location ".build"                       -- ビルドファイルの出力先
    multiprocessorcompile "On"              -- マルチプロセッサコンパイルを有効化
    exceptionhandling "On"                  -- 例外処理を有効化

    filter "configurations:*"
        defines { "JPH_DEBUG_RENDERER" } -- 値は1でなくても定義されていることが重要
    filter {}

    filter "configurations:Debug"
        optimize "Off"
        symbols "On"

    filter "action:vs*"
        buildoptions { "/utf-8" }
    filter {}

    filter "configurations:Release"
        optimize "Full"
        symbols "On"
        -- NDEBUG は premake が自動では付けないため明示的に定義する。
        -- これが無いと assert() が Release でも生き続け、EnTT の ENTT_ASSERT が
        -- 全コンポーネントアクセスに乗ったまま製品ビルドが作られてしまう。
        defines { "NDEBUG" }

    filter {}

    filter "configurations:*"
        linkoptions { "/IGNORE:4006" }
    filter {}
end

----------------------------------------
-- DirectXTexプロジェクトを追加
----------------------------------------
project "DirectXTex"
    location ".build/DirectXTex"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    -- PCH も一旦やめて、素の状態にする
    flags { "NoPCH" }

    files {
        "External/DirectXTex/DirectXTex/*.h",
        "External/DirectXTex/DirectXTex/*.inl",
        "External/DirectXTex/DirectXTex/*.cpp",
        "External/DirectXTex/Common/*.h",
    }

    -- GPU / Compute Shader 関連は全部切る
    removefiles {
        "External/DirectXTex/DirectXTex/BCDirectCompute.cpp",
        "External/DirectXTex/DirectXTex/DirectXTexCompressGPU.cpp",
        "External/DirectXTex/DirectXTex/DirectXTexD3D12.cpp"
    }

    includedirs {
        "External/DirectXTex/DirectXTex",
        "External/DirectXTex/Common",
    }

    filter "system:windows"
        defines { "WIN32", "_WIN32_WINNT=0x0A00" }
    filter {}

----------------------------------------
-- joltプロジェクトを追加
----------------------------------------
project "JoltPhysics"
    location ".build/JoltPhysics"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    -- PCH も一旦やめて、素の状態にする
    flags { "NoPCH" }

	local SOURCE_PATH = "External/JoltPhysics/Jolt"

   	warnings "Default"
	-- inlining "Auto"			-- 常にインライン展開
	-- optimize "Full"			-- 常に最適化

	buildoptions {
		"/Zo",	-- 最適化されたデバッグ機能の強化
	}
	
	-- 追加するソースコード
	-- *=フォルダ内 **=フォルダ内とその階層下サブフォルダ内
    files {
		path.join(SOURCE_PATH, "**.c"),
		path.join(SOURCE_PATH, "**.cpp"),
		path.join(SOURCE_PATH, "**.h"),
		path.join(SOURCE_PATH, "**.natvis"),
	}
	
	-- "" インクルードパス
	includedirs {
		SOURCE_PATH,
		"External",
		"External/JoltPhysics",
	}

	-- プリコンパイル済ヘッダー
	-- pchheader "Jolt.h"
	-- pchsource (path.join(SOURCE_PATH, "pch.cpp"))
	forceincludes "Jolt.h"

	-- 除去するファイル
	removefiles {
	}

	-- プリプロセッサ #define
   	defines {
   		"JPH_DEBUG_RENDERER=1",
	}

	-- フォルダ分け
	vpaths {
		["ヘッダー ファイル/*"] = {
			path.join(SOURCE_PATH, "**.h"),
			path.join(SOURCE_PATH, "**.hxx"),
			path.join(SOURCE_PATH, "**.hpp"),
			path.join(SOURCE_PATH, "**.inl")
		},
		["ソース ファイル/*"] = {
			path.join(SOURCE_PATH, "**.c"),
			path.join(SOURCE_PATH, "**.cxx"),
			path.join(SOURCE_PATH, "**.cpp")
		},
		["*"] = {
			path.join(SOURCE_PATH, "**.natvis")
		},
	}

----------------------------------------
-- Effekseer コアランタイム
---------------------------------------
project "Effekseer"
    location ".build/Effekseer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags { "NoPCH" }

    files {
        "External/Effekseer/Dev/Cpp/Effekseer/Effekseer/**.h",
        "External/Effekseer/Dev/Cpp/Effekseer/Effekseer/**.cpp",
    }

    includedirs {
        "External/Effekseer/Dev/Cpp/Effekseer",
        "External/Effekseer/Dev/Cpp/3rdParty",
        "External/Effekseer/Dev/Cpp",
    }

    defines { "EFK_ENABLE_SSE2" }

    filter "system:windows"
        defines { "WIN32", "_WIN32_WINNT=0x0A00" }
    filter {}

----------------------------------------
-- Effekseer レンダラー共通
--------------------------------------
project "EffekseerRendererCommon"
    location ".build/EffekseerRendererCommon"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags { "NoPCH" }

    files {
        "External/Effekseer/Dev/Cpp/EffekseerRendererCommon/EffekseerRendererCommon/**.h",
        "External/Effekseer/Dev/Cpp/EffekseerRendererCommon/EffekseerRendererCommon/**.cpp",
    }

    includedirs {
        "External/Effekseer/Dev/Cpp",
        "External/Effekseer/Dev/Cpp/Effekseer",
        "External/Effekseer/Dev/Cpp/3rdParty",
        "External/Effekseer/Dev/Cpp/EffekseerRendererCommon",
        "External/Effekseer/Dev/Cpp/EffekseerMaterialCompiler",
    }

    filter "system:windows"
        defines { "WIN32", "_WIN32_WINNT=0x0A00" }
    filter {}

    links {
        "Effekseer",
        "d3d11",
        "dxgi",
        "d3dcompiler",
    }

----------------------------------------
-- Effekseer DX11 レンダラー
---------------------------------------
project "EffekseerRendererDX11"
    location ".build/EffekseerRendererDX11"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    flags { "NoPCH" }

    files {
        "External/Effekseer/Dev/Cpp/EffekseerRendererDX11/EffekseerRendererDX11/**.h",
        "External/Effekseer/Dev/Cpp/EffekseerRendererDX11/EffekseerRendererDX11/**.cpp",
        "External/Effekseer/Dev/Cpp/EffekseerMaterialCompiler/**.h",
        "External/Effekseer/Dev/Cpp/EffekseerMaterialCompiler/**.cpp",
    }

    includedirs {
        "External/Effekseer/Dev/Cpp",
        "External/Effekseer/Dev/Cpp/Effekseer",
        "External/Effekseer/Dev/Cpp/EffekseerRendererDX11",
        "External/Effekseer/Dev/Cpp/EffekseerMaterialCompiler",
        "External/Effekseer/Dev/Cpp/3rdParty",
        "External/Effekseer/Dev/Cpp/EffekseerRendererCommon",
    }

    defines { "EFK_USE_DX11" }

    filter "system:windows"
        defines { "WIN32", "_WIN32_WINNT=0x0A00" }
    filter {}

    links {
        "EffekseerRendererCommon",
        "Effekseer",
        "d3d11",
        "dxgi",
        "d3dcompiler",
    }

----------------------------------------
-- コアプロジェクト
----------------------------------------
project "Tsukino.Core"
    location ".build/Tsukino.Core"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    -- エンジン自身のソースツリー上の絶対パスをコンパイル時定数として注入する。
    -- GetEngineAssetRootPath()(Debugビルド用)が参照し、Tools/やTsukino.BuiltIn/Assetsを
    -- 取り込み側リポジトリへコピー・リンクせずに直接解決できるようにする。
    defines { 'TSUKINO_ENGINE_ROOT="' .. ROOT_ENGINE_ROOT .. '"' }

    pchheader "pch.h"
    pchsource "Tsukino.Core/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Core/src/**.cpp",
        "Tsukino.Core/include/**.hpp",
        "Tsukino.Core/pch.cpp"
    }

    includedirs {
        "Tsukino.Core/include",
        "External/cereal/include",
        "External/hlslpp/include", 
        "External/entt/single_include"
    }

----------------------------------------
-- 描画関係の共通モジュール
----------------------------------------
project "Tsukino.GraphicsCommon"
    location ".build/Tsukino.GraphicsCommon"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    pchheader "pch.h" 
    pchsource "Tsukino.GraphicsCommon/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    -- 共通モジュールのソース
    files {
        "Tsukino.GraphicsCommon/src/**.cpp",
        "Tsukino.GraphicsCommon/include/**.hpp",
        "Tsukino.GraphicsCommon/pch.cpp"
    }

    -- インクルードパス（他モジュールから参照される前提）
    includedirs {
        "Tsukino.Core/include",
        "Tsukino.GraphicsCommon/include",
        "External/cereal/include",
	    "External/hlslpp/include",
    }

    links {
        "Tsukino.Core"
    }

----------------------------------------
-- エンジンプロジェクト
----------------------------------------
project "Tsukino.Engine"
    location ".build/Tsukino.Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    pchheader "pch.h" 
    pchsource "Tsukino.Engine/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    local ENGINE_ROOT = path.getdirectory(_SCRIPT)  -- このプロジェクトが実際に置かれているディレクトリ（Engineルート）

    -- Debug時：Tools/の実体解決はGetEngineAssetRootPath()(コンパイル時に注入したTSUKINO_ENGINE_ROOT)が
    -- 直接エンジンのソースツリーを参照するため、取り込み側リポジトリへのコピー・リンクは不要。

    -- リリース時：exeの場所(GetAssetRootPath())基準になるため、
    -- FontImporter(MakeSpriteFont.exe)・AudioImporter(XWBTool.exe)が使う
    -- 外部ツール(Tools/)もexeの隣へコピーする
    filter "configurations:Release"
        postbuildcommands {
            "{COPYDIR} " .. ENGINE_ROOT .. "/Tools %{cfg.targetdir}/Tools"
        }
    filter {}

    files {
        "Tsukino.Engine/src/**.cpp",
        "Tsukino.Engine/include/**.hpp",
        "Tsukino.Engine/pch.cpp"
    }

    includedirs {
        "Tsukino.Engine/include",
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Core/include",
        "External/cereal/include",
        "External/hlslpp/include", 
        "External/entt/single_include",
        "External/DirectXTex",
        ".build/packages/AssimpCpp:5.0.1.6/build/native/include",
    }

    links {
        "Tsukino.Core",
        "Tsukino.GraphicsCommon",
        "DirectXTex",
        "EffekseerRendererDX11",
        "EffekseerRendererCommon",
        "Effekseer",
    }

    nuget {"AssimpCpp:5.0.1.6"}

----------------------------------------
-- 描画プロジェクト
----------------------------------------
project "Tsukino.Renderer"
    location ".build/Tsukino.Renderer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    pchheader "pch.h" 
    pchsource "Tsukino.Renderer/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Renderer/src/**.cpp",
        "Tsukino.Renderer/include/**.hpp",
        "Tsukino.Renderer/pch.cpp",
        "External/hlslpp/include/hlsl++.natvis",
    }

    includedirs {
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Core/include",
        "Tsukino.BuiltIn/include",
        "Tsukino.EngineIntegration/include",
        "External/cereal/include",
        "External/hlslpp/include",
        "External/entt/single_include",
        "External/Effekseer/Dev/Cpp",
        "External/Effekseer/Dev/Cpp/Effekseer",
        "External/Effekseer/Dev/Cpp/EffekseerRendererDX11",
        "External/Effekseer/Dev/Cpp/EffekseerRendererCommon",
        "External/Effekseer/Dev/Cpp/3rdParty",
        ".build/packages/directxtk_desktop_win10.2026.4.1.1/include",
    }

    links {
        "Tsukino.Core",
        "Tsukino.Engine",
        "Tsukino.GraphicsCommon",
        "EffekseerRendererDX11",
        "EffekseerRendererCommon",
        "Effekseer",
    }

    nuget { "directxtk_desktop_win10:2026.4.1.1" }

----------------------------------------
-- 物理プロジェクト
----------------------------------------
project "Tsukino.Physics"
    location ".build/Tsukino.Physics"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    --forceincludes { "pch.h" }               -- 強制インクルード

    --pchheader "pch.h"
    --pchsource "Tsukino.Physics/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Physics/src/**.cpp",
        "Tsukino.Physics/include/**.hpp",
        "Tsukino.Physics/pch.cpp"
    }

    includedirs {
        "Tsukino.Physics/include",
        "Tsukino.Core/include",
    }

    links {
        "Tsukino.Core"
    }

----------------------------------------
-- オーディオプロジェクト
----------------------------------------
project "Tsukino.Audio"
    location ".build/Tsukino.Audio"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    --forceincludes { "pch.h" }               -- 強制インクルード
    --pchheader "pch.h"
    --pchsource "Tsukino.Audio/pch.cpp"

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Audio/src/**.cpp",
        "Tsukino.Audio/include/**.hpp",
        --"Tsukino.Audio/pch.cpp"
    }

    includedirs {
        "Tsukino.Audio/include",
        "Tsukino.Core/include",
        "Tsukino.Engine/include",
        ".build/packages/directxtk_desktop_win10.2026.4.1.1/include",
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Core",
    }

    nuget { "directxtk_desktop_win10:2026.4.1.1" }

----------------------------------------
-- 組み込みプロジェクト
----------------------------------------
project "Tsukino.BuiltIn"
    location ".build/Tsukino.BuiltIn"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    pchheader "pch.h" 
    pchsource "Tsukino.BuiltIn/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    local BUILTIN_ROOT = path.getdirectory(_SCRIPT)  -- このプロジェクトが実際に置かれているディレクトリ（Engineルート）

    -- デバッグ時：Engineがworkspaceルート直下にある（単体ビルド）ならコピーせず直接参照
    filter "configurations:Debug"
        debugdir "%{wks.location}/.."
    filter {}

    -- Debug時：Assets/の実体解決はGetEngineAssetRootPath()(コンパイル時に注入したTSUKINO_ENGINE_ROOT)が
    -- 直接エンジンのソースツリーを参照するため、取り込み側リポジトリへのコピー・リンクは不要。

    -- リリース（配布）時：exeの場所を参照し、Engine自身のAssetsをそこへコピーする
    filter "configurations:Release"
        debugdir "%{cfg.targetdir}"
        postbuildcommands {
            "{COPYDIR} " .. BUILTIN_ROOT .. "/Tsukino.BuiltIn/Assets %{cfg.targetdir}/Tsukino.BuiltIn/Assets"
        }
    filter {}

    files {
        "Tsukino.BuiltIn/src/**.cpp",
        "Tsukino.BuiltIn/include/**.hpp",
        "Tsukino.BuiltIn/Assets/**.hlsl",
        "Tsukino.BuiltIn/Assets/**.hlsli",
        "Tsukino.BuiltIn/pch.cpp"
    }

    -- .hlsl/.hlsliはビルド対象から除外（.hlsliはインクルード専用でコンパイル自体行われない）
    filter  "files:**.hlsl or files:**.hlsli"
        buildaction "None"
    filter {}

    includedirs {
        "Tsukino.BuiltIn/include",
        "Tsukino.Audio/include",
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Core/include",
        "External/cereal/include",
        "External/hlslpp/include",
        "External/entt/single_include",
        "External/JoltPhysics",
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Audio",
        "Tsukino.Renderer",
        "Tsukino.GraphicsCommon",
        "Tsukino.Core",
        "JoltPhysics",
        "EffekseerRendererDX11",
        "EffekseerRendererCommon",
        "Effekseer",
    }

    nuget { "directxtk_desktop_win10:2026.4.1.1" }

----------------------------------------
-- エンジン統合プロジェクト
----------------------------------------
project "Tsukino.EngineIntegration"
    location ".build/Tsukino.EngineIntegration"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    pchheader "pch.h" 
    pchsource "Tsukino.EngineIntegration/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.EngineIntegration/src/**.cpp",
        "Tsukino.EngineIntegration/include/**.hpp",
        "Tsukino.EngineIntegration/pch.cpp",
        "External/hlslpp/include/hlsl++.natvis",
    }

    includedirs {
        "Tsukino.EngineIntegration/include",
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Audio/include",
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.Core/include",
        "Tsukino.BuiltIn/include",
        --"Tsukino.Physics/include",
        "External/cereal/include",
        "External/hlslpp/include",
        "External/entt/single_include",
        "External/JoltPhysics",
        "External/Effekseer/Dev/Cpp",
        "External/Effekseer/Dev/Cpp/Effekseer",
        "External/Effekseer/Dev/Cpp/EffekseerRendererDX11",
        "External/Effekseer/Dev/Cpp/EffekseerRendererCommon",
        "External/Effekseer/Dev/Cpp/3rdParty",
        ".build/packages/directxtk_desktop_win10.2026.4.1.1/include",
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Audio",
        "Tsukino.Renderer",
        "Tsukino.GraphicsCommon",
        "Tsukino.BuiltIn",
        "Tsukino.Core",
        "JoltPhysics",
        --"Tsukino.Physics",
        "EffekseerRendererDX11",
        "EffekseerRendererCommon",
        "Effekseer",
        "d3d11",
        "dxgi",
        "d3dcompiler",
        "dwrite"
    }

    nuget { "directxtk_desktop_win10:2026.4.1.1" }

----------------------------------------
-- サンドボックス（実行ファイル）
-- 単体ビルド時のみ有効。サブモジュールとして外部から取り込まれた場合はスキップされる。
----------------------------------------
if ROOT_IS_SAME_AS_ROOT then
project "Tsukino.Sandbox"
    location ".build/Tsukino.Sandbox"
    kind "WindowedApp"   
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    pchheader "pch.h"
    pchsource "Tsukino.Sandbox/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    local SANDBOX_ROOT     = path.getdirectory(_SCRIPT)  -- このプロジェクトが実際に置かれているディレクトリ
    local IS_SAME_AS_ROOT  = (path.getabsolute(SANDBOX_ROOT) == path.getabsolute(_MAIN_SCRIPT_DIR))

    -- デバッグ時：workspaceルート直下にある（単体ビルド）ならコピーせず直接参照
    filter "configurations:Debug"
        debugdir "%{wks.location}/.."
    filter {}

    if not IS_SAME_AS_ROOT then
        -- サブモジュール等でこのプロジェクトのルート≠workspaceルートの場合は、Debugでも
        -- 自身のAssetsをworkspaceルート直下へ同期する
        filter "configurations:Debug"
            postbuildcommands {
                "{COPYDIR} " .. SANDBOX_ROOT .. "/Tsukino.Sandbox/Assets %{wks.location}/../Tsukino.Sandbox/Assets"
            }
        filter {}
    end

    -- リリース（配布）時：exeの場所を参照し、自身のAssetsをそこへコピーする
    filter "configurations:Release"
        debugdir "%{cfg.targetdir}"
        postbuildcommands {
            "{COPYDIR} " .. SANDBOX_ROOT .. "/Tsukino.Sandbox/Assets %{cfg.targetdir}/Tsukino.Sandbox/Assets",
        }
    filter {}

    files {
        "Tsukino.Sandbox/src/**.cpp",
        "Tsukino.Sandbox/include/**.hpp",
        "Tsukino.Sandbox/pch.cpp",
        "Tsukino.Sandbox/Assets/**.hlsl"
    }

    -- .hlslはビルド対象から除外
    filter  "files:**.hlsl" 
        buildaction "None"
    filter {}

    includedirs {
        "Tsukino.Sandbox/include",
        "Tsukino.Audio/include",
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.BuiltIn/include",
        "Tsukino.EngineIntegration/include",
        --"Tsukino.Physics/include",
        "Tsukino.Core/include",
        "External/cereal/include",
        "External/hlslpp/include",
        "External/entt/single_include",
        "External/JoltPhysics",
        "External/Effekseer/Dev/Cpp",
        "External/Effekseer/Dev/Cpp/Effekseer",
        "External/Effekseer/Dev/Cpp/EffekseerRendererDX11",
        "External/Effekseer/Dev/Cpp/EffekseerRendererCommon",
        "External/Effekseer/Dev/Cpp/3rdParty",
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Renderer",
        "Tsukino.GraphicsCommon",
        "Tsukino.Audio",
        "Tsukino.BuiltIn",
        "Tsukino.EngineIntegration",
        "JoltPhysics",
        --"Tsukino.Physics",
        "Tsukino.Core",
        "EffekseerRendererDX11",
        "EffekseerRendererCommon",
        "Effekseer",
        "d3d11",
        "dxgi",
        "d3dcompiler",
        "dwrite",
    }

    nuget { "directxtk_desktop_win10:2026.4.1.1",
            "AssimpCpp:5.0.1.6",
    }
end
