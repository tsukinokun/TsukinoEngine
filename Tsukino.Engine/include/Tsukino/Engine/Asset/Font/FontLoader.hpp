//--------------------------------------------------------------
//! @file   FontLoader.hpp
//! @brief  フォントアセットローダーの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  FontLoader
    //! @brief  フォントアセットのローダー
    //--------------------------------------------------------------
    class FontLoader : public IAssetLoader {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        FontLoader() = default;

        //--------------------------------------------------------------
        // 対応する拡張子か判定する
        //! @param  ext [in] 拡張子
        //! @return 対応している場合は true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //--------------------------------------------------------------
        // フォントファイルを読み込み FontAsset を生成する
        //! @param  path [in] フォントファイルのパス
        //! @return 読み込まれた FontAsset（失敗時は nullptr）
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;
    };

}    // namespace Tsukino::Asset
