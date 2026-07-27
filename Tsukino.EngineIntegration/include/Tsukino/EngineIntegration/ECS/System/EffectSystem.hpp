//-------------------------------------------------------------
//! @file   EffectSystem.hpp
//! @brief  エフェクト再生システムの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/ECS/Entity/Entity.hpp>
#include <Tsukino/Core/ECS/Event/EventBus.hpp>
#include <Tsukino/Core/ECS/Event/ScopedConnection.hpp>
#include <Tsukino/Engine/ECS/EngineEvent/EntityEvent.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <Effekseer.h>
#include <EffekseerRendererDX11.h>
#include <EffekseerRendererCommon/EffekseerRendererCommon/TextureLoader.h>
#include <Tsukino/EngineIntegration/IO/EffectFileInterface.hpp>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>

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
        //! @param  dc          [in] D3D11 デバイスコンテキスト
        //! @param  view        [in] ビュー行列（カメラ）
        //! @param  projection  [in] 射影行列
        //--------------------------------------------------------------
        void RenderEffects(ID3D11DeviceContext* dc,
                           const Tsukino::Core::Math::matrix& view,
                           const Tsukino::Core::Math::matrix& projection);

        //-------------------------------------------------------------
        //! @brief  エフェクトを再生する
        //! @param  registry   [in] ECS レジストリ
        //! @param  asset      [in] 再生するエフェクトアセット
        //! @param  effectPath [in] エフェクトファイルのパス
        //! @param  position   [in] 再生位置 (x, y, z)
        //! @param  looping    [in] ループ再生するか
        //! @return エフェクトハンドル（負値の場合は失敗）
        //-------------------------------------------------------------
        int PlayEffect(Tsukino::ECS::Registry& registry, Tsukino::Asset::AssetHandle asset, const Tsukino::Core::Path& effectPath, const float* position, bool looping = false);

        //-------------------------------------------------------------
        //! @brief  指定したハンドルのエフェクトを停止する
        //! @param  handle  [in] 停止するエフェクトハンドル
        //-------------------------------------------------------------
        void StopEffect(int handle);

        //-------------------------------------------------------------
        //! @brief  指定したハンドルのエフェクトを一時停止する
        //! @param  handle  [in] 一時停止するエフェクトハンドル
        //-------------------------------------------------------------
        void PauseHandle(int handle);

        //-------------------------------------------------------------
        //! @brief  指定したハンドルのエフェクトを再開する
        //! @param  handle  [in] 再開するエフェクトハンドル
        //-------------------------------------------------------------
        void ResumeHandle(int handle);

        //-------------------------------------------------------------
        //! @brief  指定したハンドルが再生中か取得する
        //! @param  handle  [in] 確認するエフェクトハンドル
        //! @return 再生中なら true
        //-------------------------------------------------------------
        [[nodiscard]]
        bool IsPlaying(int handle) const;

        //-------------------------------------------------------------
        //! @brief  指定したハンドルの再生速度を設定する
        //! @param  handle  [in] 対象エフェクトハンドル
        //! @param  speed   [in] 再生速度
        //-------------------------------------------------------------
        void SetPlaySpeed(int handle, float speed);

        //-------------------------------------------------------------
        //! @brief  指定したハンドルへトリガーを送信する
        //! @param  handle  [in] 対象エフェクトハンドル
        //! @param  index   [in] トリガーインデックス
        //-------------------------------------------------------------
        void SendTrigger(int handle, int32_t index = 0);

        //-------------------------------------------------------------
        //! @brief  全てのエフェクトを停止する
        //-------------------------------------------------------------
        void StopAllEffects();

        //-------------------------------------------------------------
        //! @brief  Effekseer初期化済みか
       //! @return true:初期化済み / false:未初期化
        //-------------------------------------------------------------
        [[nodiscard]]
        bool IsInitialized() const {
            return m_initialized;
        }

        //-------------------------------------------------------------
        //! @brief  Effekseerを初期化する
        //! @param  registry      [in] ECS レジストリ
        //! @param  maxParticles  [in] 最大パーティクル数
        //-------------------------------------------------------------
        void Initialize(Tsukino::ECS::Registry& registry, Tsukino::ECS::EventBus& eventBus, int maxParticles = 20000);

        //-------------------------------------------------------------
        //! @brief  Effekseerを終了する
        //-------------------------------------------------------------
        void Finalize();

    private:
        //-------------------------------------------------------------
        //! @brief  エンティティ破棄時のコールバック
        //! @param  event [in] エンティティ破棄イベント
        //-------------------------------------------------------------
        void OnEffectEntityDestroyed(const Tsukino::ECS::EngineEvent::EntityDestroyedEvent& event);

        // 読み込み済みエフェクト管理
        std::unordered_map<Tsukino::Asset::AssetHandle, Effekseer::EffectRef> m_loadedEffects;

        Effekseer::ManagerRef              m_manager;
        EffekseerRendererDX11::RendererRef m_renderer;

        Tsukino::ECS::Registry*        m_registry = nullptr;
        Tsukino::ECS::ScopedConnection m_entityDestroyedConn;

        Effekseer::TextureLoaderRef m_textureLoader;
        Tsukino::EngineIntegration::EffectFileInterface* m_effectFileInterface = nullptr;
        Effekseer::RefPtr<Tsukino::EngineIntegration::EffectFileInterface> m_effectFileInterfaceRef;

        bool m_initialized = false;
    };

}    // namespace Tsukino::BuiltIn::ECS