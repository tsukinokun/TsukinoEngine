//------------------------------------------------------------
//! @file       EngineIntegration.cpp
//! @brief      エンジン全体で共有されるクラスを統合したクラスの実装
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineIntegration.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Serialization/CameraComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/TransformComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/RigidbodyComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/EffectComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/DirectionalLightComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/SkyAtmosphereComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/DebugCameraComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/ModelComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/CollisionComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/TerrainGenerationRequestComponentSerialization.hpp>

#include <Tsukino/Core/Log.hpp>

#include <memory>

// 名前空間 : Tsukino::Integration
namespace Tsukino::EngineIntegration {
    //------------------------------------------------------------
    //! @brief コンストラクタ
    //------------------------------------------------------------
    EngineIntegration::EngineIntegration() {
        //------------------------------------------------------------
        // ユニークポインタを作成して
        //------------------------------------------------------------
        m_window           = std::make_unique<Tsukino::Core::Window>();
        m_inputSystem      = std::make_unique<Tsukino::Input::InputSystem>();
        m_renderer         = std::make_unique<Tsukino::Renderer::Renderer>();
        m_assetManager     = std::make_unique<Tsukino::Asset::AssetManager>();
        m_builtinAssets    = std::make_unique<Tsukino::BuiltIn::BuiltInAssets>();
        m_gameSceneManager = std::make_unique<GameSceneManager>();
        m_audioManager     = std::make_unique<Tsukino::Audio::AudioManager>();
        m_prefabFactory    = std::make_unique<Tsukino::Engine::ECS::Prefab::PrefabFactory>();

        //------------------------------------------------------------
        // コンテキストにポインタをセット
        //------------------------------------------------------------
        m_ctx.window           = m_window.get();
        m_ctx.renderer         = m_renderer.get();
        m_ctx.assetManager     = m_assetManager.get();
        m_ctx.builtinAssets    = m_builtinAssets.get();
        m_ctx.gameSceneManager = m_gameSceneManager.get();
        m_ctx.inputSystem      = m_inputSystem.get();
        m_ctx.audioManager     = m_audioManager.get();
        m_ctx.prefabFactory    = m_prefabFactory.get();

        //------------------------------------------------------------
        // PrefabFactoryがAssetRefを解決できるようAssetManagerを渡しておく
        //------------------------------------------------------------
        m_prefabFactory->SetAssetManager(m_assetManager.get());
    }

    //------------------------------------------------------------
    //! @brief デストラクタ
    //! @note  メンバの破棄はこの本体が走る前に行われるため、
    //!        CoUninitialize() は全てのCOM利用者（Renderer, AudioManager等）が
    //!        解放された後に呼ばれることが保証される。
    //------------------------------------------------------------
    EngineIntegration::~EngineIntegration() {
        if(m_comInitialized) {
            CoUninitialize();
            m_comInitialized = false;
        }
    }

    //------------------------------------------------------------
    //! @brief  エンジンの初期化関数
    //------------------------------------------------------------
    bool EngineIntegration::Initialize(int width, int height, const std::string& title, Tsukino::Core::Window::WindowStyle style) {
        //--------------------------------------------------------------
        // COM初期化
        //--------------------------------------------------------------
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        // COMの初期化に失敗した場合はエラーログを出力して終了
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to initialize COM library.");
            return false;
        }
        // 成功した場合のみ、デストラクタで対になる CoUninitialize を呼ぶ
        m_comInitialized = true;

        //--------------------------------------------------------------
        // BuiltInのCopmonentをPrefabFactoryに登録
        //--------------------------------------------------------------
        RegisterBuiltInComponents();

        //--------------------------------------------------------------
        // AssetManager 初期化
        //--------------------------------------------------------------
        m_assetManager->Initialize();

        //--------------------------------------------------------------
        // ビルトインアセットの初期化
        //--------------------------------------------------------------
        m_builtinAssets->Initialize(m_assetManager.get());

        //--------------------------------------------------------------
        // コンテキストを渡してGameSceneManagerを初期化
        //--------------------------------------------------------------
        m_gameSceneManager->Initialize(&m_ctx);

        //--------------------------------------------------------------
        // ウィンドウ生成
        //--------------------------------------------------------------
        if(!m_window->Create(title, width, height, style)) {
            return false;
        }

        //--------------------------------------------------------------
        // オーディオマネージャの初期化
        //--------------------------------------------------------------
        if(!m_audioManager->Initialize()) {
            Tsukino::Core::Log::Error("Failed to initialize AudioManager.");
            return false;
        }

        //--------------------------------------------------------------
        // メッセージコールバックの設定
        //--------------------------------------------------------------
        m_window->SetMessageCallback([this](UINT msg, WPARAM wp, LPARAM lp) {
            switch(msg) {
                // --- マウスボタン ---
            case WM_LBUTTONDOWN:
                m_inputSystem->SetKeyState(Input::KeyCode::LButton, true);
                break;
            case WM_LBUTTONUP:
                m_inputSystem->SetKeyState(Input::KeyCode::LButton, false);
                break;
            case WM_RBUTTONDOWN:
                m_inputSystem->SetKeyState(Input::KeyCode::RButton, true);
                break;
            case WM_RBUTTONUP:
                m_inputSystem->SetKeyState(Input::KeyCode::RButton, false);
                break;
            case WM_MBUTTONDOWN:
                m_inputSystem->SetKeyState(Input::KeyCode::MButton, true);
                break;
            case WM_MBUTTONUP:
                m_inputSystem->SetKeyState(Input::KeyCode::MButton, false);
                break;
            // --- キーボード入力 ---
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:    // Altキーなどを含める場合
                m_inputSystem->SetKeyState(static_cast<Input::KeyCode>(wp), true);
                break;

            case WM_KEYUP:
            case WM_SYSKEYUP:
                m_inputSystem->SetKeyState(static_cast<Input::KeyCode>(wp), false);
                break;

            // --- マウス座標 ---
            case WM_MOUSEMOVE:
                // LOWORD, HIWORD で X, Y 座標を取り出す
                m_inputSystem->SetMousePosition(static_cast<i32>(LOWORD(lp)), static_cast<i32>(HIWORD(lp)));
                break;

            // --- マウスホイール ---
            case WM_MOUSEWHEEL:
                // ホイールの回転量を取り出して加算（120単位で届くので割る）
                float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / static_cast<float>(WHEEL_DELTA);
                m_inputSystem->AddWheelDelta(delta);
                break;
            }
        });

        //--------------------------------------------------------------
        // フォーカス喪失時に入力状態をクリアする
        // （キーを押したまま Alt+Tab したときの押しっぱなし対策）
        //--------------------------------------------------------------
        m_window->SetFocusLostCallback([this]() { m_inputSystem->ClearAllKeys(); });

        //--------------------------------------------------------------
        // ウィンドウのリサイズをスワップチェインへ伝える
        //
        // Renderer より先に登録しても、実際に呼ばれるのは
        // メッセージループが回り始めてからなので問題ない。
        //--------------------------------------------------------------
        m_window->SetResizeCallback([this](int newWidth, int newHeight) {
            if(m_renderer) {
                m_renderer->Resize(static_cast<uint32_t>(newWidth), static_cast<uint32_t>(newHeight));
            }
        });

        auto debugVsAsset   = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.debugVS));
        auto debugPsAsset   = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.debugPS));
        auto tonemapVSAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.tonemapVS));
        auto tonemapPSAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.tonemapPS));
        auto shadowStaticVSAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.shadowStaticVS));
        auto shadowSkeletalVSAsset = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.shadowVS));
        auto shadowPSAsset         = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.shadowPS));
        auto lightingPSAsset       = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.lightingPS));

        //--------------------------------------------------------------
        // レンダラー生成
        //--------------------------------------------------------------
        Tsukino::Renderer::RendererShaderSet shaderSet{};
        shaderSet.debugVS          = debugVsAsset.get();
        shaderSet.debugPS          = debugPsAsset.get();
        shaderSet.tonemapVS        = tonemapVSAsset.get();
        shaderSet.tonemapPS        = tonemapPSAsset.get();
        shaderSet.shadowStaticVS   = shadowStaticVSAsset.get();
        shaderSet.shadowSkeletalVS = shadowSkeletalVSAsset.get();
        shaderSet.shadowPS         = shadowPSAsset.get();
        shaderSet.lightingPS       = lightingPSAsset.get();

        if(!m_renderer->Initialize(m_window->GetHWND(), m_window->GetWidth(), m_window->GetHeight(), shaderSet)) {
            return false;
        }

        return true;
    }

    //------------------------------------------------------------
    //! @brief  エンジン標準の組み込みコンポーネントを工場に自動登録する
    //------------------------------------------------------------
    void EngineIntegration::RegisterBuiltInComponents() {
        // 標準のトランスフォームとカメラを登録
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::TransformComponent>("TransformComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::CameraComponent>("CameraComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::RigidbodyComponent>("RigidbodyComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::EffectComponent>("EffectComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::DirectionalLightComponent>("DirectionalLightComponent");
        // PointLight/SpotLightはDirectionalLightと異なりシリアライザ未実装（デフォルト値でのアタッチのみ対応）
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::PointLightComponent>("PointLightComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::SpotLightComponent>("SpotLightComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::SkyAtmosphereComponent>("SkyAtmosphereComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::DebugCameraComponent>("DebugCameraComponent");
        // タグのみのコンポーネントはシリアライズ対応不要（アタッチのみでよい）
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::DebugCameraTag>("DebugCameraTag");
        // fontHandleがAssetHandle（プロセス内限定でシリアライズ不可）を持つため、
        // 現状シリアライザは未実装。デフォルト値でのアタッチのみ対応する。
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::FontComponent>("FontComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::ModelComponent>("ModelComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::CollisionComponent>("CollisionComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::TerrainGenerationRequestComponent>("TerrainGenerationRequestComponent");
    }
}    // namespace Tsukino::EngineIntegration
