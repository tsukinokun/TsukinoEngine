//--------------------------------------------------------------
//! @file   EffectSystem.cpp
//! @brief  エフェクト再生システムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/EffectSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/EngineIntegration/IO/EffectFileInterface.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Effect/EffectAsset.hpp>
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Engine/ECS/EngineEvent/EntityEvent.hpp>
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>
#include <EffekseerRendererCommon/EffekseerRendererCommon/TextureLoader.h>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  デストラクタ
    //--------------------------------------------------------------
    EffectSystem::~EffectSystem() {
        Finalize();
    }

    //--------------------------------------------------------------
    //! @brief  Effekseerを初期化する
    //! @param  registry      [in] ECS レジストリ
    //! @param  maxParticles  [in] 最大パーティクル数
    //--------------------------------------------------------------
    void EffectSystem::Initialize(Tsukino::ECS::Registry& registry, Tsukino::ECS::EventBus& eventBus, int maxParticles) {
        if(m_initialized) {
            return;
        }

        m_registry = &registry;

        Tsukino::EngineIntegration::EngineContext* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context || !context->renderer) {
            return;
        }

        ID3D11Device*        device = context->renderer->GetDevice();
        ID3D11DeviceContext* dc     = context->renderer->GetContext();
        if(!device || !dc) {
            return;
        }

        m_renderer = EffekseerRendererDX11::Renderer::Create(device, dc, 20000);
        if(!m_renderer) {
            return;
        }

        m_manager = Effekseer::Manager::Create(maxParticles);
        if(!m_manager) {
            m_renderer.Reset();
            return;
        }

        m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
        m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());

        m_effectFileInterfaceRef = Effekseer::MakeRefPtr<Tsukino::EngineIntegration::EffectFileInterface>();
        m_effectFileInterface    = m_effectFileInterfaceRef.Get();
        m_textureLoader          = EffekseerRenderer::CreateTextureLoader(m_renderer->GetGraphicsDevice(), m_effectFileInterfaceRef);
        m_manager->SetTextureLoader(m_textureLoader);

        m_entityDestroyedConn = eventBus.Subscribe<Tsukino::ECS::EngineEvent::EntityDestroyedEvent>(
            [this](const Tsukino::ECS::EngineEvent::EntityDestroyedEvent& event) { OnEffectEntityDestroyed(event); });

        //--------------------------------------------------------------
        // 破棄経路によらず確実に回収するため、EnTT の破棄シグナルにも購読する。
        // Finalize() で必ず解除すること（System は Registry より先に破棄されるため）。
        //--------------------------------------------------------------
        registry.OnDestroy<EffectComponent>().connect<&EffectSystem::OnEffectComponentDestroyed>(*this);

        m_initialized = true;
    }

    //--------------------------------------------------------------
    //! @brief  Effekseerを終了する
    //--------------------------------------------------------------
    void EffectSystem::Finalize() {
        if(!m_initialized) {
            return;
        }

        m_entityDestroyedConn.Disconnect();

        //--------------------------------------------------------------
        // Registry より先に破棄されるため、シグナル購読を必ず解除する。
        // 解除し忘れると破棄済みの this へコールバックが飛ぶ。
        //--------------------------------------------------------------
        if(m_registry) {
            m_registry->OnDestroy<EffectComponent>().disconnect(this);
        }

        StopAllEffects();
        m_loadedEffects.clear();
        m_manager.Reset();
        m_renderer.Reset();
        m_effectFileInterfaceRef.Reset();
        m_effectFileInterface = nullptr;
        m_initialized         = false;
        m_registry            = nullptr;
    }

    //--------------------------------------------------------------
    //! @brief  エフェクトを再生する
    //! @param  registry   [in] ECS レジストリ
    //! @param  asset      [in] 再生するエフェクトアセット
    //! @param  effectPath [in] エフェクトファイルのパス
    //! @param  position   [in] 再生位置 (x, y, z)
    //! @param  looping    [in] ループ再生するか
    //! @param  scale      [in] 再生スケール（等倍=1.0）
    //! @return エフェクトハンドル（負値の場合は失敗）
    //--------------------------------------------------------------
    int EffectSystem::PlayEffect(
        Tsukino::ECS::Registry& registry, Tsukino::Asset::AssetHandle asset, const Tsukino::Core::Path& effectPath, const float* position, bool looping, float scale) {
        if(!m_initialized || !m_manager) {
            return -1;
        }

        auto it = m_loadedEffects.find(asset);
        if(it == m_loadedEffects.end()) {
            Tsukino::EngineIntegration::EngineContext* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
            if(!context || !context->assetManager) {
                return -1;
            }

            Tsukino::Core::Ref<Tsukino::Asset::IAsset> baseAsset = context->assetManager->Get(asset);
            if(!baseAsset) {
                return -1;
            }

            Tsukino::Core::Ref<Tsukino::Asset::EffectAsset> effectAsset = std::dynamic_pointer_cast<Tsukino::Asset::EffectAsset>(baseAsset);
            if(!effectAsset || effectAsset->binary.empty()) {
                return -1;
            }

            m_effectFileInterface->SetBaseDirectory(effectPath.parent_path());

            Effekseer::EffectRef effect = Effekseer::Effect::Create(m_manager, effectAsset->binary.data(), static_cast<int32_t>(effectAsset->binary.size()));
            if(!effect) {
                return -1;
            }

            m_loadedEffects[asset] = effect;
            it                     = m_loadedEffects.find(asset);
        }

        if(it == m_loadedEffects.end() || !it->second) {
            return -1;
        }

        float x = position ? position[0] : 0.0f;
        float y = position ? position[1] : 0.0f;
        float z = position ? position[2] : 0.0f;

        ::Effekseer::Handle efkHandle = m_manager->Play(it->second, x, y, z);

        if(efkHandle == -1) {
            return -1;
        }

        // Effekseerではエフェクトの制作単位とワールドの単位系が食い違うことがあるため、
        // インスタンス単位で拡大率を与えられるようにする。Effect::Createのmagnification
        // （読み込み時に焼き込む）ではなくSetScaleを使うのは、m_loadedEffectsがアセット単位の
        // キャッシュのため、同じアセットを別スケールで再生すると最初の値が黙って使われてしまうため
        if(scale != 1.0f) {
            m_manager->SetScale(efkHandle, scale, scale, scale);
        }

        return efkHandle;
    }

    //--------------------------------------------------------------
    //! @brief  指定したハンドルのエフェクトを停止する
    //! @param  handle  [in] 停止するエフェクトハンドル
    //--------------------------------------------------------------
    void EffectSystem::StopEffect(int handle) {
        if(handle < 0 || !m_manager) {
            return;
        }

        m_manager->StopEffect(handle);
    }

    //--------------------------------------------------------------
    //! @brief  指定したハンドルのエフェクトを一時停止する
    //! @param  handle  [in] 一時停止するエフェクトハンドル
    //--------------------------------------------------------------
    void EffectSystem::PauseHandle(int handle) {
        if(handle < 0 || !m_manager) {
            return;
        }

        m_manager->SetPaused(handle, true);
    }

    //--------------------------------------------------------------
    //! @brief  指定したハンドルのエフェクトを再開する
    //! @param  handle  [in] 再開するエフェクトハンドル
    //--------------------------------------------------------------
    void EffectSystem::ResumeHandle(int handle) {
        if(handle < 0 || !m_manager) {
            return;
        }

        m_manager->SetPaused(handle, false);
    }

    //--------------------------------------------------------------
    //! @brief  指定したハンドルが再生中か取得する
    //! @param  handle  [in] 確認するエフェクトハンドル
    //! @return 再生中なら true
    //--------------------------------------------------------------
    bool EffectSystem::IsPlaying(int handle) const {
        if(handle < 0 || !m_manager) {
            return false;
        }

        return m_manager->Exists(handle);
    }

    //--------------------------------------------------------------
    //! @brief  指定したハンドルの再生速度を設定する
    //! @param  handle  [in] 対象エフェクトハンドル
    //! @param  speed   [in] 再生速度
    //--------------------------------------------------------------
    void EffectSystem::SetPlaySpeed(int handle, float speed) {
        if(handle < 0 || !m_manager) {
            return;
        }

        m_manager->SetSpeed(handle, speed);
    }

    //--------------------------------------------------------------
    //! @brief  指定したハンドルへトリガーを送信する
    //! @param  handle  [in] 対象エフェクトハンドル
    //! @param  index   [in] トリガーインデックス
    //-------------------------------------------------------------
    void EffectSystem::SendTrigger(int handle, int32_t index) {
        if(handle < 0 || !m_manager) {
            return;
        }

        m_manager->SendTrigger(handle, index);
    }

    //--------------------------------------------------------------
    //! @brief  全てのエフェクトを停止する
    //--------------------------------------------------------------
    void EffectSystem::StopAllEffects() {
        if(!m_manager) {
            return;
        }

        m_manager->StopAllEffects();
    }

    //--------------------------------------------------------------
    //! @brief  エンティティ破棄時のコールバック
    //! @param  event [in] エンティティ破棄イベント
    //--------------------------------------------------------------
    void EffectSystem::OnEffectEntityDestroyed(const Tsukino::ECS::EngineEvent::EntityDestroyedEvent& event) {
        if(!m_registry || !m_manager) {
            return;
        }

        if(m_registry->HasComponent<EffectComponent>(event.entity)) {
            auto& comp = m_registry->GetComponent<EffectComponent>(event.entity);
            if(comp.handle >= 0) {
                m_manager->StopEffect(comp.handle);
                comp.handle = -1;
            }
            comp.active  = false;
            comp.stopped = false;
        }
    }

    //--------------------------------------------------------------
    //! @brief  EffectComponent 破棄時に Effekseer のハンドルを停止する
    //--------------------------------------------------------------
    void EffectSystem::OnEffectComponentDestroyed(entt::registry& registry, entt::entity entity) {
        if(!m_manager) {
            return;
        }

        // EnTT は「取り外す直前」に呼ぶため、この時点ではまだ読める
        EffectComponent* comp = registry.try_get<EffectComponent>(entity);
        if(!comp) {
            return;
        }

        if(comp->handle >= 0) {
            m_manager->StopEffect(comp->handle);
            comp->handle = -1;
        }
        comp->active  = false;
        comp->stopped = false;
    }

    //--------------------------------------------------------------
    //! @brief  システムの更新（エフェクトの再生・停止制御）
    //! @param  registry    [in] ECS レジストリ
    //! @param  deltaTime   [in] 前フレームからの経過時間
    //--------------------------------------------------------------
    void EffectSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if(!m_initialized || !m_manager) {
            return;
        }

        const float targetFps = 60.0f;    // 基準とするFPS

        float deltaFrame = deltaTime * targetFps;

        // エフェクシアにはフレームを渡す
        m_manager->Update(deltaFrame);

        auto view = registry.View<EffectComponent>();
        for(Tsukino::ECS::Entity entity : view) {
            EffectComponent& comp = view.get<EffectComponent>(entity);

            if(comp.stopped) {
                if(comp.handle >= 0) {
                    StopEffect(comp.handle);
                    comp.handle = -1;
                }
                comp.active  = false;
                comp.stopped = false;
                continue;
            }

            if(comp.active && comp.handle < 0) {
                float pos[3] = {0.0f, 0.0f, 0.0f};
                if(registry.HasComponent<TransformComponent>(entity)) {
                    auto& tf = registry.GetComponent<TransformComponent>(entity);
                    pos[0]   = tf.position.x;
                    pos[1]   = tf.position.y;
                    pos[2]   = tf.position.z;
                }
                int newHandle = PlayEffect(registry, comp.effectAsset, comp.effectPath, pos, comp.looping);
                if(newHandle >= 0) {
                    comp.handle = newHandle;
                } else {
                    comp.active = false;
                }
            } else if(comp.active && comp.handle >= 0) {
                if(!comp.looping && !m_manager->Exists(comp.handle)) {
                    comp.active = false;
                    comp.handle = -1;
                    continue;
                }

                if(registry.HasComponent<TransformComponent>(entity)) {
                    auto& tf = registry.GetComponent<TransformComponent>(entity);
                    m_manager->SetLocation(comp.handle, tf.position.x, tf.position.y, tf.position.z);
                }

                if(comp.playSpeed != 1.0f) {
                    m_manager->SetSpeed(comp.handle, comp.playSpeed);
                }
            }
        }
    }

    //--------------------------------------------------------------
    //! @brief  エフェクトの描画（D3D11デバイスコンテキストでEffekseerを実行）
    //! @param  dc  [in] D3D11 デバイスコンテキスト
    //--------------------------------------------------------------
    void EffectSystem::RenderEffects(ID3D11DeviceContext* dc, const Tsukino::Core::Math::matrix& view, const Tsukino::Core::Math::matrix& projection) {
        if(!m_initialized || !m_renderer || !dc) {
            return;
        }

        ::Effekseer::Matrix44 efkView{};
        ::Effekseer::Matrix44 efkProj{};
        std::memcpy(efkView.Values, &view, sizeof(Tsukino::Core::Math::matrix));
        std::memcpy(efkProj.Values, &projection, sizeof(Tsukino::Core::Math::matrix));

        m_renderer->SetCameraMatrix(efkView);
        m_renderer->SetProjectionMatrix(efkProj);

        m_renderer->BeginRendering();
        m_manager->Draw();
        m_renderer->EndRendering();
    }

}    // namespace Tsukino::BuiltIn::ECS
