//--------------------------------------------------------------
//! @file   CollisionComponentSerialization.hpp
//! @brief  CollisionComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  CollisionComponentのcerealシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const CollisionComponent& collision) {
        // bodyID・isInitialized・ハイトフィールドの実データ（HeightmapGenerationSystemが
        // 実行時に生成する）・onCollisionEnterコールバックは保存しない
        archive(cereal::make_nvp("type", collision.type),
                cereal::make_nvp("extent", collision.extent),
                cereal::make_nvp("offsetPosition", collision.offsetPosition),
                cereal::make_nvp("offsetRotation", collision.offsetRotation),
                cereal::make_nvp("isSensor", collision.isSensor));
    }

    //--------------------------------------------------------------
    //! @brief  CollisionComponentのcerealデシリアライズ定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, CollisionComponent& collision) {
        archive(collision.type, collision.extent, collision.offsetPosition, collision.offsetRotation, collision.isSensor);
        // ロード直後はJolt側のBodyが未生成の状態にする
        collision.isInitialized = false;
    }

}    // namespace Tsukino::BuiltIn::ECS
