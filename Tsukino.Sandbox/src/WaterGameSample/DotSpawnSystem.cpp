//-------------------------------------------------------------
//! @file   DotSpawnSystem.cpp
//! @brief  DotSpawnSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/WaterGameSample/ECS/System/DotSpawnSystem.hpp>

#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/DotSpawnerComponenet.hpp>
#include <Tsukino/Sandbox/WaterGameSample/ECS/Component/DotComponenet.hpp>

#include <Tsukino/BuiltIn/ECS/Component/CollisionComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/ECS/Registry/Registry.hpp>

#include <hlsl++.h>
#include <random>
#include <cmath>
#include <algorithm>

// 名前空間 : WaterGame::ECS
namespace WaterGame::ECS {

    namespace {
        //-------------------------------------------------------------
        //! @brief  ハイトフィールドのサンプルデータから、任意のワールドXZ座標の高さを
        //!         バイリニア補間で求める（真上からレイを飛ばした場合と同じ結果になる）
        //! @param  col       [in]  Heightfield用のCollisionComponent
        //! @param  worldX    [in]  ワールドX座標
        //! @param  worldZ    [in]  ワールドZ座標
        //! @param  outHeight [out] 求めた高さ
        //! @return グリッド範囲内であればtrue
        //-------------------------------------------------------------
        bool TryGetHeightFromHeightfield(const Tsukino::BuiltIn::ECS::CollisionComponent& col, float worldX, float worldZ, float& outHeight) {
            if(col.heightfieldSamples.empty() || col.heightfieldSize <= 1)
                return false;

            // ワールド座標 -> グリッド座標（浮動小数点）へ逆算
            float gx = (worldX - col.heightfieldOffset.x) / col.heightfieldScale.x;
            float gz = (worldZ - col.heightfieldOffset.z) / col.heightfieldScale.z;

            int size = col.heightfieldSize;

            // 範囲外なら失敗
            if(gx < 0.0f || gz < 0.0f || gx > (float)(size - 1) || gz > (float)(size - 1))
                return false;

            int   ix    = (int)std::floor(gx);
            int   iz    = (int)std::floor(gz);
            int   ix1   = std::min(ix + 1, size - 1);
            int   iz1   = std::min(iz + 1, size - 1);
            float fracX = gx - (float)ix;
            float fracZ = gz - (float)iz;

            auto sampleAt = [&](int x, int z) -> float { return col.heightfieldSamples[(size_t)z * (size_t)size + (size_t)x]; };

            // バイリニア補間
            float h00 = sampleAt(ix, iz);
            float h10 = sampleAt(ix1, iz);
            float h01 = sampleAt(ix, iz1);
            float h11 = sampleAt(ix1, iz1);

            float h0 = h00 + (h10 - h00) * fracX;
            float h1 = h01 + (h11 - h01) * fracX;
            float h  = h0 + (h1 - h0) * fracZ;

            outHeight = col.heightfieldOffset.y + h * col.heightfieldScale.y;
            return true;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief システムの更新処理
    //-------------------------------------------------------------
    void DotSpawnSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* context = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!context)
            return;

        //-------------------------------------------------------------
        // 地形（Heightfield）のCollisionComponentを1つ探す
        //-------------------------------------------------------------
        const Tsukino::BuiltIn::ECS::CollisionComponent* terrainCol = nullptr;

        auto terrainView = registry.View<Tsukino::BuiltIn::ECS::CollisionComponent>();
        for(auto entity : terrainView) {
            auto& col = registry.GetComponent<Tsukino::BuiltIn::ECS::CollisionComponent>(entity);
            if(col.type == Tsukino::BuiltIn::ECS::ColliderType::Heightfield && !col.heightfieldSamples.empty()) {
                terrainCol = &col;
                break;
            }
        }

        if(!terrainCol)
            return;    // 地形データがまだ生成されていない（HeightmapGenerationSystem待ち）

        //-------------------------------------------------------------
        // スポーン処理
        //-------------------------------------------------------------
        auto spawnerView = registry.View<DotSpawnerComponent>();

        spawnerView.each([&](auto entity, auto& spawner) {
            if(spawner.hasSpawned)
                return;

            std::mt19937                          rng(spawner.seed);
            // 形の範囲を考慮した動的な生成範囲にする
            float minX = terrainCol->heightfieldOffset.x;
            float maxX = minX + (float)(terrainCol->heightfieldSize - 1) * terrainCol->heightfieldScale.x;
            float minZ = terrainCol->heightfieldOffset.z;
            float maxZ = minZ + (float)(terrainCol->heightfieldSize - 1) * terrainCol->heightfieldScale.z;

            // 原点を中心に広げたい場合は、地形の範囲と areaHalfSize の交差領域を使う
            // あるいはシンプルに地形全域に生成するなら以下のようにします
            std::uniform_real_distribution<float> distX(minX, maxX);
            std::uniform_real_distribution<float> distZ(minZ, maxZ);

            int       spawnedCount = 0;
            int       attempts     = 0;
            const int maxAttempts  = spawner.dotCount * 4;

            while(spawnedCount < spawner.dotCount && attempts < maxAttempts) {
                attempts++;

                float worldX = distX(rng);
                float worldZ = distZ(rng);

                float groundHeight = 0.0f;
                if(!TryGetHeightFromHeightfield(*terrainCol, worldX, worldZ, groundHeight))
                    continue;    // グリッド範囲外

                Tsukino::ECS::Entity dotEntity = registry.CreateEntity();

                auto& tf    = registry.AddComponent<Tsukino::BuiltIn::ECS::TransformComponent>(dotEntity);
                tf.position = hlslpp::float3(worldX, groundHeight + spawner.heightOffset, worldZ);
                tf.rotation = hlslpp::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
                tf.scale    = hlslpp::float3(1.0f, 1.0f, 1.0f);
                tf.dirty    = true;

                auto& model       = registry.AddComponent<Tsukino::BuiltIn::ECS::ModelComponent>(dotEntity);
                model.modelHandle = context->assetManager->Load(Tsukino::Core::Path("Tsukino.Sandbox/Assets/WaterGameSample/Models/Dot.fbx"));
                model.visible     = true;

                auto& dot      = registry.AddComponent<DotComponent>(dotEntity);
                dot.scoreValue = 1;

                spawnedCount++;
            }

            spawner.hasSpawned = true;
        });
    }

}    // namespace WaterGame::ECS
