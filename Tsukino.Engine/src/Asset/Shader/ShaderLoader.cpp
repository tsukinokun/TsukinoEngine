//--------------------------------------------------------------
//! @file   ShaderLoader.cpp
//! @brief  シェーダアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Shader/ShaderLoader.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool ShaderLoader::CanLoad(const std::string& ext) const {
        return ext == ".cso";
    }

    //--------------------------------------------------------------
    //! @brief シェーダファイルを読み込みShaderAssetを生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> ShaderLoader::Load(const Tsukino::Core::Path& path) {
        std::string filePath = path.string();

        // バイナリファイルを開く
        std::ifstream file(filePath, std::ios::binary);
        if(!file.is_open()) {
            return nullptr;
        }

        // アセット生成
        Tsukino::Core::Ref<ShaderAsset> asset = Tsukino::Core::CreateRef<ShaderAsset>();
        asset->filePath                       = filePath;

        // ステージ判定
        asset->shaderStage = DetectStage(path);

        // プロファイル設定（ディスパッチテーブル）
        using Stage = Tsukino::Shader::ShaderStage;

        // ステージに対応するシェーダープロファイルを定義
        static const std::unordered_map<Stage, std::string> profileTable = {
            {Stage::Vertex,   "vs_5_0"},
            {Stage::Pixel,    "ps_5_0"},
            {Stage::Compute,  "cs_5_0"},
            {Stage::Geometry, "gs_5_0"},
            {Stage::Hull,     "hs_5_0"},
            {Stage::Domain,   "ds_5_0"},
        };

        // ステージに対応するプロファイルを設定（Unknown の場合は空文字列）
        if(auto it = profileTable.find(asset->shaderStage); it != profileTable.end()) {
            asset->profile = it->second;
        } else {
            asset->profile = "";    // Unknown
        }

        // エントリーポイントは固定（.cso はコンパイル済みなので本来不要）
        asset->entryPoint = "Main";

        // バイナリ読み込み
        asset->binary = std::vector<u8>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        // アセットを返す
        return asset;
    }

    //--------------------------------------------------------------
    //! @brief  シェーダーステージをファイルパスから推測する関数
    //--------------------------------------------------------------
    Tsukino::Shader::ShaderStage ShaderLoader::DetectStage(const Tsukino::Core::Path& path) {
        using Stage = Tsukino::Shader::ShaderStage;

        static const std::unordered_map<std::string, Stage> table = {
            {".vs.cso", Stage::Vertex  },
            {".ps.cso", Stage::Pixel   },
            {".cs.cso", Stage::Compute },
            {".gs.cso", Stage::Geometry},
            {".hs.cso", Stage::Hull    },
            {".ds.cso", Stage::Domain  },
        };

        std::string name = path.filename();

        // ファイル名の末尾が特定のサフィックスで終わるかをチェックしてステージを判定
        for(const auto& [suffix, stage] : table) {
            if(name.ends_with(suffix)) {
                return stage;
            }
        }

        // どのサフィックスにもマッチしない場合は Unknown を返す
        return Stage::Unknown;
    }

}    // namespace Tsukino::Asset
