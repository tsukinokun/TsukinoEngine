//------------------------------------------------------------
//! @file       EngineIntegration.cpp
//! @brief      エンジン全体で共有されるクラスを統合したクラスの実装
//! @author     山﨑愛
//------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineIntegration.hpp>

#include <Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp>
#include <Tsukino/EngineIntegration/Scene/GameSceneBase.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/EffectComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DirectionalLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/PointLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpotLightComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SkyAtmosphereComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/MotionBlurComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DebugCameraTag.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TerrainGenerationRequestComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/DraggableComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Serialization/CameraComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/TransformComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/RigidbodyComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/EffectComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/DirectionalLightComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/PointLightComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/SpotLightComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/SkyAtmosphereComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/FogComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/AmbientParticleComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/MotionBlurComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/DebugCameraComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/ModelComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/CollisionComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/TerrainGenerationRequestComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/SpriteComponentSerialization.hpp>
#include <Tsukino/BuiltIn/ECS/Serialization/FontComponentSerialization.hpp>

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
    //! @note  デストラクタは「本体 → メンバの破棄」の順に走る。本体で
    //!        CoUninitialize() を呼ぶだけでは、まだ生きている Renderer /
    //!        AudioManager より先に COM を落とすことになる（以前のコメントは
    //!        破棄順を逆に書いていた）。
    //!        そのため宣言の逆順＝暗黙の破棄順と同じ並びで明示的に解放し、
    //!        COM 利用者が全ていなくなってから CoUninitialize() する。
    //------------------------------------------------------------
    EngineIntegration::~EngineIntegration() {
        //------------------------------------------------------------
        // 【必ず最初に行う】ウィンドウへ登録したコールバックを外す。
        //
        // これらのコールバックは this を捕らえ、m_inputSystem / m_renderer を
        // 直接参照する。一方 m_window は破棄順の都合で最後に解放されるため、
        // ~Window() の DestroyWindow() が WindowProc を再入させる時点では、
        // それらのメンバは既に解放済みになっている。
        //
        // 実際 DestroyWindow() が送る WM_ACTIVATEAPP / WM_KILLFOCUS が
        // m_focusLostCallback →（解放済みの）m_inputSystem->ClearAllKeys() を呼び、
        // 終了時に STATUS_FATAL_USER_CALLBACK_EXCEPTION (0xC000041D) で
        // 落ちていた。ウィンドウがアクティブなまま終了したときだけ
        // WM_ACTIVATEAPP / WM_KILLFOCUS が飛ぶため、再現が非決定的だった。
        //
        // Window 側でも同じ理由でデストラクタ先頭に通知先の解除を入れてあるが、
        // 「登録した側が責任を持って外す」形にしておかないと、
        // メンバ構成を変えた瞬間に同じ問題が再発する。
        //------------------------------------------------------------
        if(m_window) {
            m_window->SetMessageCallback(nullptr);
            m_window->SetResizeCallback(nullptr);
            m_window->SetFocusLostCallback(nullptr);
        }

        //------------------------------------------------------------
        // 宣言の逆順で明示的に解放する（暗黙の破棄順とまったく同じ並び。
        // CoUninitialize() をこの後に持ってくるためだけに明示している）
        //------------------------------------------------------------
        m_gameSceneManager.reset();
        m_prefabFactory.reset();
        m_audioManager.reset();
        m_inputSystem.reset();
        m_builtinAssets.reset();
        m_assetManager.reset();
        m_renderer.reset();
        m_window.reset();

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
        // クリックスルーの判定コールバックの設定
        //
        // ClickThroughウィンドウはOSレベルでは常にヒットテストを無視して
        // 背面へクリックを通すため、ドラッグ可能なスプライトにカーソルが
        // 重なった瞬間（＝実際のWM_LBUTTONDOWNが届く前）にWS_EX_TRANSPARENTを
        // 解除しておかないと、そのクリックが背面ウィンドウへ抜けてしまう。
        //--------------------------------------------------------------
        m_window->SetHitTestCallback([this](int x, int y) {
            GameSceneBase* scene = m_gameSceneManager->GetCurrentScene();
            if(!scene)
                return false;
            return Tsukino::BuiltIn::ECS::InteractionSystem::HitTest(
                scene->GetScene().GetRegistry(), static_cast<float>(x), static_cast<float>(y));
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
        auto motionBlurPSAsset     = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.motionBlurPS));
        auto fogPSAsset            = std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.fogPS));
        auto ambientParticleVSAsset =
            std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.ambientParticleVS));
        auto ambientParticlePSAsset =
            std::static_pointer_cast<Tsukino::Asset::ShaderAsset>(m_assetManager->Get(m_builtinAssets->shaders.ambientParticlePS));

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
        shaderSet.motionBlurPS     = motionBlurPSAsset.get();
        shaderSet.fogPS            = fogPSAsset.get();
        shaderSet.ambientParticleVS = ambientParticleVSAsset.get();
        shaderSet.ambientParticlePS = ambientParticlePSAsset.get();

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
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::PointLightComponent>("PointLightComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::SpotLightComponent>("SpotLightComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::SkyAtmosphereComponent>("SkyAtmosphereComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::MotionBlurComponent>("MotionBlurComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::FogComponent>("FogComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::AmbientParticleComponent>("AmbientParticleComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::DebugCameraComponent>("DebugCameraComponent");
        // タグのみのコンポーネントはシリアライズ対応不要（アタッチのみでよい）
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::DebugCameraTag>("DebugCameraTag");
        // text（std::wstring）とfontHandle（AssetHandle、プロセス内限定でシリアライズ不可）は
        // シリアライズ対象外。それ以外の見た目パラメータ（色・揃え位置など）は対応済み。
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::FontComponent>("FontComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::ModelComponent>("ModelComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::CollisionComponent>("CollisionComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::TerrainGenerationRequestComponent>("TerrainGenerationRequestComponent");
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::SpriteComponent>("SpriteComponent");
        // isDragging/dragOffsetは実行時状態のみのため、シリアライズ対応不要（アタッチのみでよい）
        m_prefabFactory->RegisterComponent<Tsukino::BuiltIn::ECS::DraggableComponent>("DraggableComponent");
    }
}    // namespace Tsukino::EngineIntegration
