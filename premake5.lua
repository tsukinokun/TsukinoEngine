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
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

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
        "Tsukino.GraphicsCommon",
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
        "Tsukino.Core/include",
        "External/hlslpp/include", 
        "External/entt/single_include",
        "External/DirectXTex"
    }

    links {
        "Tsukino.Core",
        "DirectXTex"
    }

----------------------------------------
-- 描画プロジェクト
----------------------------------------
project "Tsukino.Renderer"
    location ".build/Tsukino.Renderer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

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
        "Tsukino.Renderer/include",
        "Tsukino.Core/include",
    }

    links {
        "Tsukino.Core"
    }

----------------------------------------
-- 物理プロジェクト
----------------------------------------
project "Tsukino.Physics"
    location ".build/Tsukino.Physics"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

    pchheader "pch.h"
    pchsource "Tsukino.Physics/pch.cpp"

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
-- サンドボックス（実行ファイル）
----------------------------------------
project "Tsukino.Sandbox"
    location ".build/Tsukino.Sandbox"
    kind "WindowedApp"   
    language "C++"
    cppdialect "C++20"
    forceincludes { "pch.h" }               -- 強制インクルード

    pchheader "pch.h"
    pchsource "Tsukino.Sandbox/pch.cpp"
    debugdir "%{cfg.targetdir}"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Sandbox/src/**.cpp",
        "Tsukino.Sandbox/include/**.hpp",
        "Tsukino.Sandbox/pch.cpp"
    }

    includedirs {
        "Tsukino.Sandbox/include",
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        --"Tsukino.Physics/include",
        "Tsukino.Core/include",
        "External/hlslpp/include",
        "External/entt/single_include"
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Renderer",
        --"Tsukino.Physics",
        "Tsukino.Core",
        "d3d11", 
        "dxgi",
        "d3dcompiler"
    }

    postbuildcommands {
    "{COPYDIR} %{wks.location}/../Tsukino.Sandbox/Assets %{cfg.targetdir}/Assets"
    }
