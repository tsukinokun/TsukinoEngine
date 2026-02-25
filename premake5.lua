workspace "TsukinoEngine"                   -- ソリューション名
    architecture "x64"                      -- アーキテクチャ
    configurations { "Debug", "Release" }   -- ビルド構成

    startproject "Sandbox"                  -- スタートアッププロジェクト
    location ".build"                       -- ビルドファイルの出力先


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
