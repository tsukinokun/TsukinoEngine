//--------------------------------------------------------------
//! @file   TransformComponentSerialization.hpp
//! @brief  TransformComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  cereal用：保存（セーブ）処理の定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const TransformComponent& transform) {
        //--------------------------------------------------------------
        // インスペクタで編集する「位置・回転・スケール」の3つだけを保存する
        // 行列キャッシュや parent は除外
        //--------------------------------------------------------------
        archive(cereal::make_nvp("position", transform.position), cereal::make_nvp("rotation", transform.rotation), cereal::make_nvp("scale", transform.scale));
    }

    //--------------------------------------------------------------
    //! @brief  cereal用：読み込み（ロード）処理の定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, TransformComponent& transform) {
        //--------------------------------------------------------------
        // 保存時と同じ順番で復元
        //--------------------------------------------------------------
        archive(transform.position, transform.rotation, transform.scale);
        //--------------------------------------------------------------
        // 初期パラメータが変わったので、行列（local/world）を
        // 次フレームで強制再計算させるために dirty フラグを true にする
        //--------------------------------------------------------------
        transform.dirty = true;
    }

}    // namespace Tsukino::BuiltIn::ECS
