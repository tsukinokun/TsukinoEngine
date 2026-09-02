# Tsukino.Engine の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 主要な型

ゲーム側から実際に触る型。メンバを全て展開している。

### Tsukino::Asset::AssetHandle

`Tsukino.Engine/include/Tsukino/Engine/Asset/AssetHandle.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `AssetHandle()=default` | デフォルトコンストラクタ |
| `AssetHandle(u64 value)` | 引数付きコンストラクタ |
| `u64 Value() const` | 値を取得する関数 |
| `bool IsValid() const` | ハンドルが有効か確認する関数 |
| `bool operator==(const AssetHandle &other) const` | 同一のハンドルか比較する演算子オーバーロード |
| `bool operator!=(const AssetHandle &other) const` | 異なるハンドルか比較する演算子オーバーロード |
| `AssetHandle Invalid()` | 無効なハンドルを取得する関数 |

### Tsukino::Asset::AssetManager

`Tsukino.Engine/include/Tsukino/Engine/Asset/AssetManager.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `AssetManager()=default` |  |
| `~AssetManager()` | デストラクタ |
| `void Initialize()` | AssetManagerを初期化する関数 |
| `AssetHandle Load(const Tsukino::Core::Path &path)` | アセットをロードする関数 |
| `Tsukino::Core::Ref< IAsset > Get(AssetHandle handle)` | 任意のアセットハンドルからアセットを取得する関数 |
| `bool Exists(AssetHandle handle)` | アセットハンドルが存在するか確認する関数 |
| `void RegisterImporter(AssetType type, Tsukino::Core::Ref< IAssetImporter > importer)` | インポーターを登録する関数 |
| `void RegisterAsset(AssetHandle handle, Tsukino::Core::Ref< IAsset > asset)` | アセットを登録する関数 |

### Tsukino::Asset::AssetRef

`Tsukino.Engine/include/Tsukino/Engine/Asset/AssetRef.hpp`

**公開メンバ**

| メンバ | 説明 |
|---|---|
| `AssetHandle handle` |  |
| `std::string path` |  |

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `AssetRef()=default` |  |
| `AssetRef(AssetHandle h)` |  |
| `AssetRef & operator=(AssetHandle h)` |  |
| `operator AssetHandle() const` |  |
| `bool IsValid() const` |  |
| `u64 Value() const` |  |
| `bool operator==(const AssetRef &other) const` |  |
| `bool operator!=(const AssetRef &other) const` |  |
| `bool operator==(AssetHandle other) const` |  |
| `bool operator!=(AssetHandle other) const` |  |

### Tsukino::ECS::Scene

`Tsukino.Engine/include/Tsukino/Engine/ECS/Scene.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `Scene()=default` | デフォルトコンストラクタ |
| `~Scene()=default` | デストラクタ |
| `void Initialize()` | シーン初期化 |
| `void Update(float deltaTime)` | 毎フレームの更新 |
| `void Shutdown()` | シーンの終了処理 |
| `Entity CreateEntity()` | エンティティの生成 |
| `void DestroyEntity(Entity entity)` | エンティティの破棄 |
| `Registry & GetRegistry()` |  |
| `void AddSystem(std::shared_ptr< ISystem > system, int priority=0)` | システムの追加 |
| `EventBus & GetEventBus()` | EventBus へのアクセス |

### Tsukino::Engine::ECS::Prefab::PrefabFactory

`Tsukino.Engine/include/Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `PrefabFactory()=default` | デフォルトコンストラクタ |
| `~PrefabFactory()=default` | デストラクタ |
| `PrefabFactory(const PrefabFactory &)=delete` |  |
| `PrefabFactory & operator=(const PrefabFactory &)=delete` |  |
| `PrefabFactory(PrefabFactory &&)=default` |  |
| `PrefabFactory & operator=(PrefabFactory &&)=default` |  |
| `template <typename ComponentType> void RegisterComponent(const std::string &typeName)` | コンポーネントの型名とロード処理を工場に登録する |
| `void SetAssetManager(Tsukino::Asset::AssetManager *assetManager)` | AssetRefフィールドの解決に使うAssetManagerを設定する |
| `template <typename T> void ApplyOverride(Tsukino::ECS::Registry &registry, entt::entity entity, const T &overrideValue)` | 生成済みエンティティの指定コンポーネントを、任意の値で上書きする |
| `entt::entity Instantiate(const std::string &prefabJsonPath, Tsukino::ECS::Registry &registry)` | PrefabのJSON（目次）から動的にコンポーネントを組み立ててエンティティを生成する |
| `PrefabInstance InstantiateGroup(const std::vector< GroupEntry > &entries, Tsukino::ECS::Registry &registry)` | 複数のPrefabを名前付きでまとめて生成し、コンポーネント内のEntityRefフィールドを バッチ内の他エンティティへ解決する |
| `PrefabInstance InstantiateGroup(const std::string &groupJsonPath, Tsukino::ECS::Registry &registry)` | グループ定義JSON（"Entities": {名前: Prefabパス} のマップ）からバッチ生成を行う |
| `bool CaptureEntity(Tsukino::ECS::Registry &registry, entt::entity entity, const std::string &outDir)` | 生成済みエンティティが持つ、セーブ対応済みコンポーネントをすべてJSONへ書き出し、 それらをまとめるPrefab（目次）JSONも生成する（開発者向けブートストラップ用途） |
| `template <typename T> bool Load(const std::string &jsonPath, const std::string &keyName, T &outData)` | 任意のComponent（T）をJSONファイルからロードする（個別用） |
| `template <typename T> bool Save(const std::string &jsonPath, const std::string &keyName, const T &desc)` | 任意のComponent（T）をJSONファイルにセーブする（個別用） |

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::Asset::AssetHandle** — `Tsukino.Engine/include/Tsukino/Engine/Asset/AssetHandle.hpp`
  - AssetHandle(), AssetHandle(), Value(), IsValid(), operator==(), operator!=(), Invalid()
- **Tsukino::Asset::AssetHandleGenerator** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Util/AssetHandleGenerator.hpp`
  - Generate()
- **Tsukino::Asset::AssetManager** — `Tsukino.Engine/include/Tsukino/Engine/Asset/AssetManager.hpp`
  - AssetManager(), ~AssetManager(), Initialize(), Load(), Get(), Exists(), RegisterImporter(), RegisterAsset()
- **Tsukino::Asset::AssetRef** — `Tsukino.Engine/include/Tsukino/Engine/Asset/AssetRef.hpp`
  - handle, path, AssetRef(), AssetRef(), operator=(), operator AssetHandle(), IsValid(), Value(), operator==(), operator!=(), operator==(), operator!=()
- **Tsukino::Asset::AssetRefResolverArchive** — `Tsukino.Engine/include/Tsukino/Engine/Asset/AssetRefResolverArchive.hpp`
  - AssetRefResolverArchive(), operator()()
- **Tsukino::Asset::AudioAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Audio/AudioAsset.hpp`
  - metadata, waveBankPath, waveIndex, GetHandle(), SetHandle(), GetType()
- **Tsukino::Asset::AudioImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Audio/AudioImporter.hpp`
  - Import()
- **Tsukino::Asset::AudioLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Audio/AudioLoader.hpp`
  - AudioLoader(), CanLoad(), Load()
- **Tsukino::Asset::CubemapAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Cubemap/CubemapAsset.hpp`
  - ddsData, width, height, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::CubemapDesc** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Cubemap/CubemapDesc.hpp`
  - px, nx, py, ny, pz, nz, serialize()
- **Tsukino::Asset::CubemapImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Cubemap/CubemapImporter.hpp`
  - Import()
- **Tsukino::Asset::CubemapLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Cubemap/CubemapLoader.hpp`
  - CubemapLoader(), CanLoad(), Load()
- **Tsukino::Asset::DynamicFontAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/DynamicFontAsset.hpp`
  - m_faceName, m_pixelSize, m_fontFileData, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::DynamicFontImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/DynamicFontImporter.hpp`
  - Import()
- **Tsukino::Asset::DynamicFontLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/DynamicFontLoader.hpp`
  - DynamicFontLoader(), CanLoad(), Load()
- **Tsukino::Asset::EffectAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Effect/EffectAsset.hpp`
  - binary, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::EffectImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Effect/EffectImporter.hpp`
  - EffectImporter(), Import()
- **Tsukino::Asset::EffectLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Effect/EffectLoader.hpp`
  - EffectLoader(), CanLoad(), Load()
- **Tsukino::Asset::FontAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/FontAsset.hpp`
  - m_binaryData, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::FontImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/FontImporter.hpp`
  - Import()
- **Tsukino::Asset::FontLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Font/FontLoader.hpp`
  - FontLoader(), CanLoad(), Load()
- **Tsukino::Asset::IAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/IAsset.hpp`
  - ~IAsset(), GetHandle(), SetHandle(), GetType()
- **Tsukino::Asset::IAssetImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/IAssetImporter.hpp`
  - ~IAssetImporter(), Import()
- **Tsukino::Asset::IAssetLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/IAssetLoader.hpp`
  - ~IAssetLoader(), CanLoad(), Load()
- **Tsukino::Asset::MaterialAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Material/MaterialAsset.hpp`
  - data, albedoHandle, normalHandle, metallicRoughnessHandle, emissiveHandle, aoHandle, vertexShaderHandle, pixelShaderHandle, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::MeshAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Mesh/MeshAsset.hpp`
  - data, MeshAsset(), GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::ModelAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Model/ModelAsset.hpp`
  - modelData, materialHandles, ModelAsset(), GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::ModelImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Model/ModelImporter.hpp`
  - Import()
- **Tsukino::Asset::ModelLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Model/ModelLoader.hpp`
  - ModelLoader(), CanLoad(), Load()
- **Tsukino::Asset::ShaderAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Shader/ShaderAsset.hpp`
  - shaderStage, entryPoint, profile, source, binary, filePath, ShaderAsset(), GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::ShaderImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Shader/ShaderImporter.hpp`
  - Import()
- **Tsukino::Asset::ShaderLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Shader/ShaderLoader.hpp`
  - ShaderLoader(), CanLoad(), Load(), DetectStage()
- **Tsukino::Asset::TextureAsset** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Texture/TextureAsset.hpp`
  - width, height, format, pixels, GetHandle(), GetType(), SetHandle()
- **Tsukino::Asset::TextureImporter** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Texture/TextureImporter.hpp`
  - Import()
- **Tsukino::Asset::TextureLoader** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Texture/TextureLoder.hpp`
  - TextureLoader(), CanLoad(), Load()
- **Tsukino::Asset::Vertex** — `Tsukino.Engine/src/Asset/Model/ModelImporter.cpp`
  - position, normal, texcoord
- **Tsukino::Asset::XWBEntry** — `Tsukino.Engine/include/Tsukino/Engine/Asset/Audio/AudioAsset.hpp`
  - offset, length, sampleRate, channels, formatTag
- **Tsukino::ECS::EngineEvent::EntityCreatedEvent** — `Tsukino.Engine/include/Tsukino/Engine/ECS/EngineEvent/EntityEvent.hpp`
  - entity
- **Tsukino::ECS::EngineEvent::EntityDestroyedEvent** — `Tsukino.Engine/include/Tsukino/Engine/ECS/EngineEvent/EntityEvent.hpp`
  - entity
- **Tsukino::ECS::EngineEvent::SceneChangeRequestedEvent** — `Tsukino.Engine/include/Tsukino/Engine/ECS/EngineEvent/SceneEvent.hpp`
  - nextSceneName
- **Tsukino::ECS::EngineEvent::SceneInitializedEvent** — `Tsukino.Engine/include/Tsukino/Engine/ECS/EngineEvent/SceneEvent.hpp`
  - （公開メンバなし）
- **Tsukino::ECS::Scene** — `Tsukino.Engine/include/Tsukino/Engine/ECS/Scene.hpp`
  - Scene(), ~Scene(), Initialize(), Update(), Shutdown(), CreateEntity(), DestroyEntity(), GetRegistry(), AddSystem(), GetEventBus()
- **Tsukino::ECS::SystemManager** — `Tsukino.Engine/include/Tsukino/Engine/ECS/SystemManager.hpp`
  - SystemManager(), ~SystemManager(), AddSystem(), Update(), Clear(), GetProfiles()
- **Tsukino::ECS::SystemManager::SystemEntry** — `Tsukino.Engine/include/Tsukino/Engine/ECS/SystemManager.hpp`
  - system, priority, name, accumulatedMs
- **Tsukino::ECS::SystemManager::SystemProfile** — `Tsukino.Engine/include/Tsukino/Engine/ECS/SystemManager.hpp`
  - name, averageMs
- **Tsukino::Engine::ECS::Prefab::PrefabFactory** — `Tsukino.Engine/include/Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp`
  - PrefabFactory(), ~PrefabFactory(), PrefabFactory(), operator=(), PrefabFactory(), operator=(), RegisterComponent(), SetAssetManager(), ApplyOverride(), Instantiate(), InstantiateGroup(), InstantiateGroup(), CaptureEntity(), Load(), Save()
- **Tsukino::Engine::ECS::Prefab::PrefabFactory::GroupEntry** — `Tsukino.Engine/include/Tsukino/Engine/ECS/Prefab/PrefabFactory.hpp`
  - name, prefabPath
- **Tsukino::Physics::SpringBoneChain** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - name, anchorNodeIndex, settings, nodes, colliders, previousAnchorPosition, anchorInitialized
- **Tsukino::Physics::SpringBoneNode** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - nodeIndex, parentIndexInChain, restLength, currentPosition, previousPosition, correctedRotation, initialized
- **Tsukino::Physics::SpringBoneSettings** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - stiffness, drag, inertia, gravityScale, gravityDir, boneRadius, angleLimitDeg, collisionIterations, serialize()
- **Tsukino::Physics::SpringColliderSphere** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - attachNodeIndex, localOffset, radius, serialize()
- **Tsukino::Physics::WorldPose** — `Tsukino.Engine/include/Tsukino/Engine/Physics/SpringBone/SpringBoneData.hpp`
  - position, rotation
