//--------------------------------------------------------------
//! @file	WinMain.cpp
//! @brief	Sandboxのエントリポイント
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/EngineIntegration/EngineAPI.hpp>
#include <Tsukino/EngineIntegration/EngineIntegration.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>

// 必要なシステムとコンポーネントのインクルード
#include <Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp>
#include <Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/SpriteComponent.hpp>

#include <Windows.h>
#include <entt/entt.hpp>
#include <hlsl++.h>

//--------------------------------------------------------------
// アプリケーションのエントリポイント
//! @param hInstance アプリケーションインスタンス
//! @param hPrevInstance 非推奨（常にNULL）
//! @param lpCmdLine コマンドライン引数
//! @param nCmdShow ウィンドウ表示状態（例：SW_SHOW）
//! @return 終了コード（通常は0）
//--------------------------------------------------------------
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    // ログの初期化
    Tsukino::EngineIntegration::EngineIntegration engineIntegration;
    // 初期化
    if(!engineIntegration.Initialize()) {
        // 初期化に失敗した場合はエラーログを出力して終了
        Tsukino::Core::Log::Error("Failed to initialize EngineIntegration.");
        return false;
    }

    Tsukino::EngineIntegration::EngineContext& engineContext = engineIntegration.GetContext();
    Tsukino::ECS::Registry&                    registry      = engineIntegration.GetRegistry();
    Tsukino::EngineIntegration::EngineAPI      engineAPI(engineContext, registry);

    //--------------------------------------------------------------
    // システムの生成
    //--------------------------------------------------------------
    //--------------------------------------------------------------
    // Transformは一番最初に計算する (優先度 0)
    engineAPI.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::TransformSystem>(), 0);
    // スプライトなど描画用のコマンド生成は後で行う (優先度 10)
    engineAPI.AddSystem(std::make_shared<Tsukino::BuiltIn::ECS::SpriteRenderSystem>(), 10);

    //--------------------------------------------------------------
    // アセットのロードとエンティティの作成
    //--------------------------------------------------------------
    Tsukino::Asset::AssetHandle textureHandle = engineContext.assets->Load(Tsukino::Core::Path("Assets/Textures/test.jpg"));

    // エンティティ生成
    Tsukino::ECS::Entity entity = registry.CreateEntity();

    // TransformComponent の追加と初期化
    Tsukino::BuiltIn::ECS::TransformComponent& transform = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(entity);
    transform.position                                   = hlslpp::float3(0.0f, 0.0f, 0.0f);
    transform.rotation                                   = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);    // 無回転
    transform.scale                                      = hlslpp::float3(1.0f, 1.0f, 1.0f);
    transform.dirty                                      = true;          // 初回計算のためフラグを立てる
    transform.parent                                     = entt::null;    // 親なし

    // SpriteComponent の追加
    Tsukino::BuiltIn::ECS::SpriteComponent& sprite = registry.AddComponent<Tsukino::BuiltIn::ECS::SpriteComponent>(entity);
    sprite.textureHandle                           = textureHandle;
    sprite.tintColor                               = hlslpp::float4(1.0f, 1.0f, 1.0f, 1.0f);    // 白色
    sprite.uvRect                                  = hlslpp::float4(0.0f, 0.0f, 1.0f, 1.0f);

    //--------------------------------------------------------------
    // メインループ
    //--------------------------------------------------------------
    // テスト用の固定デルタタイム
    const float deltaTime = 1.0f / 60.0f;

    //--------------------------------------------------------------
    // メインループ
    //--------------------------------------------------------------
    while(engineAPI.ProcessMessages()) {
        // 登録されたすべてのECSシステムを一括更新
        engineAPI.Update(deltaTime);
        // 描画処理
        engineAPI.Render();
    }

    //--------------------------------------------------------------
    // ウィンドウは自動的に破棄される
    //--------------------------------------------------------------
    return 0;
}
