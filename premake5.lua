workspace "TsukinoEngine"                   -- ソリューション名
    architecture "x64"                      -- アーキテクチャ
    configurations { "Debug", "Release" }   -- ビルド構成

    startproject "Sandbox"                  -- スタートアッププロジェクト
    location ".build"                       -- ビルドファイルの出力先 
    
    filter "configurations:Debug" 
        optimize "Off" 
        symbols "On" 

    filter "action:vs*" 
        buildoptions { "/utf-8" }
    filter {}
    
    filter "configurations:Release"
        optimize "Full"
        symbols "On" 
        
    filter {}

    filter "configurations:*"
        linkoptions { "/IGNORE:4006" }
    filter {}

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
        "External/hlslpp/include", 
        "External/entt/single_include",
        "External/DirectXTex",
        "External/cereal/include",
    }

    links {
        "Tsukino.Core",
        "Tsukino.GraphicsCommon",
        "DirectXTex"
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
        "Tsukino.Renderer/pch.cpp"
    }

    includedirs {
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Core/include",
        "External/hlslpp/include", 
        ".build/packages/directxtk_desktop_win10.2026.4.1.1/include",
    }

    links {
        "Tsukino.Core",
        "Tsukino.Engine",
        "Tsukino.GraphicsCommon",
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

    -- デバッグ時：ルートを参照、コピーもしない
    filter "configurations:Debug"
        debugdir "%{wks.location}/.."

    -- リリース（配布）時：exeの場所を参照、アセットをコピーする
    filter "configurations:Release"
        debugdir "%{cfg.targetdir}"
        postbuildcommands {
            "{COPYDIR} %{wks.location}/../Tsukino.BuiltIn/Assets %{cfg.targetdir}/Assets"
        }
    filter {}

    files {
        "Tsukino.BuiltIn/src/**.cpp",
        "Tsukino.BuiltIn/include/**.hpp",
        "Tsukino.BuiltIn/Assets/**.hlsl",
        "Tsukino.BuiltIn/pch.cpp"
    }

    -- .hlslはビルド対象から除外
    filter  "files:**.hlsl" 
        buildaction "None"
    filter {}

    includedirs {
        "Tsukino.BuiltIn/include",
        "Tsukino.Audio/include",
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Core/include",
        "External/hlslpp/include",
        "External/entt/single_include",
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Audio",
        "Tsukino.Renderer",
        "Tsukino.GraphicsCommon",
        "Tsukino.Core",
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
        "Tsukino.EngineIntegration/pch.cpp"
    }

    includedirs {
        "Tsukino.GraphicsCommon/include",
        "Tsukino.Audio/include",
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.Core/include",
        "Tsukino.BuiltIn/include",
        "Tsukino.EngineIntegration/include",
        --"Tsukino.Physics/include",
        "External/hlslpp/include",
        "External/entt/single_include",
        ".build/packages/directxtk_desktop_win10.2026.4.1.1/include",
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Audio",
        "Tsukino.Renderer",
        "Tsukino.GraphicsCommon",
        "Tsukino.BuiltIn",
         "Tsukino.Core",
        --"Tsukino.Physics",
        "d3d11", 
        "dxgi",
        "d3dcompiler"
    }

    nuget { "directxtk_desktop_win10:2026.4.1.1" }

----------------------------------------
-- サンドボックス（実行ファイル）
----------------------------------------
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

    -- デバッグ時：ルートを参照、コピーもしない
    filter "configurations:Debug"
        debugdir "%{wks.location}/.."

    -- リリース（配布）時：exeの場所を参照、アセットをコピーする
    filter "configurations:Release"
        debugdir "%{cfg.targetdir}"
        postbuildcommands {
            "{COPYDIR} %{wks.location}/../Tsukino.Sandbox/Assets %{cfg.targetdir}/Assets",
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
        "External/hlslpp/include",
        "External/entt/single_include"
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Renderer",
        "Tsukino.GraphicsCommon",
        "Tsukino.Audio",
        "Tsukino.BuiltIn",
        "Tsukino.EngineIntegration",
        --"Tsukino.Physics",
        "Tsukino.Core",
        "d3d11", 
        "dxgi",
        "d3dcompiler"
    }

    nuget { "directxtk_desktop_win10:2026.4.1.1" }
