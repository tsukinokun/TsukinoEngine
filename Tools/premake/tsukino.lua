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
--       tsukino_icon("Assets/Icon/App.ico")   -- 任意。exe とウィンドウのアイコン
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
        linkoptions { "/IGNORE:4006" }
    filter {}

    filter "configurations:Debug"
        optimize "Off"
        symbols "On"
        -- Jolt のデバッグ描画（PhysicsWorld::DebugDraw*）。Release では消えて no-op になる。
        -- Jolt の型の中身が変わるため、Jolt プロジェクト側の定義（エンジンの premake5.lua）と
        -- 必ず同じ構成に揃えること。値は1でなくても定義されていることが重要
        defines { "JPH_DEBUG_RENDERER" }
    filter {}

    filter "configurations:Release"
        optimize "Full"
        symbols "On"                        -- .pdb は残す（exe には入らない）
        defines { "NDEBUG" }                -- premakeは自動では付けない

        -- exe を小さくする。/GL + /LTCG で翻訳単位をまたいだインライン化と不要コードの除去
        linktimeoptimization "On"
        -- /Gy（関数単位）/Gw（グローバル変数単位）でセクションを分け、/OPT がそれを捨てられるようにする
        functionlevellinking "On"
        buildoptions { "/Gw" }
        -- symbols "On" だと /DEBUG が付き、/OPT:REF・/OPT:ICF が既定で無効になるため明示する
        linkoptions { "/OPT:REF", "/OPT:ICF" }
        -- /INCREMENTAL は /OPT・/LTCG と両立しない（付いたままだと警告を出して無視される）
        incrementallink "Off"
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
        "imm32",
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
            "{COPYFILE} " .. root .. "/LICENSE %{cfg.targetdir}/LICENSE",
            "{COPYFILE} " .. root .. "/THIRD_PARTY_NOTICES.md %{cfg.targetdir}/THIRD_PARTY_NOTICES.md",
        }
        postbuildcommands(tsukino_crt_commands())
        -- Tools/ のうち実行時に要るのは外部ツールの exe だけ。premake/ はビルド用のスクリプト
        postbuildcommands(tsukino_mirror_commands(path.join(rel, "Tools"), "Tools", {
            excludeDirs = { "premake" },
        }))
    filter {}
end

----------------------------------------
-- Release の postbuildcommands に渡す、VC++ ランタイムの DLL を exe の隣へ置くコマンド列を返す。
--
-- exe は動的 CRT（/MD）でリンクされ、VC++ 再頒布可能パッケージが入っていない PC では
-- 「vcruntime140.dll が見つかりません」で起動できない。ビルド済みの assimp の DLL と
-- directxtk の lib も /MD なので、/MT に切り替えても解決しない。
-- UCRT（api-ms-win-crt-*）は Windows 10 以降に標準で入っているので含めない。
--
-- 足りないまま配布物ができないよう、見つからなければビルドを失敗させる。
-- コピーするのはビルドに使ったツールセットの Redist フォルダの DLL で、
-- exe が必要とするバージョンと揃う。
----------------------------------------
function tsukino_crt_commands()
    local crt = "$(VCToolsRedistInstallDir)x64\\Microsoft.VC$(PlatformToolsetVersion).CRT\\"
    local commands = {}
    for _, dll in ipairs({ "vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll" }) do
        table.insert(commands, 'copy /Y "' .. crt .. dll .. '" "$(TargetDir)" >nul || exit /b 1')
    end
    return commands
end

----------------------------------------
-- Release の postbuildcommands に渡す、フォルダを exe の隣へ同期するコマンド列を返す。
--
--   srcFromRoot : ワークスペースルート（ゲーム側 premake5.lua のある場所）からの相対パス
--   dstFromTarget: exe の出力先（targetdir）からの相対パス
--   opts.excludeFiles: コピーしないファイルのパターン（例 { "*.efkproj" }）
--   opts.excludeDirs : コピーしないフォルダ名（例 { "premake" }）
--
-- robocopy /MIR を使う。xcopy と違い、変わっていないファイルは書き直さず、元から消えた
-- ファイルは出力先からも消える。終了コード 0〜7 は成功（1 = コピーした、2 = 余分を消した…）
-- で、8 以上だけが失敗。VS の postbuild は最後のコマンドの終了コードで成否を決めるため、
-- 8 以上なら止め、最後に 0 へ戻す。
--
-- /XF・/XD に当たるファイルは、出力先に前のビルドの分が残っていても /MIR では消えない。
-- 除外を足したときに古い分が配布物に残らないよう、後から明示的に消す。
--
-- パスは VS のマクロ（$(SolutionDir) = wks.location、$(TargetDir) = cfg.targetdir）で書く。
-- {COPYDIR} と違い premake がパスを変換しないので、区切りも \ に揃える。
----------------------------------------
function tsukino_mirror_commands(srcFromRoot, dstFromTarget, opts)
    opts = opts or {}
    local excludeFiles = opts.excludeFiles or {}
    local excludeDirs  = opts.excludeDirs or {}

    local src = '"$(SolutionDir)..\\' .. path.translate(srcFromRoot, "\\") .. '"'
    local dstDir = "$(TargetDir)" .. path.translate(dstFromTarget, "\\")
    local dst = '"' .. dstDir .. '"'

    local copy = "robocopy " .. src .. " " .. dst .. " /MIR /R:2 /W:1 /NFL /NDL /NJH /NJS /NP"
    if #excludeFiles > 0 then copy = copy .. " /XF " .. table.concat(excludeFiles, " ") end
    if #excludeDirs  > 0 then copy = copy .. " /XD " .. table.concat(excludeDirs, " ") end

    local commands = { copy .. " & if errorlevel 8 exit /b 1" }

    if #excludeFiles > 0 then
        local patterns = {}
        for _, pattern in ipairs(excludeFiles) do
            table.insert(patterns, '"' .. dstDir .. "\\" .. pattern .. '"')
        end
        table.insert(commands, "del /s /q " .. table.concat(patterns, " ") .. " >nul 2>&1")
    end
    for _, dir in ipairs(excludeDirs) do
        local target = '"' .. dstDir .. "\\" .. dir .. '"'
        table.insert(commands, "if exist " .. target .. " rmdir /s /q " .. target)
    end

    -- robocopy の 1〜7 や del の失敗が postbuild 全体の失敗にならないよう 0 に戻す
    table.insert(commands, "cmd /c exit 0")
    return commands
end

----------------------------------------
-- 実行ファイルのproject直下で、location を指定した「後」に呼ぶ（任意）。
-- .ico を exe のリソースに埋め込み、エクスプローラー上の exe のアイコンにする。
-- Window::Create() が同じリソースを読むので、タイトルバー・タスクバー・
-- Alt+Tab のアイコンも揃う。
--
--   iconPath: ワークスペースルート（ゲーム側 premake5.lua のある場所）からの相対パス
--
-- .rc はプロジェクトの location へ生成する。中身の .ico のパスは絶対パスになるが、
-- 生成物なのでリポジトリには入らない。
-- リソース名 TSUKINO_APP_ICON は Window.cpp の kAppIconResourceName と揃えること。
----------------------------------------
function tsukino_icon(iconPath)
    local icon = path.getabsolute(path.join(_MAIN_SCRIPT_DIR, iconPath))
    if not os.isfile(icon) then
        premake.warn("tsukino_icon: アイコンが見つかりません: %s（既定のアイコンのままビルドします）", icon)
        return
    end

    local prj = project()
    local dir = prj.location or prj.basedir
    local rc  = path.join(dir, "TsukinoAppIcon.rc")

    -- rc の文字列では \ がエスケープ文字になるため、区切りは / のまま書く。
    -- 中身が同じなら書き直さない（更新日時が変わると、premake を回すたびに
    -- リソースのコンパイルとリンクがやり直しになる）
    os.mkdir(dir)
    if os.writefile_ifnotequal("TSUKINO_APP_ICON ICON \"" .. icon .. "\"\n", rc) < 0 then
        premake.error("tsukino_icon: %s を書き出せませんでした", rc)
    end

    files { rc }
end
