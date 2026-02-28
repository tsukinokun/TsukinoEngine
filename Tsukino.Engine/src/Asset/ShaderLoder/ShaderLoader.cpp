//--------------------------------------------------------------
//! @file   ShaderLoader.cpp
//! @brief  シェーダアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Shader/ShaderLoader.hpp>
#include <fstream>
#include <sstream>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool ShaderLoader::CanLoad(const std::string& ext) const {
        return ext == ".shader" || ext == ".hlsl";
    }

    //--------------------------------------------------------------
    //! @brief シェーダファイルを読み込みShaderAssetを生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> ShaderLoader::Load(const Tsukino::Core::Path& path) {
        //--------------------------------------------------------------
        // ファイルを開く
        //--------------------------------------------------------------
        std::ifstream file(path.string());

        // ファイルが開けなかった場合は nullptr を返す
        if(!file.is_open()) {
            return nullptr;
        }

        //--------------------------------------------------------------
        // ファイルの内容を文字列に読み込む
        //--------------------------------------------------------------
        std::stringstream ss;
        ss << file.rdbuf();    //バッファをうつす

        //--------------------------------------------------------------
        // ShaderAssetを生成
        //--------------------------------------------------------------
        auto asset      = Tsukino::Core::CreateRef<ShaderAsset>();    // ShaderAssetを生成
        asset->source   = ss.str();                                   // 読み込んだ内容をセット
        asset->filePath = path.string();                              // 元ファイルのパスもセット

        // アセットを返す
        return asset;
    }

}    // namespace Tsukino::Asset
