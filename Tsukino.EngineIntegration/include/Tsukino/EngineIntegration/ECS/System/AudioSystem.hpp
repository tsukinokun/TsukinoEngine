#pragma once
//--------------------------------------------------------------
//! @file   AudioSystem.hpp
//! @brief  オーディオコンポーネントを処理するシステムの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @class  AudioSystem
    //! @brief  AudioComponentを持つエンティティの音声再生を管理するシステム
    //--------------------------------------------------------------
    class AudioSystem : public Tsukino::ECS::ISystem {
    public:
        //--------------------------------------------------------------
        //! @brief  システムの更新処理
        //! @param  registry   [in] ECSレジストリ
        //! @param  deltaTime  [in] 経過時間 (秒)
        //--------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;
    };
}
