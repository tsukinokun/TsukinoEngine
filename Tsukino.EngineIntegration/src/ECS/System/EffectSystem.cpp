//--------------------------------------------------------------
//! @file   EffectSystem.cpp
//! @brief  エフェクト再生システムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/ECS/System/EffectSystem.hpp>
#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Effect/EffectAsset.hpp>
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>

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
    void EffectSystem::Initialize(Tsukino::ECS::Registry& registry, int maxParticles) {
        if (m_initialized) {
            return;
        }

        Tsukino::EngineIntegration::EngineContext* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if (!context || !context->renderer) {
            return;
        }

        ID3D11Device* device = context->renderer->GetDevice();
        ID3D11DeviceContext* dc = context->renderer->GetContext();
        if (!device || !dc) {
            return;
        }

        m_renderer = EffekseerRendererDX11::Renderer::Create(device, dc, 20000);
        if (!m_renderer) {
            return;
        }

        m_manager = Effekseer::Manager::Create(maxParticles);
        if (!m_manager) {
            m_renderer.Reset();
            return;
        }

        m_manager->SetSpriteRenderer(m_renderer->CreateSpriteRenderer());
        m_manager->SetModelRenderer(m_renderer->CreateModelRenderer());

        m_initialized = true;
    }

    //--------------------------------------------------------------
    //! @brief  Effekseerを終了する
    //--------------------------------------------------------------
    void EffectSystem::Finalize() {
        if (!m_initialized) {
            return;
        }

        StopAllEffects();
        m_loadedEffects.clear();
        m_manager.Reset();
        m_renderer.Reset();
        m_initialized = false;
    }

    //--------------------------------------------------------------
    //! @brief  エフェクトを再生する
    //! @param  registry   [in] ECS レジストリ
    //! @param  asset      [in] 再生するエフェクトアセット
    //! @param  position   [in] 再生位置 (x, y, z)
    //! @return エフェクトハンドル（負値の場合は失敗）
    //--------------------------------------------------------------
    int EffectSystem::PlayEffect(Tsukino::ECS::Registry& registry,
                                  Tsukino::Asset::AssetHandle asset,
                                  const float* position) {
        if (!m_initialized || !m_manager) {
            return -1;
        }

        auto it = m_loadedEffects.find(asset);
        if (it == m_loadedEffects.end()) {
            Tsukino::EngineIntegration::EngineContext* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
            if (!context || !context->assetManager) {
                return -1;
            }

            Tsukino::Core::Ref<Tsukino::Asset::IAsset> baseAsset = context->assetManager->Get(asset);
            if (!baseAsset) {
                return -1;
            }

            Tsukino::Core::Ref<Tsukino::Asset::EffectAsset> effectAsset = std::dynamic_pointer_cast<Tsukino::Asset::EffectAsset>(baseAsset);
            if (!effectAsset || effectAsset->binary.empty()) {
                return -1;
            }

            Effekseer::EffectRef effect = Effekseer::Effect::Create(m_manager, effectAsset->binary.data(), static_cast<int32_t>(effectAsset->binary.size()));
            if (!effect) {
                return -1;
            }

            m_loadedEffects[asset] = effect;
            it = m_loadedEffects.find(asset);
        }

        if (!it->second) {
            return -1;
        }

        float x = position ? position[0] : 0.0f;
        float y = position ? position[1] : 0.0f;
        float z = position ? position[2] : 0.0f;

        ::Effekseer::Handle efkHandle = m_manager->Play(it->second, x, y, z);

        if (efkHandle == -1) {
            return -1;
        }

        return efkHandle;
    }

    //--------------------------------------------------------------
    //! @brief  指定したハンドルのエフェクトを停止する
    //! @param  handle  [in] 停止するエフェクトハンドル
    //--------------------------------------------------------------
    void EffectSystem::StopEffect(int handle) {
        if (handle < 0 || !m_manager) {
            return;
        }

        m_manager->StopEffect(handle);
    }

    //--------------------------------------------------------------
    //! @brief  全てのエフェクトを停止する
    //--------------------------------------------------------------
    void EffectSystem::StopAllEffects() {
        if (!m_manager) {
            return;
        }

        m_manager->StopAllEffects();
    }

    //--------------------------------------------------------------
    //! @brief  エンティティ破棄時のコールバック
    //! @param  registry [in] ECS レジストリ
    //! @param  entity   [in] 破棄されたエンティティ
    //! @param  comp     [in] 破棄されたEffectComponent
    //--------------------------------------------------------------
    void EffectSystem::OnEffectEntityDestroyed(Tsukino::ECS::Registry& registry,
                                                Tsukino::ECS::Entity entity,
                                                const EffectComponent& comp) {
        if (comp.handle >= 0 && m_manager) {
            m_manager->StopEffect(comp.handle);
        }
    }

    //--------------------------------------------------------------
    //! @brief  システムの更新（エフェクトの再生・停止制御）
    //! @param  registry    [in] ECS レジストリ
    //! @param  deltaTime   [in] 前フレームからの経過時間
    //--------------------------------------------------------------
    void EffectSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        if (!m_initialized || !m_manager) {
            return;
        }

        m_manager->Update(deltaTime);

        auto view = registry.View<EffectComponent>();
        for (Tsukino::ECS::Entity entity : view) {
            EffectComponent& comp = view.get<EffectComponent>(entity);

            if (comp.stopped) {
                if (comp.handle >= 0) {
                    StopEffect(comp.handle);
                    comp.handle = -1;
                }
                comp.active = false;
                continue;
            }

            if (comp.active && comp.handle < 0) {
                float pos[3] = {0.0f, 0.0f, 0.0f};
                int newHandle = PlayEffect(registry, comp.effectAsset, pos);
                if (newHandle >= 0) {
                    comp.handle = newHandle;
                } else {
                    comp.active = false;
                }
            }
        }
    }

    //--------------------------------------------------------------
    //! @brief  エフェクトの描画（D3D11デバイスコンテキストでEffekseerを実行）
    //! @param  dc  [in] D3D11 デバイスコンテキスト
    //--------------------------------------------------------------
    void EffectSystem::RenderEffects(ID3D11DeviceContext* dc) {
        if (!m_initialized || !m_renderer || !dc) {
            return;
        }

        m_renderer->BeginRendering();
        m_manager->Draw();
        m_renderer->EndRendering();
    }

}    // namespace Tsukino::BuiltIn::ECS
