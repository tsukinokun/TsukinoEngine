//-------------------------------------------------------------
//! @file   PhysicsSystem.hpp
//! @brief  PhysicsSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/DebugTools/DebugFeatures.hpp>

#include <Tsukino/Physics/BodyHandle.hpp>
#include <Tsukino/Physics/PhysicsTypes.hpp>

#include <entt/entt.hpp>
#include <hlsl++.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace Tsukino::ECS {
    class EventBus;
}

namespace Tsukino::Physics {
    class PhysicsWorld;
}

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    struct CollisionComponent;    // 前方宣言

    //-------------------------------------------------------------
    //! @class  PhysicsSystem
    //! @brief  ECS と物理ワールド（Tsukino.Physics）を仲介するシステム
    //! @details
    //! 物理エンジンそのものは Tsukino::Physics::PhysicsWorld に閉じており、
    //! このクラスはコンポーネントの読み書きとイベント発行だけを担当する。
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

        //-------------------------------------------------------------
        //! @brief  指定のカプセル形状と現在重なっている全エンティティを取得する
        //!         （センサー的な即時オーバーラップ判定。物理的な反発は起きない）
        //! @param  center     [in] カプセル中心のワールド座標
        //! @param  rotation   [in] カプセルの向き（物理側のカプセルはローカルY軸方向が軸）
        //! @param  radius     [in] カプセル半径
        //! @param  halfHeight [in] カプセル円柱部分の半分の高さ
        //! @return 重なっているエンティティの一覧（CollisionComponentを持つもののみ）
        //-------------------------------------------------------------
        std::vector<entt::entity> OverlapCapsule(const hlslpp::float3& center, const hlslpp::quaternion& rotation, float radius, float halfHeight);

        //-------------------------------------------------------------
        //! @brief  物理コリジョンのデバッグワイヤーフレーム描画を有効/無効にする
        //! @param  enabled [in] true: 有効化, false: 無効化
        //! @note   宣言は構成に依らず常に存在する。
        //!         TSUKINO_DEBUG_COLLISION_DRAW が無効なビルドでは何もしない。
        //!         （Debugで通る呼び出し側コードがReleaseで壊れるのを防ぐため）
        //-------------------------------------------------------------
        void SetDebugDrawEnabled(bool enabled);

    private:
        //-------------------------------------------------------------
        //! @brief  Registry の破棄シグナルへ購読する（初回 Update で一度だけ）
        //! @param  registry [in] 購読対象のレジストリ
        //! @details
        //! ECS の外に実体を持つリソース（物理ワールドの Body / キャラクター）は、
        //! イベントバス経由では回収し損ねる。
        //! ゲームコードが Scene::DestroyEntity() を通さず
        //! Registry::DestroyEntity() を直接呼ぶ経路が存在するためである。
        //! EnTT の破棄シグナルはどの経路でも必ず発火するので、
        //! 回収は必ずこちらで行う。
        //-------------------------------------------------------------
        void ConnectRegistrySignals(Tsukino::ECS::Registry& registry);

        //-------------------------------------------------------------
        //! @brief  CollisionComponent 破棄時に物理ワールドの Body を回収する
        //! @param  registry [in] レジストリ（EnTT が渡す）
        //! @param  entity   [in] 破棄されるエンティティ
        //! @note   EnTT は「コンポーネントを取り外す直前」に呼ぶため、
        //!         ハンドラ内ではまだ bodyID を読める。
        //-------------------------------------------------------------
        void OnCollisionComponentDestroyed(entt::registry& registry, entt::entity entity);

        //-------------------------------------------------------------
        //! @brief  CharacterControllerComponent 破棄時にキャラクターを回収する
        //! @param  registry [in] レジストリ（EnTT が渡す）
        //! @param  entity   [in] 破棄されるエンティティ
        //-------------------------------------------------------------
        void OnCharacterControllerDestroyed(entt::registry& registry, entt::entity entity);

        //! 物理ワールド本体。Jolt はこの中に閉じている
        std::unique_ptr<Tsukino::Physics::PhysicsWorld> m_world;

        //! Kinematic ボディの速度算出に使う、前フレームの位置
        std::unordered_map<entt::entity, hlslpp::float3> m_prevPositions;

        //! エンティティごとのキャラクターコントローラー
        std::unordered_map<entt::entity, Tsukino::Physics::CharacterHandle> m_characters;

        //! 接触の取り出し用バッファ（毎フレーム使い回す）
        std::vector<Tsukino::Physics::ContactRecord> m_drainedContacts;

        //! 衝突イベントの発行先（メインスレッドからのみ使う）
        Tsukino::ECS::EventBus* m_eventBus = nullptr;

        // 以下2つは構成に依らず常に宣言する。
        // #ifdef で消すと sizeof(PhysicsSystem) が Debug と Release で食い違い、
        // 別々の _DEBUG でビルドされた翻訳単位が混ざったときに ODR 違反になるため。
#ifdef TSUKINO_DEBUG_COLLISION_DRAW_ALWAYS_ON
        //! デバッグ描画が有効か（ALWAYS_ONマクロにより起動時からON）
        bool m_isDebugDrawEnabled = true;
#else
        //! デバッグ描画が有効か
        bool m_isDebugDrawEnabled = false;
#endif    // TSUKINO_DEBUG_COLLISION_DRAW_ALWAYS_ON

        //! 直前フレームでF5キーが押されていたか
        bool m_f5WasDown = false;

        //! シグナル購読済みのレジストリ。デストラクタで購読解除するために保持する
        //! （System は Registry より先に破棄されるため、解除しないと
        //!   破棄済みの this へコールバックが飛ぶ）
        Tsukino::ECS::Registry* m_connectedRegistry = nullptr;
    };

}    // namespace Tsukino::BuiltIn::ECS
