//-------------------------------------------------------------
//! @file   PhysicsSystem.hpp
//! @brief  PhysicsSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/DebugTools/DebugFeatures.hpp>

namespace Tsukino::ECS {
    class EventBus;
}

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    struct CollisionComponent;    // 前方宣言

    //-------------------------------------------------------------
    //! @class  PhysicsSystem
    //-------------------------------------------------------------
    class PhysicsSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief コンストラクタ
        //-------------------------------------------------------------
        explicit PhysicsSystem(Tsukino::ECS::EventBus& eventBus);

        //-------------------------------------------------------------
        //! @brief デストラクタ
        //-------------------------------------------------------------
        ~PhysicsSystem() override;

        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

#ifdef TSUKINO_DEBUG_COLLISION_DRAW
        //-------------------------------------------------------------
        //! @brief  物理コリジョンのデバッグワイヤーフレーム描画を有効/無効にする
        //! @param  enabled [in] true: 有効化, false: 無効化
        //-------------------------------------------------------------
        void SetDebugDrawEnabled(bool enabled);
#endif    // TSUKINO_DEBUG_COLLISION_DRAW

    private:
        //-------------------------------------------------------------
        //! @brief  Registry の破棄シグナルへ購読する（初回 Update で一度だけ）
        //! @param  registry [in] 購読対象のレジストリ
        //! @details
        //! ECS の外に実体を持つリソース（Jolt の Body / CharacterVirtual）は、
        //! イベントバス経由では回収し損ねる。
        //! ゲームコードが Scene::DestroyEntity() を通さず
        //! Registry::DestroyEntity() を直接呼ぶ経路が存在するためである。
        //! EnTT の破棄シグナルはどの経路でも必ず発火するので、
        //! 回収は必ずこちらで行う。
        //-------------------------------------------------------------
        void ConnectRegistrySignals(Tsukino::ECS::Registry& registry);

        //-------------------------------------------------------------
        //! @brief  CollisionComponent 破棄時に Jolt の Body を回収する
        //! @param  registry [in] レジストリ（EnTT が渡す）
        //! @param  entity   [in] 破棄されるエンティティ
        //! @note   EnTT は「コンポーネントを取り外す直前」に呼ぶため、
        //!         ハンドラ内ではまだ bodyID を読める。
        //-------------------------------------------------------------
        void OnCollisionComponentDestroyed(entt::registry& registry, entt::entity entity);

        //-------------------------------------------------------------
        //! @brief  CharacterControllerComponent 破棄時に CharacterVirtual を回収する
        //! @param  registry [in] レジストリ（EnTT が渡す）
        //! @param  entity   [in] 破棄されるエンティティ
        //-------------------------------------------------------------
        void OnCharacterControllerDestroyed(entt::registry& registry, entt::entity entity);

        struct Impl;
        Impl* m_impl;

        //! シグナル購読済みのレジストリ。デストラクタで購読解除するために保持する
        //! （System は Registry より先に破棄されるため、解除しないと
        //!   破棄済みの this へコールバックが飛ぶ）
        Tsukino::ECS::Registry* m_connectedRegistry = nullptr;
    };

}    // namespace Tsukino::BuiltIn::ECS
