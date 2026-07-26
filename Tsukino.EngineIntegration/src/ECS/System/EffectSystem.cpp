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
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>
#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/Renderer/Renderer.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Engine/Asset/AssetHandle.hpp>
#include <Tsukino/Engine/ECS/EngineEvent/EntityEvent.hpp>
#include <Effekseer.h>
#include <EffekseerRendererDX11.h>
#include <EffekseerRendererCommon/EffekseerRendererCommon/TextureLoader.h>
#include <filesystem>

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

        m_textureLoader = EffekseerRenderer::CreateTextureLoader(
            m_renderer->GetGraphicsDevice(), 
            Effekseer::MakeRefPtr<Tsukino::EngineIntegration::EffectFileInterface>()
        );
        m_manager->SetTextureLoader(m_textureLoader);

        m_entityDestroyedConn            = eventBus.Subscribe<Tsukino::ECS::EngineEvent::EntityDestroyedEvent>(
            [this](const Tsukino::ECS::EngineEvent::EntityDestroyedEvent& event) { OnEffectEntityDestroyed(event); });

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
        StopAllEffects();
        m_loadedEffects.clear();
        m_manager.Reset();
        m_renderer.Reset();
        m_initialized = false;
        m_registry    = nullptr;
    }

    //--------------------------------------------------------------
    //! @brief  エフェクトを再生する
    //! @param  registry   [in] ECS レジストリ
    //! @param  asset      [in] 再生するエフェクトアセット
    //! @param  position   [in] 再生位置 (x, y, z)
    //! @param  looping    [in] ループ再生するか
    //! @return エフェクトハンドル（負値の場合は失敗）
    //--------------------------------------------------------------
    int EffectSystem::PlayEffect(Tsukino::ECS::Registry& registry, Tsukino::Asset::AssetHandle asset, const float* position, bool looping) {
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
    //--------------------------------------------------------------
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
                int newHandle = PlayEffect(registry, comp.effectAsset, pos, comp.looping);
                if(newHandle >= 0) {
                    comp.handle = newHandle;
                    
                    if(!comp.textureBindings.empty()) {
                        Effekseer::EffectRef effect = m_manager->GetEffect(newHandle);
                        if(effect) {
                            for(const auto& bindingInfo : comp.textureBindings) {
                                ApplyTextureBinding(effect, bindingInfo.texturePath, bindingInfo.layer);
                            }
                        }
                    }
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
    void EffectSystem::RenderEffects(ID3D11DeviceContext* dc,
                                      const Tsukino::Core::Math::matrix& view,
                                      const Tsukino::Core::Math::matrix& projection) {
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

    //--------------------------------------------------------------
    //! @brief  エフェクトテクスチャを設定する
    //--------------------------------------------------------------
    void EffectSystem::SetEffectTexture(int handle, int layer, const Tsukino::Core::Path& texturePath) {
        if(handle < 0 || !m_manager || !m_renderer || !m_textureLoader) {
            return;
        }

        if(layer < 0 || layer >= Effekseer::Manager::LayerCount) {
            return;
        }

        Effekseer::EffectRef effect = m_manager->GetEffect(handle);
        if(!effect) {
            return;
        }

        auto texturePathStr = texturePath.string();
        std::vector<char16_t> path16(texturePathStr.size() * 2 + 1, 0);
        Effekseer::ConvertUtf8ToUtf16(path16.data(), static_cast<int32_t>(path16.size()), texturePathStr.c_str());
        
        auto texture = m_textureLoader->Load(path16.data(), Effekseer::TextureType::Color);
        if(!texture) {
            texture = m_textureLoader->Load(path16.data(), Effekseer::TextureType::Normal);
        }
        if(!texture) {
            texture = m_textureLoader->Load(path16.data(), Effekseer::TextureType::Distortion);
        }

        if(texture) {
            effect->SetTexture(layer, Effekseer::TextureType::Color, texture);
            m_textureBindings[handle] = { layer, texturePath, Tsukino::Asset::AssetHandle() };
        }
    }

    //--------------------------------------------------------------
    //! @brief  エフェクトテクスチャをロードして設定する
    //--------------------------------------------------------------
    void EffectSystem::LoadAndSetEffectTexture(int handle, int layer, const Tsukino::Core::Path& texturePath) {
        if(!m_initialized || !m_manager) {
            return;
        }

        Tsukino::EngineIntegration::EngineContext* context = m_registry ? 
            m_registry->GetContext<Tsukino::EngineIntegration::EngineContext*>() : nullptr;
        if(!context || !context->assetManager) {
            return;
        }

        auto textureAsset = context->assetManager->Load(texturePath);
        if(!textureAsset.IsValid()) {
            return;
        }

        SetEffectTexture(handle, layer, texturePath);
        m_textureBindings[handle].textureAsset = textureAsset;
    }

    //--------------------------------------------------------------
    //! @brief  エフェクトテクスチャを元に戻す
    //--------------------------------------------------------------
    void EffectSystem::ResetEffectTexture(int handle, int layer) {
        if(handle < 0 || !m_manager) {
            return;
        }

        auto it = m_textureBindings.find(handle);
        if(it == m_textureBindings.end()) {
            return;
        }

        Effekseer::EffectRef effect = m_manager->GetEffect(handle);
        if(!effect) {
            return;
        }

        effect->SetTexture(layer, Effekseer::TextureType::Color, nullptr);
        m_textureBindings.erase(it);
    }

    //--------------------------------------------------------------
    //! @brief  エフェクトのテクスチャバインドを適用する
    //--------------------------------------------------------------
    void EffectSystem::ApplyTextureBinding(Effekseer::EffectRef effect, const Tsukino::Core::Path& texturePath, int layer) {
        if(!effect || layer < 0 || layer >= Effekseer::Manager::LayerCount) {
            return;
        }

        if(!m_renderer || !m_textureLoader) {
            Tsukino::Core::Log::Error("[EffectSystem] renderer or textureLoader is null");
            return;
        }

        auto texturePathStr = texturePath.string();
        Tsukino::Core::Log::Info("[EffectSystem] Loading texture from: " + texturePathStr);
        
        auto textureData = ReadTextureFile(texturePathStr);
        if(!textureData.empty()) {
            auto texture = m_textureLoader->Load(textureData.data(), static_cast<int32_t>(textureData.size()), Effekseer::TextureType::Color, false);
            if(!texture) {
                texture = m_textureLoader->Load(textureData.data(), static_cast<int32_t>(textureData.size()), Effekseer::TextureType::Normal, false);
            }
            if(!texture) {
                texture = m_textureLoader->Load(textureData.data(), static_cast<int32_t>(textureData.size()), Effekseer::TextureType::Distortion, false);
            }

            if(texture) {
                Tsukino::Core::Log::Info("[EffectSystem] Texture loaded successfully, binding to layer " + std::to_string(layer));
                effect->SetTexture(layer, Effekseer::TextureType::Color, texture);
            } else {
                Tsukino::Core::Log::Error("[EffectSystem] Failed to load texture from: " + texturePathStr);
            }
        } else {
            Tsukino::Core::Log::Error("[EffectSystem] Failed to read texture file: " + texturePathStr);
        }
    }

    std::vector<uint8_t> EffectSystem::ReadTextureFile(const std::string& path) {
        std::vector<std::string> searchPaths = {
            "Assets/Effects/Texture/" + std::filesystem::path(path).filename().string(),
            "Assets/Texture/" + std::filesystem::path(path).filename().string(),
            "Texture/" + std::filesystem::path(path).filename().string(),
            "Assets/" + path,
            path
        };
        
        for (const auto& searchPath : searchPaths) {
            if (Tsukino::IO::FileSystem::Exists(Tsukino::Core::Path(searchPath))) {
                Tsukino::Core::Log::Info("[EffectSystem::ReadTextureFile] Found texture at: " + searchPath);
                return Tsukino::IO::FileSystem::ReadBinary(Tsukino::Core::Path(searchPath));
            }
        }
        
        Tsukino::Core::Log::Error("[EffectSystem::ReadTextureFile] Texture NOT found: " + path);
        return {};
    }

}    // namespace Tsukino::BuiltIn::ECS
