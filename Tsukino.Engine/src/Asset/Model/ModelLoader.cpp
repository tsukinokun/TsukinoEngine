//--------------------------------------------------------------
//! @file   ModelLoader.cpp
//! @brief  モデルアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Model/ModelLoader.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/Engine/Asset/Material/MaterialAsset.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Util/AssetHandleGenerator.hpp>

#include <Tsukino/Core/Log.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool ModelLoader::CanLoad(const std::string& ext) const {
        return ext == ".tsm";
    }

    //--------------------------------------------------------------
    //! @brief モデルファイルを読み込み ModelAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> ModelLoader::Load(const Tsukino::Core::Path& path) {
        const std::string filePath = path.string();

        std::ifstream ifs(filePath, std::ios::binary);
        if(!ifs.is_open()) {
            Tsukino::Core::Log::Error("ModelLoader: Failed to open model: " + filePath);
            return nullptr;
        }

        // ファイルサイズ確認
        ifs.seekg(0, std::ios::end);
        const std::streamoff fileSize = ifs.tellg();
        if(fileSize <= 0) {
            Tsukino::Core::Log::Error("ModelLoader: Empty or invalid file: " + filePath);
            return nullptr;
        }
        ifs.seekg(0, std::ios::beg);

        Tsukino::Core::Ref<ModelAsset> asset = Tsukino::Core::CreateRef<ModelAsset>();

        try {
            cereal::BinaryInputArchive archive(ifs);
            archive(asset->modelData);
        } catch (const std::exception& e) {
            Tsukino::Core::Log::Error("ModelLoader: Failed to deserialize .tsm: " + filePath + " - " + e.what());
            return nullptr;
        }

        //--------------------------------------------------------------
        // マテリアルのテクスチャをロードしてMaterialAssetを構築
        //--------------------------------------------------------------
        if(m_assetManager) {
            asset->materialHandles.resize(asset->modelData.materials.size());

            for(u32 i = 0; i < asset->modelData.materials.size(); ++i) {
                const auto& matData = asset->modelData.materials[i];

                auto matAsset  = Tsukino::Core::CreateRef<MaterialAsset>();
                matAsset->data = matData;

                // テクスチャをロード
                if(!matData.albedoMap.empty())
                    matAsset->albedoHandle = m_assetManager->Load(Tsukino::Core::Path(matData.albedoMap));
                if(!matData.normalMap.empty())
                    matAsset->normalHandle = m_assetManager->Load(Tsukino::Core::Path(matData.normalMap));
                if(!matData.metallicRoughnessMap.empty())
                    matAsset->metallicRoughnessHandle = m_assetManager->Load(Tsukino::Core::Path(matData.metallicRoughnessMap));
                if(!matData.emissiveMap.empty())
                    matAsset->emissiveHandle = m_assetManager->Load(Tsukino::Core::Path(matData.emissiveMap));
                if(!matData.aoMap.empty())
                    matAsset->aoHandle = m_assetManager->Load(Tsukino::Core::Path(matData.aoMap));

                // MaterialAssetをAssetManagerに登録してハンドルを取得。
                // マテリアルは単体のファイルを持たないため、モデルのパスと
                // マテリアル番号を合わせたものを識別キーにする
                AssetHandle matHandle = AssetHandleGenerator::GenerateFromKey(filePath + "|material|" + std::to_string(i));
                matAsset->SetHandle(matHandle);
                m_assetManager->RegisterAsset(matHandle, matAsset);

                asset->materialHandles[i] = matHandle;
            }
        }

        Tsukino::Core::Log::Info("ModelLoader: Successfully loaded .tsm (" + std::to_string(asset->modelData.nodes.size()) + " mesh nodes): " + filePath);
        
        return asset;
    }
}    // namespace Tsukino::Asset
