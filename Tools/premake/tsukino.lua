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
    architecture "x64"
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
        root .. "/External/SDL/include",

        root .. "/External/Effekseer/Dev/Cpp",
        root .. "/External/Effekseer/Dev/Cpp/Effekseer",
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

        "dxgi",      -- 使う GPU を選ぶ（Tsukino.Renderer の GraphicsContext）
        "imm32",
        "dwmapi",    -- Window の ClickThrough（DwmExtendFrameIntoClientArea）

        -- SDL3 の静的ライブラリと、SDL が使う Windows のライブラリ
        -- （External/SDL/CMakeLists.txt の sdl_link_dependency(base ...) と揃える）
        "SDL3-static",
        "winmm",
        "ole32",
        "oleaut32",
        "version",
        "uuid",
        "advapi32",
        "setupapi",
        "shell32",
    }

    libdirs { "%{wks.location}/SDL3/build/%{cfg.buildcfg}" }

    -- bgfx とその土台（bimg・bx）。bgfx のヘッダは Tsukino.Renderer の中だけで使うので、includedirs は足さない
    links { "bgfx", "bimg_encode", "bimg_decode", "bimg", "bx" }
    libdirs { "%{wks.location}/bgfx/build/%{cfg.buildcfg}" }

    nuget {
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
    filter {}
end

----------------------------------------
-- エンジンの SDL3 プロジェクト（kind "Utility"）の prebuildcommands に渡す、
-- External/SDL を CMake で静的ライブラリにビルドするコマンド列を返す。
--
-- 出力は <wks.location>/SDL3/build/<構成>/SDL3-static.lib（tsukino_link() の libdirs と揃える）。
-- SDL のソースの場所は、ワークスペースルートからの相対パスを VS のマクロで書く
-- （絶対パスを焼き込むと、生成物がマシン固有になる。tsukino_release_payload() と同じ理由）。
----------------------------------------
function tsukino_sdl3_build_commands()
    local rel   = path.translate(path.getrelative(_MAIN_SCRIPT_DIR, _TSUKINO_ENGINE_ROOT), "\\")
    local src   = '"$(SolutionDir)..\\' .. rel .. '\\External\\SDL"'
    local build = '"$(SolutionDir)SDL3\\build"'

    return {
        'set "TSUKINO_CMAKE=$(VSInstallDir)Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe"',
        'if not exist "%TSUKINO_CMAKE%" set "TSUKINO_CMAKE=cmake"',
        'if not exist ' .. build .. '\\CMakeCache.txt "%TSUKINO_CMAKE%" -S ' .. src .. ' -B ' .. build
            .. ' -A x64 -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF -DSDL_EXAMPLES=OFF'
            .. ' -DSDL_INSTALL=OFF -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=' .. build .. ' >nul || exit /b 1',
        '"%TSUKINO_CMAKE%" --build ' .. build .. ' --config $(Configuration) -- /nologo /v:q /m "/clp:ErrorsOnly;NoSummary" || exit /b 1',
    }
end

----------------------------------------
-- エンジンの bgfx プロジェクト（kind "Utility"）の prebuildcommands に渡す、
-- bgfx・bimg・bx を CMake で静的ライブラリにビルドするコマンド列を返す。
--
-- CMake の手順は External/bgfx.cmake を使い、ソースは External/bgfx・bimg・bx
-- （bgfx.cmake が固定しているのと同じコミットの submodule）を渡す。
-- 渡さないと bgfx.cmake が構成のたびにネットから取ってくる。
-- 出力は <wks.location>/bgfx/build/<構成>/ に bgfx.lib・bimg.lib・bimg_decode.lib・bimg_encode.lib・bx.lib
-- （tsukino_link() の libdirs と揃える）。作り方は tsukino_sdl3_build_commands() と同じ。
--
-- シェーダーの変換ツール shaderc も作る（<wks.location>/bgfx/build/Release/shaderc.exe）。
-- shaderc はビルドの道具なので、構成に関係なく Release で1回だけ作る。
-- shaderc が使う glslang・SPIRV-Tools・SPIRV-Cross・tint は External/bgfx/3rdparty に
-- 入っているので、ネットにはつながない。初回は shaderc の分だけさらに数分かかる。
--
-- CMake の設定を変えたら TSUKINO_BGFX_OPTIONS_REV を1つ上げる。前の設定で作った
-- ビルドフォルダが残っていても、キャッシュにこの番号が無ければ構成し直す。
-- テクスチャの上限（BGFX_CONFIG_MAX_TEXTURES）は既定の 4096 では足りない
-- （ChronoSort はカードの画像を 4,000 枚以上読む）ので広げる。
----------------------------------------
function tsukino_bgfx_build_commands()
    local rel   = path.translate(path.getrelative(_MAIN_SCRIPT_DIR, _TSUKINO_ENGINE_ROOT), "\\")
    local ext   = '$(SolutionDir)..\\' .. rel .. '\\External\\'
    local build = '"$(SolutionDir)bgfx\\build"'
    local rev   = "2"
    local msbuild = ' -- /nologo /v:q /m "/clp:ErrorsOnly;NoSummary" || exit /b 1'

    return {
        'set "TSUKINO_CMAKE=$(VSInstallDir)Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe"',
        'if not exist "%TSUKINO_CMAKE%" set "TSUKINO_CMAKE=cmake"',
        'findstr /c:"TSUKINO_BGFX_OPTIONS_REV:STRING=' .. rev .. '" ' .. build .. '\\CMakeCache.txt >nul 2>&1 || "%TSUKINO_CMAKE%" -S "' .. ext .. 'bgfx.cmake" -B ' .. build
            .. ' -A x64 -DTSUKINO_BGFX_OPTIONS_REV:STRING=' .. rev
            .. ' -DBGFX_BUILD_TOOLS=ON -DBGFX_BUILD_TOOLS_SHADER=ON -DBGFX_BUILD_TOOLS_BIN2C=OFF'
            .. ' -DBGFX_BUILD_TOOLS_GEOMETRY=OFF -DBGFX_BUILD_TOOLS_TEXTURE=OFF'
            .. ' -DBGFX_BUILD_EXAMPLES=OFF -DBGFX_INSTALL=OFF -DBGFX_CUSTOM_TARGETS=OFF'
            .. ' -DBGFX_CONFIG_MAX_TEXTURES=16384'
            .. ' "-DBX_DIR=' .. ext .. 'bx" "-DBIMG_DIR=' .. ext .. 'bimg" "-DBGFX_DIR=' .. ext .. 'bgfx"'
            .. ' -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=' .. build .. ' -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=' .. build .. ' >nul || exit /b 1',
        '"%TSUKINO_CMAKE%" --build ' .. build .. ' --config $(Configuration) --target bgfx bimg bimg_decode bimg_encode bx' .. msbuild,
        '"%TSUKINO_CMAKE%" --build ' .. build .. ' --config Release --target shaderc' .. msbuild,
    }
end

----------------------------------------
-- project 直下で呼ぶ。
-- dir にある bgfx のシェーダー（*.vs.sc = 頂点、*.ps.sc = ピクセル）を、ビルドの前に
-- bgfx の形式へ変換する。エンジンの組み込みシェーダーも、ゲームのシェーダーもこれを使う。
--
--   tsukino_shaders("MyGame/Assets/Shaders")
--
--   dir: 呼んだ premake スクリプトからの相対パス（files と同じ）
--
-- 出力は <dir>/<形式>/<名前>.sc.bin（Windows は dxbc = s_5_0）。生成物なので .gitignore に入れる。
-- 実行時は AssetManager::Load("<dir>/Water.ps.sc") のように .sc のパスで読むと、
-- この出力が読まれる（ShaderLoader::ToBinaryPath）。実行時には変換しない。
-- Release の配布物へは、アセットのフォルダごとコピーすれば出力も一緒に入る。
--
-- 入出力の宣言（varying.def.sc）は dir にあればそれを、無ければ組み込みのものを使う。
-- #include は dir → 組み込みの Shaders/bgfx の順に探すので、Scene.sh などをそのまま使える。
--
-- 手順は Tools/Shaders/CMakeLists.txt（bgfx.cmake の bgfx_compile_shaders を使う）に書いてあり、
-- ここはそれを CMake で構成してビルドするだけ。変わった .sc だけを変換し直す。
-- shaderc は tsukino_bgfx_build_commands() が作ったものを使うので、bgfx-cmake の後に走らせる。
-- 中で filter を使うので、filter の外（project の直下）で呼ぶこと。
----------------------------------------
function tsukino_shaders(dir)
    local rel    = path.translate(path.getrelative(_MAIN_SCRIPT_DIR, path.getabsolute(dir)), "\\")
    local engine = path.translate(path.getrelative(_MAIN_SCRIPT_DIR, _TSUKINO_ENGINE_ROOT), "\\")
    local build  = '"$(SolutionDir)shaders\\' .. rel:gsub("[\\/:%.]", "_") .. '"'
    local rev    = "2"

    files { dir .. "/*.sc", dir .. "/*.sh" }
    filter "files:**.sc or **.sh"
        buildaction "None"
    filter {}

    dependson { "bgfx-cmake" }

    prebuildcommands {
        'set "TSUKINO_CMAKE=$(VSInstallDir)Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe"',
        'if not exist "%TSUKINO_CMAKE%" set "TSUKINO_CMAKE=cmake"',
        'findstr /c:"TSUKINO_SHADERS_OPTIONS_REV:STRING=' .. rev .. '" ' .. build .. '\\CMakeCache.txt >nul 2>&1 || "%TSUKINO_CMAKE%" -S "$(SolutionDir)..\\' .. engine .. '\\Tools\\Shaders" -B ' .. build
            .. ' -A x64 -DTSUKINO_SHADERS_OPTIONS_REV:STRING=' .. rev
            .. ' "-DTSUKINO_SHADERC=$(SolutionDir)bgfx\\build\\Release\\shaderc.exe"'
            .. ' "-DTSUKINO_SHADER_DIR=$(SolutionDir)..\\' .. rel .. '" >nul || exit /b 1',
        -- .sc を足した・消したときは、CMake がファイルの一覧を取り直してプロジェクトを作り直す。
        -- 同じビルドの中では作り直す前の一覧で変換するので、先に取り直しだけ（ZERO_CHECK）を済ませる。
        -- これが無いと、足した .sc が変換されるのは次のビルドからになる
        '"%TSUKINO_CMAKE%" --build ' .. build .. ' --config Release --target ZERO_CHECK -- /nologo /v:q "/clp:ErrorsOnly;NoSummary" || exit /b 1',
        '"%TSUKINO_CMAKE%" --build ' .. build .. ' --config Release -- /nologo /v:q /m "/clp:ErrorsOnly;NoSummary" || exit /b 1',
    }
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
--
-- 新しい VS で古いツールセット（VS 18 で v143）を使うと、VCToolsRedistInstallDir は
-- 新しい Redist（14.5x）を指し、中には Microsoft.VC145.CRT しか無い。
-- ツールセットの名前のフォルダが無ければ、そこにある CRT を使う
-- （CRT は新しいものが古いツールセットの exe でも動く）。
----------------------------------------
function tsukino_crt_commands()
    local redist = "$(VCToolsRedistInstallDir)x64\\"
    local commands = {
        'set "TSUKINO_CRT=' .. redist .. 'Microsoft.VC$(PlatformToolsetVersion).CRT"',
        'if not exist "%TSUKINO_CRT%" for /d %%d in ("' .. redist .. 'Microsoft.VC*.CRT") do set "TSUKINO_CRT=%%d"',
    }
    for _, dll in ipairs({ "vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll" }) do
        table.insert(commands, 'copy /Y "%TSUKINO_CRT%\\' .. dll .. '" "$(TargetDir)" >nul || exit /b 1')
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
