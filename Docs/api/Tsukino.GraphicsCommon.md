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
- **Tsukino::GraphicsCommon::MaterialData** — `Tsukino.GraphicsCommon/include/Tsukino/GraphicsCommon/Material/MaterialData.hpp`
  - name, shadingModel, baseColor, emissive, metallic, roughness, specular, waterSpeed, waterScale, waterHeight, albedoMap, normalMap, metallicRoughnessMap, emissiveMap, aoMap, serialize()
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
