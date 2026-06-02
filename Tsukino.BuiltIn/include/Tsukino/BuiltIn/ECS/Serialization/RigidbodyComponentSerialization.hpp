//--------------------------------------------------------------
//! @file   RigidbodyComponentSerialization.hpp
//! @brief  RigidbodyComponentに対するcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  RigidbodyComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const RigidbodyComponent& rb) {
        archive(cereal::make_nvp("type", rb.type),
                cereal::make_nvp("mass", rb.mass),
                cereal::make_nvp("friction", rb.friction),
                cereal::make_nvp("restitution", rb.restitution),
                cereal::make_nvp("gravityFactor", rb.gravityFactor),
                cereal::make_nvp("linearVelocity", rb.linearVelocity),
                cereal::make_nvp("angularVelocity", rb.angularVelocity));
    }

    //--------------------------------------------------------------
    //! @brief  RigidbodyComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, RigidbodyComponent& rb) {
        archive(rb.type, rb.mass, rb.friction, rb.restitution, rb.gravityFactor, rb.linearVelocity, rb.angularVelocity);

        // ロード時は必ず初期化未完了状態にする
        rb.isInitialized = false;
    }

}    // namespace Tsukino::BuiltIn::ECS
