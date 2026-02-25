workspace "TsukinoEngine"                   -- ソリューション名
    architecture "x64"                      -- アーキテクチャ
    configurations { "Debug", "Release" }   -- ビルド構成

    startproject "Sandbox"                  -- スタートアッププロジェクト
    location ".build"                       -- ビルドファイルの出力先

-- コアプロジェクト
project "Tsukino.Core"
    location ".build/Tsukino.Core"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Core/src/**.cpp",
        "Tsukino.Core/include/**.hpp"
    }

    includedirs {
        "Tsukino.Core/include",
        "External/hlslpp/include", 
        "External/entt/single_include"
    }

-- エンジンプロジェクト
project "Tsukino.Engine"
    location ".build/Tsukino.Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Engine/src/**.cpp",
        "Tsukino.Engine/include/**.hpp"
    }

    includedirs {
        "Tsukino.Engine/include",
        "Tsukino.Core/include",
        "External/hlslpp/include", 
        "External/entt/single_include"
    }

    links {
        "Tsukino.Core"
    }

-- 描画プロジェクト
project "Tsukino.Renderer"
    location ".build/Tsukino.Renderer"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Renderer/src/**.cpp",
        "Tsukino.Renderer/include/**.hpp"
    }

    includedirs {
        "Tsukino.Renderer/include",
        "Tsukino.Core/include",
    }

    links {
        "Tsukino.Core"
    }

-- 物理プロジェクト
project "Tsukino.Physics"
    location ".build/Tsukino.Physics"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Physics/src/**.cpp",
        "Tsukino.Physics/include/**.hpp"
    }

    includedirs {
        "Tsukino.Physics/include",
        "Tsukino.Core/include",
    }

    links {
        "Tsukino.Core"
    }

    -- サンドボックス（実行ファイル）
project "Tsukino.Sandbox"
    location ".build/Tsukino.Sandbox"
    kind "WindowedApp"   
    language "C++"
    cppdialect "C++20"

    targetdir ("bin/%{cfg.buildcfg}")
    objdir ("bin-int/%{cfg.buildcfg}")

    files {
        "Tsukino.Sandbox/src/**.cpp",
        "Tsukino.Sandbox/include/**.hpp"
    }

    includedirs {
        "Tsukino.Sandbox/include",
        "Tsukino.Engine/include",
        "Tsukino.Renderer/include",
        "Tsukino.Physics/include",
        "Tsukino.Core/include",
        "External/hlslpp/include",
        "External/entt/single_include"
    }

    links {
        "Tsukino.Engine",
        "Tsukino.Renderer",
        "Tsukino.Physics",
        "Tsukino.Core"
    }
