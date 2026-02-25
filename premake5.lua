workspace "TsukinoEngine"                   -- ソリューション名
    architecture "x64"                      -- アーキテクチャ
    configurations { "Debug", "Release" }   -- ビルド構成

    startproject "Sandbox"                  -- スタートアッププロジェクト
    location ".build"                       -- ビルドファイルの出力先