-- このファイルが実際に置かれているディレクトリ（Engineルート）
local ROOT_ENGINE_ROOT     = path.getdirectory(_SCRIPT)
-- TsukinoEngine自身がルートスクリプト（単体ビルド）かどうか
local ROOT_IS_SAME_AS_ROOT = (path.getabsolute(ROOT_ENGINE_ROOT) == path.getabsolute(_MAIN_SCRIPT_DIR))

----------------------------------------
-- エージェント向けマニフェスト生成アクションを登録する。
--
--   premake5 gen-manifest
--
-- ビルドには一切影響しない。ここで登録しておくことで、ゲーム側リポジトリの
-- ルートから include されたときも同じアクションが使えるようになる。
----------------------------------------
-- 取り込み側と共用するヘルパー（_TSUKINO_ENGINE_ROOT もここで設定される）。
-- Sandbox もゲーム側の実行ファイルもこの同じ関数を通す。定義を1箇所に
-- 置くことで、include/link/配布物が2箇所に分かれて食い違うのを防ぐ
dofile(path.join(ROOT_ENGINE_ROOT, "Tools/premake/tsukino.lua"))
dofile(path.join(ROOT_ENGINE_ROOT, "Tools/premake/gen_manifest.lua"))
dofile(path.join(ROOT_ENGINE_ROOT, "Tools/premake/gen_api_digest.lua"))

-- 単体ビルド時のみworkspaceを宣言する。
-- 外部の親premake5.lua（サブモジュールとして取り込んだ側）からinclude()された場合は、
-- 親側が既にworkspaceを宣言している前提のためここではスキップする。
if ROOT_IS_SAME_AS_ROOT then
workspace "TsukinoEngine"                   -- ソリューション名
    architecture "x64"                      -- アーキテクチャ
    configurations { "Debug", "Release" }   -- ビルド構成

    startproject "Tsukino.Sandbox"          -- スタートアッププロジェクト
    location ".build"                       -- ビルドファイルの出力先

    -- アーキテクチャ・構成・警告まわりの共通設定。
    -- 取り込み側のworkspaceも同じ関数を呼ぶ（Tools/premake/tsukino.lua）
    tsukino_workspace_defaults()
end

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
	-- JPH_DEBUG_RENDERER は Jolt の型の中身を変えるため、Jolt を include する側
	-- （tsukino_workspace_defaults() の Debug）と必ず同じ構成でだけ定義すること
	filter "configurations:Debug"
		defines {
			"JPH_DEBUG_RENDERER=1",
		}
	filter {}

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
    }

----------------------------------------
-- SDL3（ウィンドウ・入力・IME・ゲームパッド・保存先などのプラットフォーム層）
--
-- SDL は CMake でしかビルドできないため、ここでは CMake を呼ぶだけのプロジェクトにする。
-- できた静的ライブラリ（SDL3-static.lib）は tsukino_link() がリンクする。
-- 構成名（Debug / Release）をそのまま CMake の構成に渡すので、CRT（/MDd・/MD）も揃う。
--
-- CMake は Visual Studio に同梱のものを使い、無ければ PATH の cmake を使う。
-- 2回目以降は CMakeCache.txt があるので構成し直さず、変わったところだけビルドする。
----------------------------------------
project "SDL3"
    location ".build/SDL3"
    kind "Utility"

    prebuildcommands(tsukino_sdl3_build_commands())

----------------------------------------
-- bgfx（描画の翻訳レイヤー）
--
-- SDL3 と同じく、CMake を呼ぶだけのプロジェクト。できた静的ライブラリは tsukino_link() がリンクする。
-- プロジェクト名をライブラリ名（bgfx）と同じにすると、premake が links の "bgfx" を
-- このプロジェクトへの参照と取り違えて、ライブラリをリンクしなくなる。
-- 初回の構成とビルドは数分かかる。2回目からは変わったところだけビルドする。
----------------------------------------
project "bgfx-cmake"
    location ".build/bgfx"
    kind "Utility"

    prebuildcommands(tsukino_bgfx_build_commands())

----------------------------------------
-- 組み込みシェーダーの変換（.sc → bgfx の形式）
--
-- ゲームのシェーダーと同じ tsukino_shaders() で変換する（shaderc を使うので bgfx-cmake の後に走る）。
-- 出力は Tsukino.BuiltIn/Assets/Shaders/bgfx/<形式>/ に置かれ、実行時に BuiltInShaders が読む。
----------------------------------------
project "bgfx-shaders"
    location ".build/bgfx-shaders"
    kind "Utility"

    tsukino_shaders("Tsukino.BuiltIn/Assets/Shaders/bgfx")
    tsukino_shaders("Tsukino.BuiltIn/Assets/Shaders/bgfx/Effekseer")    -- Effekseer の標準シェーダー（src/Bgfx/Effekseer が使う）

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
        "External/entt/single_include",
        "External/SDL/include",
    }

    dependson { "SDL3" }

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
        ".build/packages/AssimpCpp:5.0.1.6/build/native/include",
        -- 画像の読み込み・圧縮（bimg）。ヘッダは Engine の src/Asset/Texture/ImageCodec.cpp だけが使う
        "External/bimg/include",
        "External/bx/include",
        "External/bx/include/compat/msvc",
    }

    -- bx が要求する設定（bgfx.cmake の bx.cmake と揃える。Tsukino.Renderer と同じ）
    defines { "__STDC_LIMIT_MACROS", "__STDC_FORMAT_MACROS", "__STDC_CONSTANT_MACROS" }
    buildoptions { "/Zc:__cplusplus", "/Zc:preprocessor" }
    filter "configurations:Debug"
        defines { "BX_CONFIG_DEBUG=1" }
    filter "configurations:Release"
        defines { "BX_CONFIG_DEBUG=0" }
    filter {}
    dependson { "bgfx-cmake" }

    links {
        "Tsukino.Core",
        "Tsukino.GraphicsCommon",
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
        "Tsukino.Renderer/src/**.hpp",
        "Tsukino.Renderer/include/**.hpp",
        "Tsukino.Renderer/internal/**.hpp",
        "Tsukino.Renderer/pch.cpp",
        "External/hlslpp/include/hlsl++.natvis",
    }

    --------------------------------------------------------------------
    -- 描画は bgfx（src/Bgfx）。bgfx のヘッダは Renderer の中だけで使う
    --------------------------------------------------------------------
    includedirs {
        "External/bgfx/include",
        "External/bimg/include",
        "External/bx/include",
        "External/bx/include/compat/msvc",
    }
    -- bx が要求する設定（bgfx.cmake の bx.cmake と揃える）
    defines { "__STDC_LIMIT_MACROS", "__STDC_FORMAT_MACROS", "__STDC_CONSTANT_MACROS" }
    buildoptions { "/Zc:__cplusplus", "/Zc:preprocessor" }
    filter "configurations:Debug"
        defines { "BX_CONFIG_DEBUG=1" }
    filter "configurations:Release"
        defines { "BX_CONFIG_DEBUG=0" }
    filter {}
    dependson { "bgfx-cmake", "bgfx-shaders" }

    -- Tsukino.BuiltIn / Tsukino.EngineIntegration は上位層のため参照しない。
    -- Worldパスの後に差し込む描画は Tsukino/Renderer/IPostWorldPass.hpp の
    -- インターフェース越しに受け取る（CODING_GUIDELINES.md §5「依存関係は一方向」）
    includedirs {
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.Renderer/internal",    -- Renderer の中だけで使うヘッダ（描画 API の型を含む）。他のモジュールからは見えない
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Core/include",
        "External/cereal/include",
        "External/hlslpp/include",
        "External/entt/single_include",
        "External/Effekseer/Dev/Cpp",
        "External/Effekseer/Dev/Cpp/Effekseer",
        "External/Effekseer/Dev/Cpp/EffekseerRendererCommon",
        "External/Effekseer/Dev/Cpp/3rdParty",
        "External/bgfx/3rdparty/stb",    -- stb_truetype（文字のラスタライズ。DynamicFontAtlas.cpp だけが実装ごと取り込む）
    }

    links {
        "Tsukino.Core",
        "Tsukino.Engine",
        "Tsukino.GraphicsCommon",
        "EffekseerRendererCommon",
        "Effekseer",
    }

----------------------------------------
-- 物理プロジェクト
----------------------------------------
project "Tsukino.Physics"
    location ".build/Tsukino.Physics"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    --forceincludes { "pch.h" }               -- 強制インクルード

    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Physics/src/**.cpp",
        "Tsukino.Physics/src/**.hpp",
        "Tsukino.Physics/include/**.hpp",
    }

    includedirs {
        "Tsukino.Physics/include",
        "Tsukino.Core/include",
        "Tsukino.GraphicsCommon/include",
        "External/hlslpp/include",
        "External/cereal/include",
        "External/JoltPhysics",
    }

    links {
        "Tsukino.Core",
        "Tsukino.GraphicsCommon",
        "JoltPhysics",
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
        "External/miniaudio/miniaudio.c",    -- miniaudio の本体（C）。wav / mp3 / flac の復号と、OS ごとの音の出力
    }

    -- 使わない機能を外す（エンコード・波形の生成）
    defines { "MA_NO_ENCODING", "MA_NO_GENERATION" }
    filter "files:External/miniaudio/miniaudio.c"
        warnings "Off"
    filter {}

    includedirs {
        "Tsukino.Audio/include",
        "Tsukino.Core/include",
        "Tsukino.Engine/include",
        "External/miniaudio",
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Core",
    }


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

    -- Debug時：Assets/の実体解決はGetEngineAssetRootPath()(コンパイル時に注入したTSUKINO_ENGINE_ROOT)が
    -- 直接エンジンのソースツリーを参照するため、取り込み側リポジトリへのコピー・リンクは不要。
    -- Release時：exeの隣が基準になる。組み込みAssetsのコピーは exe 側の tsukino_release_payload() が行う

    files {
        "Tsukino.BuiltIn/src/**.cpp",
        "Tsukino.BuiltIn/include/**.hpp",
        "Tsukino.BuiltIn/pch.cpp"
    }

    includedirs {
        "Tsukino.BuiltIn/include",
        "Tsukino.Audio/include",
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Core/include",
        "Tsukino.Physics/include",
        "External/cereal/include",
        "External/hlslpp/include",
        "External/entt/single_include",
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Audio",
        "Tsukino.Renderer",
        "Tsukino.GraphicsCommon",
        "Tsukino.Core",
        "EffekseerRendererCommon",
        "Effekseer",
    }

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
        -- EngineIntegration.cpp は組み込みコンポーネントのシリアライズを全部取り込むため、既定のセクション数を超える
        buildoptions { "/permissive-", "/bigobj" }
    filter {}

    pchheader "pch.h" 
    pchsource "Tsukino.EngineIntegration/pch.cpp"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.EngineIntegration/src/**.cpp",
        "Tsukino.EngineIntegration/src/**.hpp",
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
        "Tsukino.Physics/include",
        "External/cereal/include",
        "External/hlslpp/include",
        "External/entt/single_include",
        "External/Effekseer/Dev/Cpp",
        "External/Effekseer/Dev/Cpp/Effekseer",
        "External/Effekseer/Dev/Cpp/EffekseerRendererCommon",
        "External/Effekseer/Dev/Cpp/3rdParty",
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Audio",
        "Tsukino.Renderer",
        "Tsukino.GraphicsCommon",
        "Tsukino.BuiltIn",
        "Tsukino.Core",
        "Tsukino.Physics",
        "EffekseerRendererCommon",
        "Effekseer",
        "dxgi",
    }

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

    -- 実行時の基準ディレクトリと、エンジンが持ち込むRelease配布物
    -- （組み込みAssets / Tools / ライセンス条文）はヘルパーが設定する
    tsukino_release_payload()

    -- exe とウィンドウのアイコン。image/Logo.png の顔のあたり（左上 175,65 から 330px 四方）を
    -- 16〜256px の各サイズに縮めて1つの .ico にまとめたもの
    tsukino_icon("image/Icon.ico")

    if not IS_SAME_AS_ROOT then
        -- サブモジュール等でこのプロジェクトのルート≠workspaceルートの場合は、Debugでも
        -- 自身のAssetsをworkspaceルート直下へ同期する
        filter "configurations:Debug"
            postbuildcommands {
                "{COPYDIR} " .. SANDBOX_ROOT .. "/Tsukino.Sandbox/Assets %{wks.location}/../Tsukino.Sandbox/Assets"
            }
        filter {}
    end

    -- リリース（配布）時：Sandbox自身のAssetsをexeの隣へ置く
    filter "configurations:Release"
        postbuildcommands {
            "{COPYDIR} " .. SANDBOX_ROOT .. "/Tsukino.Sandbox/Assets %{cfg.targetdir}/Tsukino.Sandbox/Assets",
        }
    filter {}

    files {
        "Tsukino.Sandbox/src/**.cpp",
        "Tsukino.Sandbox/include/**.hpp",
        "Tsukino.Sandbox/pch.cpp",
    }

    -- エンジンのinclude・lib・NuGetをまとめて設定する。
    -- ゲーム側リポジトリの実行ファイルも同じ関数を呼ぶ
    tsukino_link()
    includedirs { "Tsukino.Sandbox/include" }
end

----------------------------------------
-- 公開ヘッダの自己完結チェック（単体ビルド時のみ）
--
-- エンジン本体は forceincludes "pch.h" でビルドしているため、公開ヘッダが
-- <string> や <algorithm> を自前で include していなくても通ってしまう。
-- 一方 tsukino_link() は取り込み側にPCHを強制しないので、ゲーム側が
-- 独自のpchを持たない構成にした途端にコンパイルが壊れる。
--
-- そこで「1ヘッダを1つだけincludeする翻訳単位」を全公開ヘッダぶん生成し、
-- PCH無し・/permissive- でコンパイルする。生成物はgitignore対象。
----------------------------------------
if ROOT_IS_SAME_AS_ROOT then

local HEADERCHECK_DIR = path.join(ROOT_ENGINE_ROOT, "Tools/HeaderCheck/generated")
local headerCheckStubs = {}

os.mkdir(HEADERCHECK_DIR)

-- 既存の生成物を消してから作り直す（ヘッダを削除したときに取り残さない）
for _, stale in ipairs(os.matchfiles(HEADERCHECK_DIR .. "/*.cpp")) do
    os.remove(stale)
end

for _, moduleName in ipairs({
    "Tsukino.Core", "Tsukino.GraphicsCommon", "Tsukino.Engine", "Tsukino.Renderer",
    "Tsukino.Physics", "Tsukino.Audio", "Tsukino.BuiltIn", "Tsukino.EngineIntegration",
}) do
    local includeRoot = path.join(ROOT_ENGINE_ROOT, moduleName, "include")
    for _, header in ipairs(os.matchfiles(includeRoot .. "/**.hpp")) do
        -- include で書くときのパス（例: Tsukino/Core/Path.hpp）
        local relative = path.getrelative(includeRoot, header)
        local stubName = relative:gsub("[/%.]", "_") .. ".cpp"
        local stubPath = path.join(HEADERCHECK_DIR, stubName)

        io.writefile(stubPath, table.concat({
            "// 自動生成。premake5 の実行のたびに作り直される（手で編集しない）",
            "// このヘッダ1本だけをincludeし、単体でコンパイルが通ることを確認する。",
            "#include <" .. relative .. ">",
            "",
        }, "\n"))

        table.insert(headerCheckStubs, stubPath)
    end
end

project "Tsukino.HeaderCheck"
    location ".build/Tsukino.HeaderCheck"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    -- PCHを付けないことがこのプロジェクトの目的そのもの
    filter "action:vs*"
        buildoptions { "/permissive-" }
    filter {}

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files(headerCheckStubs)

    tsukino_link()

end
