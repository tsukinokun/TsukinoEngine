//--------------------------------------------------------------
//! @file   CubemapLoader.hpp
//! @brief  キューブマップアセットローダーの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  CubemapLoader
    //! @brief  キューブマップアセットのローダー
    //--------------------------------------------------------------
    class CubemapLoader : public IAssetLoader {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        CubemapLoader() = default;

        //--------------------------------------------------------------
        //! @brief 対応する拡張子か判定する
        //! @param  ext [in] 拡張子
        //! @return 対応している場合は true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //--------------------------------------------------------------
        //! @brief キューブマップファイルを読み込み CubemapAsset を生成する
        //! @param  path [in] .tccファイルのパス
        //! @return 読み込まれた CubemapAsset（失敗時は nullptr）
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;
    };
}    // namespace Tsukino::Asset
