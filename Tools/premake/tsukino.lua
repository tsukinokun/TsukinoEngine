----------------------------------------
-- TsukinoEngine を取り込む側のための premake ヘルパー。
--
-- ゲーム側の premake5.lua から、workspace を宣言する「前」に読み込む。
--
--   include "External/TsukinoEngine/Tools/premake/tsukino.lua"
--
--   workspace "MyGame"
--       startproject "MyGame"
--       location ".build"
--       tsukino_workspace_defaults()
--
--   include "External/TsukinoEngine"
--
--   project "MyGame"
--       kind "WindowedApp"
--       ...
--       tsukino_link()
--       tsukino_release_payload()
--
-- エンジン本体のSandboxも同じ関数を使っている。片方だけ書き換わって
-- 食い違うことが無いように、定義はここ1箇所に置くこと。
----------------------------------------

-- このファイルは <EngineRoot>/Tools/premake/ に置かれている
local TSUKINO_ROOT = path.getabsolute(path.join(path.getdirectory(_SCRIPT), "../.."))

-- エンジン本体のpremake5.luaからも参照される
_TSUKINO_ENGINE_ROOT = TSUKINO_ROOT

----------------------------------------
-- workspace 直下で呼ぶ。構成ごとの共通設定をまとめて入れる。
--
-- ここをゲーム側へ書き写すと、JPH_DEBUG_RENDERER や NDEBUG が
-- エンジンと食い違ったときに気付けなくなる。特にNDEBUGが漏れると
-- assert() がReleaseでも生き残り、EnTTのENTT_ASSERTが全コンポーネント
-- アクセスに乗ったまま製品ビルドが作られる。
----------------------------------------
function tsukino_workspace_defaults()
    architecture "x64"                      -- DX11バックエンドはx64のみ
    configurations { "Debug", "Release" }   -- 以下のfilterがこの名前を前提にしている
    multiprocessorcompile "On"
    exceptionhandling "On"

    filter "configurations:*"
        defines { "JPH_DEBUG_RENDERER" }    -- 値は1でなくても定義されていることが重要
        linkoptions { "/IGNORE:4006" }
    filter {}

    filter "configurations:Debug"
        optimize "Off"
        symbols "On"
    filter {}

    filter "configurations:Release"
        optimize "Full"
        symbols "On"
        defines { "NDEBUG" }                -- premakeは自動では付けない
    filter {}

    filter "action:vs*"
        buildoptions { "/utf-8" }
    filter {}
end

----------------------------------------
-- 実行ファイルのproject直下で呼ぶ。
-- エンジンのinclude・lib・NuGetをまとめて設定する。
----------------------------------------
function tsukino_link()
    local root = _TSUKINO_ENGINE_ROOT

    includedirs {
        root .. "/Tsukino.Core/include",
        root .. "/Tsukino.GraphicsCommon/include",
        root .. "/Tsukino.Engine/include",
        root .. "/Tsukino.Renderer/include",
        root .. "/Tsukino.Physics/include",
        root .. "/Tsukino.Audio/include",
        root .. "/Tsukino.BuiltIn/include",
        root .. "/Tsukino.EngineIntegration/include",

        root .. "/External/cereal/include",
        root .. "/External/hlslpp/include",
        root .. "/External/entt/single_include",

        root .. "/External/Effekseer/Dev/Cpp",
        root .. "/External/Effekseer/Dev/Cpp/Effekseer",
        root .. "/External/Effekseer/Dev/Cpp/EffekseerRendererDX11",
        root .. "/External/Effekseer/Dev/Cpp/EffekseerRendererCommon",
        root .. "/External/Effekseer/Dev/Cpp/3rdParty",
    }

    links {
        "Tsukino.Core",
        "Tsukino.GraphicsCommon",
        "Tsukino.Engine",
        "Tsukino.Renderer",
        "Tsukino.Physics",
        "Tsukino.Audio",
        "Tsukino.BuiltIn",
        "Tsukino.EngineIntegration",

        "Effekseer",
        "EffekseerRendererCommon",
        "EffekseerRendererDX11",

        "d3d11",
        "dxgi",
        "d3dcompiler",
        "dwrite",
    }

    nuget {
        "directxtk_desktop_win10:2026.4.1.1",
        "AssimpCpp:5.0.1.6",
    }
end

----------------------------------------
-- 実行ファイルのproject直下で呼ぶ。
-- 実行時の基準ディレクトリと、Release配布物のうちエンジンが持ち込む分を設定する。
--
-- Debug : Tsukino::Core::FileSystem::GetEngineAssetRootPath() がコンパイル時定数の
--         TSUKINO_ENGINE_ROOT からエンジンのソースツリーを直接見るためコピー不要
-- Release: exeの場所が基準になるため、組み込みアセットと外部ツールを隣へ置く
--
-- ライセンス条文も同時に置く。cerealやhlslppはヘッダオンリーでexeにコードが
-- 取り込まれるため、exeを配った時点でMIT/BSD-3のバイナリ再配布に当たる。
-- リポジトリに置いてあるだけではこの配布経路の条件を満たさない。
--
-- ゲーム自身のAssetsは各プロジェクトで別途コピーすること。
----------------------------------------
function tsukino_release_payload()
    -- postbuildcommands は includedirs と違い premake が相対化してくれないため、
    -- 絶対パスを渡すと生成物にマシン固有のパスが焼き付く。
    -- workspace ルートからエンジンへの相対パスを自分で作って使う
    local rel  = path.getrelative(_MAIN_SCRIPT_DIR, _TSUKINO_ENGINE_ROOT)
    local root = "%{wks.location}/../" .. rel

    filter "configurations:Debug"
        debugdir "%{wks.location}/.."
    filter {}

    filter "configurations:Release"
        debugdir "%{cfg.targetdir}"
        postbuildcommands {
            "{COPYDIR} "  .. root .. "/Tsukino.BuiltIn/Assets %{cfg.targetdir}/Tsukino.BuiltIn/Assets",
            "{COPYDIR} "  .. root .. "/Tools %{cfg.targetdir}/Tools",
            "{COPYFILE} " .. root .. "/LICENSE %{cfg.targetdir}/LICENSE",
            "{COPYFILE} " .. root .. "/THIRD_PARTY_NOTICES.md %{cfg.targetdir}/THIRD_PARTY_NOTICES.md",
        }
    filter {}
end
