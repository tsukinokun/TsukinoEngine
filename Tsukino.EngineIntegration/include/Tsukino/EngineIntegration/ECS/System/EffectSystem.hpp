//-------------------------------------------------------------
//! @file   EffectSystem.hpp
//! @brief  エフェクト再生システムの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>

#include <Effekseer.h>
#include <EffekseerRendererDX11.h>
#include <unordered_map>

// 前方宣言
struct ID3D11DeviceContext;

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  EffectSystem
    //! @brief  EffectComponentを持つエンティティのエフェクト再生を管理するシステム
    //-------------------------------------------------------------
    class EffectSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief  デフォルトコンストラクタ
        //-------------------------------------------------------------
        EffectSystem() = default;

        //-------------------------------------------------------------
        //! @brief  デストラクタ
        //-------------------------------------------------------------
        ~EffectSystem() override;

        //-------------------------------------------------------------
        //! @brief  システムの更新（エフェクトの再生・停止制御）
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

        //-------------------------------------------------------------
        //! @brief  エフェクトの描画（D3D11デバイスコンテキストでEffekseerを実行）
        //! @param  dc  [in] D3D11 デバイスコンテキスト
        //-------------------------------------------------------------
        void RenderEffects(ID3D11DeviceContext* dc);

        //-------------------------------------------------------------
        //! @brief  エフェクトを再生する
        //! @param  registry   [in] ECS レジストリ
        //! @param  asset      [in] 再生するエフェクトアセット
        //! @param  position   [in] 再生位置 (x, y, z)
        //! @return エフェクトハンドル（負値の場合は失敗）
        //-------------------------------------------------------------
        int PlayEffect(Tsukino::ECS::Registry& registry,
                       Tsukino::Asset::AssetHandle asset,
                       const float* position);

        //-------------------------------------------------------------
        //! @brief  指定したハンドルのエフェクトを停止する
        //! @param  handle  [in] 停止するエフェクトハンドル
        //-------------------------------------------------------------
        void StopEffect(int handle);

        //-------------------------------------------------------------
        //! @brief  全てのエフェクトを停止する
        //-------------------------------------------------------------
        void StopAllEffects();

        //-------------------------------------------------------------
        //! @brief  Effekseer初期化済みか
        //! @return true:初期化済み / false:未初期化
        //-------------------------------------------------------------
        bool IsInitialized() const { return m_initialized; }

        //-------------------------------------------------------------
        //! @brief  Effekseerを初期化する
        //! @param  registry      [in] ECS レジストリ
        //! @param  maxParticles  [in] 最大パーティクル数
        //-------------------------------------------------------------
        void Initialize(Tsukino::ECS::Registry& registry, int maxParticles = 20000);

        //-------------------------------------------------------------
        //! @brief  Effekseerを終了する
        //-------------------------------------------------------------
        void Finalize();

    private:
        //-------------------------------------------------------------
        //! @brief  エンティティ破棄時のコールバック
        //! @param  registry [in] ECS レジストリ
        //! @param  entity   [in] 破棄されたエンティティ
        //! @param  comp     [in] 破棄されたEffectComponent
        //-------------------------------------------------------------
        void OnEffectEntityDestroyed(Tsukino::ECS::Registry& registry, Tsukino::ECS::Entity entity, const EffectComponent& comp);

        // 読み込み済みエフェクト管理
        std::unordered_map<Tsukino::Asset::AssetHandle, Effekseer::EffectRef> m_loadedEffects;

        Effekseer::ManagerRef m_manager;
        EffekseerRendererDX11::RendererRef m_renderer;

        bool m_initialized = false;
    };

}    // namespace Tsukino::BuiltIn::ECS
