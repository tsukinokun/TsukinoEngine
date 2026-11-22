# Tsukino.GraphicsCommon の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::GraphicsCommon::AABB** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Mesh/MeshData.hpp`
  - min, max, serialize()
- **Tsukino::GraphicsCommon::AnimationChannel** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - nodeName, positionKeys, rotationKeys, scaleKeys, serialize()
- **Tsukino::GraphicsCommon::AnimationData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - name, duration, ticksPerSecond, channels, serialize()
- **Tsukino::GraphicsCommon::BoneInfo** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - name, nodeIndex, inverseBindPose, serialize()
- **Tsukino::GraphicsCommon::BoneWeight** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Mesh/MeshData.hpp`
  - boneIndices, weights, serialize()
- **Tsukino::GraphicsCommon::DebugVertex** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/DebugVertex.hpp`
  - position, color
- **Tsukino::GraphicsCommon::Float2** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/PackedFloat.hpp`
  - x, y
- **Tsukino::GraphicsCommon::Float3** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/PackedFloat.hpp`
  - x, y, z
- **Tsukino::GraphicsCommon::Float4** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/PackedFloat.hpp`
  - x, y, z, w
- **Tsukino::GraphicsCommon::MaterialData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Material/MaterialData.hpp`
  - name, shadingModel, baseColor, emissive, metallic, roughness, specular, alphaCutoff, toonThreshold, toonSmoothness, toonShadeColor, toonSpecularSize, albedoMap, normalMap, metallicRoughnessMap, emissiveMap, aoMap, serialize()
- **Tsukino::GraphicsCommon::MeshData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Mesh/MeshData.hpp`
  - vertexData, indices, boneWeights, vertexStride, vertexCount, indexCount, materialIndex, format, bounds, serialize()
- **Tsukino::GraphicsCommon::ModelData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - nodes, meshes, materials, animations, skeleton, rootNodeIndex, serialize()
- **Tsukino::GraphicsCommon::NodeData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Node/NodeData.hpp`
  - name, meshIndices, translation, rotation, scale, parentIndex, childIndices, serialize()
- **Tsukino::GraphicsCommon::QuaternionKey** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - time, value, serialize()
- **Tsukino::GraphicsCommon::SkeletonData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - bones, serialize()
- **Tsukino::GraphicsCommon::VectorKey** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Model/ModelData.hpp`
  - time, value, serialize()
- **Tsukino::GraphicsCommon::VertexPNUV** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/VertexPNUV.hpp`
  - position, normal, uv
- **Tsukino::GraphicsCommon::VertexPUV** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/VertexPUV.hpp`
  - position, uv

## 列挙型

- **Tsukino::GraphicsCommon::PrimitiveType** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Mesh/PrimitiveType.hpp` — プリミティブの種類を表す列挙型
  - `Quad`
  - `Count`
- **Tsukino::GraphicsCommon::SamplerType** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/State/SamplerType.hpp` — 描画時のテクスチャサンプリング方式
  - `PointWrap` — ドット絵・UI用（補間なし / リピート）
  - `PointClamp` — ドット絵・UI用（補間なし / 切り捨て）
  - `LinearWrap` — 一般テクスチャ用（なめらか / リピート）
  - `LinearClamp` — スプライト用（なめらか / 切り捨て）
  - `AnisotropicWrap` — 高品質3D用（異方性フィルタ / リピート）
  - `AnisotropicClamp` — 高品質3D用（異方性フィルタ / 切り捨て）
  - `Count`
- **Tsukino::GraphicsCommon::ShadingModel** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Material/MaterialData.hpp` — シェーディングモデルの種類
  - `PBR`
  - `Unlit`
  - `Toon`
- **Tsukino::GraphicsCommon::TextureFormat** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Texture/TextureFormat.hpp`
  - `Unknown` — 不明（圧縮の規則では「圧縮しない」の意味）
  - `R8` — 8bit × 1ch
  - `RG8` — 8bit × 2ch
  - `RGBA8` — 8bit × 4ch（線形）
  - `RGBA8_sRGB` — 8bit × 4ch（sRGB）
  - `BGRA8` — 8bit × 4ch・BGRA の並び（線形）
  - `BGRA8_sRGB` — 8bit × 4ch・BGRA の並び（sRGB）
  - `R16` — 16bit 整数 × 1ch
  - `RGBA16` — 16bit 整数 × 4ch
  - `R16F` — 16bit 浮動小数 × 1ch
  - `RGBA16F` — 16bit 浮動小数 × 4ch
  - `R32F` — 32bit 浮動小数 × 1ch
  - `RGBA32F` — 32bit 浮動小数 × 4ch
  - `RGB10A2` — 10bit × 3ch ＋ 2bit
  - `BC1` — ブロック圧縮 BC1（線形。アルファは 1bit）
  - `BC1_sRGB` — ブロック圧縮 BC1（sRGB）
  - `BC3` — ブロック圧縮 BC3（線形。アルファ付き）
  - `BC3_sRGB` — ブロック圧縮 BC3（sRGB）
  - `BC4` — ブロック圧縮 BC4（1ch）
  - `BC5` — ブロック圧縮 BC5（2ch。法線マップ向け）
  - `BC7` — ブロック圧縮 BC7（線形。高品質）
  - `BC7_sRGB` — ブロック圧縮 BC7（sRGB）
  - `ASTC4x4` — ASTC 4×4（線形。スマホ向けの高品質な圧縮。BC3 / BC7 の代わり）
  - `ASTC4x4_sRGB` — ASTC 4×4（sRGB）
  - `ASTC6x6` — ASTC 6×6（線形。スマホ向けの軽い圧縮。BC1 の代わり）
  - `ASTC6x6_sRGB` — ASTC 6×6（sRGB）
  - `Count`
- **Tsukino::GraphicsCommon::VertexFormat** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Vertex/VertexFormat.hpp` — 頂点フォーマットの種類を定義する列挙型
  - `Unknown = 0`
  - `PositionUV`
  - `PositionNormalUV`
  - `Skinned`
  - `Sprite`
  - `Particle`

## 名前空間の関数

### Tsukino::GraphicsCommon — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Mesh/MeshPrimitives.hpp`

| シグネチャ | 説明 |
|---|---|
| `MeshData CreatePrimitiveMeshData(PrimitiveType type)` | プリミティブの種類を指定してメッシュデータを生成する関数 |
| `MeshData CreateQuadMeshData()` | 四角形のメッシュデータを生成する関数 |
