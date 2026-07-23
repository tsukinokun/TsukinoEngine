//--------------------------------------------------------------
//! @file   EffectLoader.hpp
//! @brief  エフェクトアセットローダーの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  EffectLoader
    //! @brief  .efk エフェクトファイルのローダー
    //--------------------------------------------------------------
    class EffectLoader : public IAssetLoader {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        EffectLoader() = default;

        //--------------------------------------------------------------
        //! 対応する拡張子か判定する
        //! @param  ext [in] 拡張子
        //! @return 対応している場合はtrue
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //--------------------------------------------------------------
        //! .efk ファイルを読み込み EffectAsset を生成する
        //! @param  path [in] エフェクトファイルのパス
        //! @return 読み込まれた EffectAsset
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;
    };

}    // namespace Tsukino::Asset
