# Tsukino.EngineIntegration の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 主要な型

ゲーム側から実際に触る型。メンバを全て展開している。

### Tsukino::EngineIntegration::EngineAPI

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineAPI.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `EngineAPI(EngineContext &context)` | コンストラクタ |
| `void ChangeScene(std::unique_ptr< GameSceneBase > newScene)` | シーン遷移関数 |
| `bool ProcessMessages()` | メッセージ処理関数 |
| `void Update(float deltaTime)` |  |
| `void Render()` | 描画関数 |

### Tsukino::EngineIntegration::EngineContext

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineContext.hpp`

**公開メンバ**

| メンバ | 説明 |
|---|---|
| `Tsukino::Renderer::Renderer * renderer` |  |
| `Tsukino::Core::Window * window` |  |
| `Tsukino::Input::InputSystem * inputSystem` |  |
| `Tsukino::Asset::AssetManager * assetManager` |  |
| `Tsukino::BuiltIn::BuiltInAssets * builtinAssets` |  |
| `GameSceneManager * gameSceneManager` |  |
| `Tsukino::Audio::AudioManager * audioManager` |  |
| `Tsukino::Engine::ECS::Prefab::PrefabFactory * prefabFactory` |  |
| `Tsukino::BuiltIn::ECS::EffectSystem * effectSystem` |  |
| `Tsukino::BuiltIn::ECS::PhysicsSystem * physicsSystem` |  |

### Tsukino::EngineIntegration::EngineIntegration

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineIntegration.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `EngineIntegration()` | コンストラクタ |
| `~EngineIntegration()` | デストラクタ |
| `EngineIntegration(const EngineIntegration &)=delete` |  |
| `EngineIntegration & operator=(const EngineIntegration &)=delete` |  |
| `EngineIntegration(EngineIntegration &&)=delete` |  |
| `EngineIntegration & operator=(EngineIntegration &&)=delete` |  |
| `bool Initialize(int width, int height, const std::string &title="TsukinoEngine", Tsukino::Core::Window::WindowStyle style=Tsukino::Core::Window::WindowStyle::Default)` | エンジンの初期化関数 |
| `EngineContext & GetContext()` | エンジン全体で共有されるクラスを取得する関数 |

### Tsukino::EngineIntegration::GameSceneBase

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/Scene/GameSceneBase.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `~GameSceneBase()=default` | デフォルトコンストラクタ |
| `void Initialize(Tsukino::EngineIntegration::EngineAPI &api, Tsukino::EngineIntegration::EngineContext *context)` | シーンの初期化インターフェース（外部から呼ばれる非仮想関数） |
| `void OnUpdate(Tsukino::EngineIntegration::EngineAPI &api, float deltaTime)=0` | シーンの更新インターフェース |
| `void OnExit()=0` | シーンの終了インターフェース |
| `Tsukino::ECS::Scene & GetScene()` | シーンへのアクセス |

### Tsukino::EngineIntegration::GameSceneManager

`Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/Scene/GameSceneManager.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `GameSceneManager()` | コンストラクタ |
| `~GameSceneManager()` | デストラクタ |
| `GameSceneManager(const GameSceneManager &)=delete` |  |
| `GameSceneManager & operator=(const GameSceneManager &)=delete` |  |
| `GameSceneManager(GameSceneManager &&)=delete` |  |
| `GameSceneManager & operator=(GameSceneManager &&)=delete` |  |
| `void Initialize(EngineContext *context)` | 初期化 |
| `void ChangeScene(std::unique_ptr< GameSceneBase > newScene)` | 次のシーンへの遷移を予約する |
| `void Update(Tsukino::EngineIntegration::EngineAPI &api, float deltaTime)` | シーンの更新 |
| `GameSceneBase * GetCurrentScene() const` | 現在アクティブなシーンを取得する |

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::BuiltIn::ECS::AnimationSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::AnimationSystem::ChannelTableKey** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp`
  - animation, skeleton, operator==()
- **Tsukino::BuiltIn::ECS::AnimationSystem::ChannelTableKeyHash** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/AnimationSystem.hpp`
  - operator()()
- **Tsukino::BuiltIn::ECS::BPLayerInterfaceImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - BPLayerInterfaceImpl(), GetNumBroadPhaseLayers(), GetBroadPhaseLayer()
- **Tsukino::BuiltIn::ECS::CameraSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/CameraSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::CharacterContactListenerImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - physicsSystem, OnContactAdded()
- **Tsukino::BuiltIn::ECS::ClipRangeTicks** — `Tsukino.EngineIntegration/src/ECS/System/AnimationSystem.cpp`
  - start, end
- **Tsukino::BuiltIn::ECS::EffectSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/EffectSystem.hpp`
  - EffectSystem(), ~EffectSystem(), Update(), RenderEffects(), PlayEffect(), StopEffect(), PauseHandle(), ResumeHandle(), IsPlaying(), SetPlaySpeed(), SendTrigger(), StopAllEffects(), IsInitialized(), Initialize(), Finalize()
- **Tsukino::BuiltIn::ECS::FogSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/FogSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::FontRendererSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp`
  - FontRendererSystem(), ~FontRendererSystem(), Update()
- **Tsukino::BuiltIn::ECS::FontRendererSystem::DrawEntry** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/FontRendererSystem.hpp`
  - atlas, spriteFont, text, position, origin, color, outlineColor, outlineWidth, scale, sortOrder
- **Tsukino::BuiltIn::ECS::HeightmapGenerationSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/HeightmapGenerationSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::InteractionSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/InteractionSystem.hpp`
  - Update(), HitTest()
- **Tsukino::BuiltIn::ECS::JoltDebugRendererImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - JoltDebugRendererImpl(), DrawLine(), DrawTriangle(), DrawText3D(), SetEngineRenderer()
- **Tsukino::BuiltIn::ECS::LightSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/LightSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::ModelSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/ModelSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::MotionBlurSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/MotionBlurSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::MotionVectorSnapshotSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/MotionVectorSnapshotSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::MyContactListener** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - OnContactAdded(), DrainContacts()
- **Tsukino::BuiltIn::ECS::MyContactListener::PendingContact** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - entityA, entityB, normal
- **Tsukino::BuiltIn::ECS::ObjectLayerPairFilterImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - ShouldCollide()
- **Tsukino::BuiltIn::ECS::ObjectVsBroadPhaseLayerFilterImpl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - ShouldCollide()
- **Tsukino::BuiltIn::ECS::PhysicsSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/PhysicsSystem.hpp`
  - PhysicsSystem(), ~PhysicsSystem(), Update(), OverlapCapsule()
- **Tsukino::BuiltIn::ECS::PhysicsSystem::Impl** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - tempAllocator, jobSystem, bpLayerInterface, objVsBpFilter, objPairFilter, physicsSystem, contactListener, debugRenderer, prevPositions, heightfieldCache, characterContactListener, characters, eventBus, drainedContacts
- **Tsukino::BuiltIn::ECS::PhysicsSystem::Impl::CharacterHandle** — `Tsukino.EngineIntegration/src/ECS/System/PhysicsSystem.cpp`
  - character
- **Tsukino::BuiltIn::ECS::SkyAtmosphereSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/SkyAtmosphereSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::SpriteRenderSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp`
  - SpriteRenderSystem(), ~SpriteRenderSystem(), Update()
- **Tsukino::BuiltIn::ECS::SpriteRenderSystem::SpriteEntry** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/SpriteRendererSystem.hpp`
  - sortOrder, cmd
- **Tsukino::BuiltIn::ECS::TransformSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/TransformSystem.hpp`
  - Update()
- **Tsukino::BuiltIn::ECS::WorldAnchorSystem** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/ECS/System/WorldAnchorSystem.hpp`
  - Update()
- **Tsukino::EngineIntegration::EffectFileInterface** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/IO/EffectFileInterface.hpp`
  - EffectFileInterface(), ~EffectFileInterface(), SetBaseDirectory(), OpenRead(), OpenWrite()
- **Tsukino::EngineIntegration::EffectFileReader** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/IO/EffectFileInterface.hpp`
  - EffectFileReader(), Read(), Seek(), GetPosition(), GetLength(), GetData()
- **Tsukino::EngineIntegration::EngineAPI** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineAPI.hpp`
  - EngineAPI(), ChangeScene(), ProcessMessages(), Update(), Render()
- **Tsukino::EngineIntegration::EngineContext** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineContext.hpp`
  - renderer, window, inputSystem, assetManager, builtinAssets, gameSceneManager, audioManager, prefabFactory, effectSystem, physicsSystem
- **Tsukino::EngineIntegration::EngineIntegration** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/EngineIntegration.hpp`
  - EngineIntegration(), ~EngineIntegration(), EngineIntegration(), operator=(), EngineIntegration(), operator=(), Initialize(), GetContext()
- **Tsukino::EngineIntegration::GameSceneBase** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/Scene/GameSceneBase.hpp`
  - ~GameSceneBase(), Initialize(), OnUpdate(), OnExit(), GetScene()
- **Tsukino::EngineIntegration::GameSceneManager** — `Tsukino.EngineIntegration/include/Tsukino/EngineIntegration/Scene/GameSceneManager.hpp`
  - GameSceneManager(), ~GameSceneManager(), GameSceneManager(), operator=(), GameSceneManager(), operator=(), Initialize(), ChangeScene(), Update(), GetCurrentScene()
