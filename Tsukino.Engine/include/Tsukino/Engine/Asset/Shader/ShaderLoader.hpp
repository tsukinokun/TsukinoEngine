//--------------------------------------------------------------
//! @file   ShaderLoader.hpp
//! @brief  シェーダアセットローダーの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>
#include <Tsukino/Core/Path.hpp>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  ShaderLoader
    //! @brief  コンパイル済みのシェーダー（.cso、bgfx 版は .sc.bin）を読み込んでShaderAssetを生成するローダー
    //--------------------------------------------------------------
    class ShaderLoader : public IAssetLoader {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        ShaderLoader() = default;

        //--------------------------------------------------------------
        // 対応する拡張子か判定する
        //! @param ext [in] 拡張子
        //! @return 対応している場合 true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //--------------------------------------------------------------
        // シェーダファイルを読み込みShaderAssetを生成する
        //! @param path [in] シェーダファイルのパス
        //! @return 読み込まれたShaderAsset
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;

        //--------------------------------------------------------------
        //! @brief  シェーダーステージをファイルパスから推測する関数
        //! @param  path [in] シェーダファイルのパス
        //! @return 推測されたシェーダーステージ
        //--------------------------------------------------------------
        [[nodiscard]]
        static Tsukino::Shader::ShaderStage DetectStage(const Tsukino::Core::Path& path);

        //--------------------------------------------------------------
        //! bgfx のシェーダーのソース（.sc）から、ビルド時に変換した結果のパスを返します。
        //! @param  [in] sourcePath シェーダーのソースのパス（例: Assets/Shaders/Water.ps.sc）
        //! @return 変換した結果のパス（例: Assets/Shaders/dxbc/Water.ps.sc.bin）
        //! @note   変換は tsukino_shaders()（Tools/premake/tsukino.lua）がビルド時に行い、
        //!         ソースと同じフォルダの、形式ごとのフォルダへ書き出す。
        //!         実行時には変換しない（変換の道具はアプリに入らない）
        //--------------------------------------------------------------
        [[nodiscard]]
        static Tsukino::Core::Path ToBinaryPath(const Tsukino::Core::Path& sourcePath);

        //--------------------------------------------------------------
        //! この環境で読む、変換済みのシェーダーの形式のフォルダ名を返します。
        //! @return フォルダ名（Windows の Direct3D 11 は "dxbc"）
        //! @note   名前は bgfx.cmake の決まりに合わせる（Tools/Shaders/CMakeLists.txt の出力先）
        //--------------------------------------------------------------
        [[nodiscard]]
        static const char* GetBinaryDirectoryName();
    };

}    // namespace Tsukino::Asset
