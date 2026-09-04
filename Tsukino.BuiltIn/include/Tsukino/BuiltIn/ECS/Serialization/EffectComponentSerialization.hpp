//--------------------------------------------------------------
//! @file   EffectComponentSerialization.hpp
//! @brief  EffectComponent に対する cereal シリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  EffectComponent のセーブ処理
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const EffectComponent& effect) {
        // effectPathはeffectAsset（AssetRef）が持つパスと同じ値になるためJSONへは書かない
        archive(cereal::make_nvp("effectAsset", effect.effectAsset),
                cereal::make_nvp("playSpeed",   effect.playSpeed),
                cereal::make_nvp("looping",     effect.looping));
    }

    //--------------------------------------------------------------
    //! @brief  EffectComponent のロード処理
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, EffectComponent& effect) {
        archive(effect.effectAsset, effect.playSpeed, effect.looping);

        //--------------------------------------------------------------
        // AssetRefResolverArchiveによる再訪問でもこのload()が呼ばれる。
        // ローカル変数へ読んでから書き戻すと、解決パスでは空文字が読まれて
        // メンバを壊すため、必ずメンバへ直接読み込むこと
        //--------------------------------------------------------------
        effect.effectPath = Tsukino::Core::Path(effect.effectAsset.path);
        effect.handle     = -1;
        effect.stopped    = false;
        effect.active     = false;
    }

}    // namespace Tsukino::BuiltIn::ECS
