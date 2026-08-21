//--------------------------------------------------------------
//! @file   DynamicFontLoader.hpp
//! @brief  動的フォントアセットローダーの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  DynamicFontLoader
    //! @brief  .dfont キャッシュファイルを読み込み DynamicFontAsset を生成するローダー
    //--------------------------------------------------------------
    class DynamicFontLoader : public IAssetLoader {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        DynamicFontLoader() = default;

        //--------------------------------------------------------------
        // 対応する拡張子か判定する
        //! @param  ext [in] 拡張子
        //! @return 対応している場合は true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //--------------------------------------------------------------
        // .dfont キャッシュファイルを読み込み DynamicFontAsset を生成する
        //! @param  path [in] .dfont キャッシュファイルのパス
        //! @return 読み込まれた DynamicFontAsset（失敗時は nullptr）
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;
    };

}    // namespace Tsukino::Asset
