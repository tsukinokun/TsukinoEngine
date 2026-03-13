//--------------------------------------------------------------
//! @file   MeshAsset.hpp
//! @brief  メッシュアセットの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>

#include <Tsukino/GraphicsCommon/Mesh/MeshData.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class   MeshAsset
    //! @brief   メッシュアセットクラス
    //! @details GPU 非依存のメッシュデータを保持するアセットクラス
    //--------------------------------------------------------------
    class MeshAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        MeshAsset() = default;

        //--------------------------------------------------------------
        //! @brief  アセットのハンドルを取得する関数
        //! @return アセットのハンドル
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetHandle GetHandle() const override {
            return m_handle;
        }

        //--------------------------------------------------------------
        //! @brief  アセットの種類を取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        [[nodiscard]]
        AssetType GetType() const override {
            return AssetType::Mesh;
        }

        //--------------------------------------------------------------
        //! @brief ローダー側から設定されるハンドル
        //! @param handle [in] 設定するハンドル
        //--------------------------------------------------------------
        void SetHandle(AssetHandle handle) { m_handle = handle; }

        // メッシュデータ
        Tsukino::GraphicsCommon::MeshData data;

    private:
        AssetHandle m_handle;
    };

}    // namespace Tsukino::Asset
