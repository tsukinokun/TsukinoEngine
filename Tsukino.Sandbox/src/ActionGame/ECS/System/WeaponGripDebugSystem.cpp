//-------------------------------------------------------------
//! @file   WeaponGripDebugSystem.cpp
//! @brief  WeaponGripDebugSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/ActionGame/ECS/System/WeaponGripDebugSystem.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/WeaponGripDebugComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/WeaponComponent.hpp>
#include <Tsukino/Sandbox/ActionGame/ECS/Component/PlayerAnimationSetComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/FontComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/ModelComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/AnimationPlayerComponent.hpp>

#include <Tsukino/EngineIntegration/EngineContext.hpp>
#include <Tsukino/Engine/Asset/AssetManager.hpp>
#include <Tsukino/Engine/Asset/Model/ModelAsset.hpp>
#include <Tsukino/GraphicsCommon/Model/ModelData.hpp>

#include <Tsukino/Core/Input/InputSystem.hpp>
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Core/Log.hpp>

#include <hlsl++.h>
#include <cfloat>
#include <sstream>
#include <iomanip>
// 名前空間 : ActionGame::ECS
namespace ActionGame::ECS {
    namespace {
        constexpr float kPositionSteps[3]      = {0.1f, 1.0f, 10.0f};    //!< F8で切り替える位置系パラメータの刻み幅
        constexpr float kAngleStepsDeg[3]      = {1.0f, 5.0f, 15.0f};    //!< F8で切り替える角度系パラメータの刻み幅（度）
        constexpr float kPoseFreezeStepSeconds = 1.0f / 30.0f;           //!< F11で1回進める時間（約1フレーム分）

        //-------------------------------------------------------------
        //! @brief  編集対象のHUD表示名を返す
        //-------------------------------------------------------------
        const wchar_t* EditTargetName(WeaponGripEditTarget target) {
            switch(target) {
            case WeaponGripEditTarget::GripPoint:
                return L"GripPoint（握り点）";
            case WeaponGripEditTarget::AttackLocalOffset:
                return L"AttackLocalOffset（手のひらオフセット）";
            case WeaponGripEditTarget::GripRotation:
                return L"GripRotation（握り角度）";
            }
            return L"?";
        }

        //-------------------------------------------------------------
        //! @brief  編集対象に対応するWeaponComponentの位置系フィールドを返す
        //!         （GripRotationの場合はnullptr。角度は四元数のため別経路で扱う）
        //-------------------------------------------------------------
        hlslpp::float3* ResolveVectorTarget(WeaponComponent& weapon, WeaponGripEditTarget target) {
            switch(target) {
            case WeaponGripEditTarget::GripPoint:
                return &weapon.gripPointLocal;
            case WeaponGripEditTarget::AttackLocalOffset:
                return &weapon.attackLocalOffset;
            default:
                return nullptr;
            }
        }

        //-------------------------------------------------------------
        //! @brief  武器メッシュ（非スケルタル）のローカル空間AABBを計算する。
        //!         ModelSystemの非スケルタル描画パスと同じノード変換
        //!         （各ノード自身のローカルSRTのみ。親ノードの変換は積まない）で求めることで、
        //!         gripPointLocal等を調整する空間＝実際に描画される空間を一致させる
        //! @param  modelData [in]  武器モデルのデータ
        //! @param  outMin    [out] AABBの最小点
        //! @param  outMax    [out] AABBの最大点
        //! @return メッシュが1つ以上見つかったか
        //-------------------------------------------------------------
        bool ComputeWeaponLocalAABB(const Tsukino::GraphicsCommon::ModelData& modelData, hlslpp::float3& outMin, hlslpp::float3& outMax) {
            bool found = false;
            outMin      = hlslpp::float3(FLT_MAX, FLT_MAX, FLT_MAX);
            outMax      = hlslpp::float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            for(const auto& node : modelData.nodes) {
                if(node.meshIndices.empty())
                    continue;

                Tsukino::Core::Math::matrix scaleMat = Tsukino::Core::Math::matrix::scale(hlslpp::float3(node.scale.x, node.scale.y, node.scale.z));
                Tsukino::Core::Math::matrix rotMat =
                    Tsukino::Core::Math::matrix::rotate(hlslpp::quaternion(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w));
                Tsukino::Core::Math::matrix transMat =
                    Tsukino::Core::Math::matrix::translate(hlslpp::float3(node.translation.x, node.translation.y, node.translation.z));
                Tsukino::Core::Math::matrix nodeLocal = hlslpp::mul(hlslpp::mul(scaleMat, rotMat), transMat);

                for(u32 meshIdx : node.meshIndices) {
                    if(meshIdx >= modelData.meshes.size())
                        continue;
                    const auto& bounds = modelData.meshes[meshIdx].bounds;

                    for(int i = 0; i < 8; ++i) {
                        hlslpp::float3 corner((i & 1) ? bounds.max.x : bounds.min.x, (i & 2) ? bounds.max.y : bounds.min.y,
                                              (i & 4) ? bounds.max.z : bounds.min.z);
                        hlslpp::float4 worldCorner = hlslpp::mul(hlslpp::float4(corner, 1.0f), nodeLocal);

                        outMin = hlslpp::min(outMin, worldCorner.xyz);
                        outMax = hlslpp::max(outMax, worldCorner.xyz);
                        found   = true;
                    }
                }
            }
            return found;
        }
    }    // namespace

    //-------------------------------------------------------------
    //! @brief システムの更新
    //-------------------------------------------------------------
    void WeaponGripDebugSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        auto* ctx = registry.GetContext<Tsukino::EngineIntegration::EngineContext*>();
        if(!ctx || !ctx->inputSystem)
            return;

        Tsukino::Input::InputSystem* input = ctx->inputSystem;

        //-------------------------------------------------------------
        // HUD（シーン初期化時に1つだけ生成されている前提）を探す
        //-------------------------------------------------------------
        WeaponGripDebugComponent*             debugState = nullptr;
        Tsukino::BuiltIn::ECS::FontComponent* hudFont     = nullptr;
        {
            auto hudView = registry.View<WeaponGripDebugComponent, Tsukino::BuiltIn::ECS::FontComponent>();
            for(auto entity : hudView) {
                debugState = &hudView.get<WeaponGripDebugComponent>(entity);
                hudFont     = &hudView.get<Tsukino::BuiltIn::ECS::FontComponent>(entity);
                break;
            }
        }
        if(!debugState || !hudFont)
            return;

        // F6：調整モードのON/OFF
        if(input->IsKeyPressed(Tsukino::Input::KeyCode::F6)) {
            debugState->enabled = !debugState->enabled;
            if(!debugState->enabled)
                debugState->poseFrozen = false;    // OFFにする際はポーズ固定も解除しておく
        }

        if(!debugState->enabled) {
            hudFont->text.clear();    // 空文字ならFontRendererSystemが描画をスキップする
            return;
        }

        //-------------------------------------------------------------
        // プレイヤーと装備中の武器を特定する（単一プレイヤー前提。他Systemと同じ方針）
        //-------------------------------------------------------------
        Tsukino::ECS::Entity playerEntity = entt::null;
        PlayerComponent*     player       = nullptr;
        {
            auto playerView = registry.View<PlayerComponent>();
            for(auto entity : playerView) {
                playerEntity = entity;
                player       = &playerView.get<PlayerComponent>(entity);
                break;
            }
        }

        if(!player || player->weaponEntity == entt::null || !registry.HasComponent<WeaponComponent>(player->weaponEntity)) {
            hudFont->text = L"[武器グリップ調整] 装備中の武器がありません";
            return;
        }

        WeaponComponent& weapon = registry.GetComponent<WeaponComponent>(player->weaponEntity);

        // F7：編集対象の切り替え（GripPoint → AttackLocalOffset → GripRotation → …）
        if(input->IsKeyPressed(Tsukino::Input::KeyCode::F7)) {
            switch(debugState->editTarget) {
            case WeaponGripEditTarget::GripPoint:
                debugState->editTarget = WeaponGripEditTarget::AttackLocalOffset;
                break;
            case WeaponGripEditTarget::AttackLocalOffset:
                debugState->editTarget = WeaponGripEditTarget::GripRotation;
                break;
            case WeaponGripEditTarget::GripRotation:
                debugState->editTarget = WeaponGripEditTarget::GripPoint;
                break;
            }
        }

        // F8：移動量・回転量の刻み幅の切り替え
        if(input->IsKeyPressed(Tsukino::Input::KeyCode::F8)) {
            debugState->stepIndex = (debugState->stepIndex + 1) % 3;
        }

        // F10：攻撃ポーズの固定（プレイヤーのアニメーション再生を止め、武器をisAttacking扱いにする）
        if(input->IsKeyPressed(Tsukino::Input::KeyCode::F10)) {
            debugState->poseFrozen = !debugState->poseFrozen;
            if(registry.HasComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(playerEntity)) {
                registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(playerEntity).is_playing = !debugState->poseFrozen;
            }
        }

        //-------------------------------------------------------------
        // 固定中：本Systemより先に実行されるPlayerAnimationSystemが確定させたisAttacking/
        // attackTimerを上書きし、攻撃ウォッチドッグ（attackTimeoutSafety）による強制終了で
        // ポーズが崩れないようにする
        //-------------------------------------------------------------
        if(debugState->poseFrozen) {
            weapon.isAttacking = true;
            weapon.attackBlend = 1.0f;    // 調整中はブレンドの立ち上がりを待たず即座にアタッチ姿勢を見せる

            if(registry.HasComponent<PlayerAnimationSetComponent>(playerEntity)) {
                registry.GetComponent<PlayerAnimationSetComponent>(playerEntity).attackTimer = 0.0f;
            }

            // F11：固定中のみ、1/30秒分だけ手動でアニメーションを進める。
            // 本Systemの実行はAnimationSystemより後なので、ここでの変更は次フレームの
            // ボーン姿勢計算から反映される（1コマ送り）
            if(input->IsKeyPressed(Tsukino::Input::KeyCode::F11)
               && registry.HasComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(playerEntity)) {
                registry.GetComponent<Tsukino::BuiltIn::ECS::AnimationPlayerComponent>(playerEntity).elapsed_time += kPoseFreezeStepSeconds;
            }
        }

        //-------------------------------------------------------------
        // IJKLUOキーで現在の編集対象を微調整する（J/L=X, I/K=Y, U/O=Z）
        //-------------------------------------------------------------
        float positionStep = kPositionSteps[debugState->stepIndex];
        float angleStepRad = kAngleStepsDeg[debugState->stepIndex] * (3.14159265f / 180.0f);

        if(debugState->editTarget == WeaponGripEditTarget::GripRotation) {
            hlslpp::quaternion delta(0.0f, 0.0f, 0.0f, 1.0f);
            bool                changed = false;
            if(input->IsKeyPressed(Tsukino::Input::KeyCode::L)) {
                delta   = hlslpp::quaternion::rotation_x(angleStepRad);
                changed = true;
            }
            if(input->IsKeyPressed(Tsukino::Input::KeyCode::J)) {
                delta   = hlslpp::quaternion::rotation_x(-angleStepRad);
                changed = true;
            }
            if(input->IsKeyPressed(Tsukino::Input::KeyCode::I)) {
                delta   = hlslpp::quaternion::rotation_y(angleStepRad);
                changed = true;
            }
            if(input->IsKeyPressed(Tsukino::Input::KeyCode::K)) {
                delta   = hlslpp::quaternion::rotation_y(-angleStepRad);
                changed = true;
            }
            if(input->IsKeyPressed(Tsukino::Input::KeyCode::O)) {
                delta   = hlslpp::quaternion::rotation_z(angleStepRad);
                changed = true;
            }
            if(input->IsKeyPressed(Tsukino::Input::KeyCode::U)) {
                delta   = hlslpp::quaternion::rotation_z(-angleStepRad);
                changed = true;
            }

            if(changed) {
                // 既存のオフセット（先に確立された姿勢）を左に置き、deltaは「その上でのローカル軸
                // 補正」として右に置く。CombatSystem.cppのgripRotOffset合成と同じ規約
                weapon.attackGripRotationOffset = hlslpp::mul(weapon.attackGripRotationOffset, delta);
            }
        } else {
            hlslpp::float3* target = ResolveVectorTarget(weapon, debugState->editTarget);
            if(target) {
                if(input->IsKeyPressed(Tsukino::Input::KeyCode::L))
                    target->x += positionStep;
                if(input->IsKeyPressed(Tsukino::Input::KeyCode::J))
                    target->x -= positionStep;
                if(input->IsKeyPressed(Tsukino::Input::KeyCode::I))
                    target->y += positionStep;
                if(input->IsKeyPressed(Tsukino::Input::KeyCode::K))
                    target->y -= positionStep;
                if(input->IsKeyPressed(Tsukino::Input::KeyCode::O))
                    target->z += positionStep;
                if(input->IsKeyPressed(Tsukino::Input::KeyCode::U))
                    target->z -= positionStep;
            }
        }

        //-------------------------------------------------------------
        // F9：現在値と武器メッシュのAABB（同じローカル空間）をログへ出力する。
        // そのままActionGameScene.cppへ貼り付けられる形式にしてある
        //-------------------------------------------------------------
        if(input->IsKeyPressed(Tsukino::Input::KeyCode::F9)) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(3);
            oss << "weapon.gripPointLocal = hlslpp::float3(" << weapon.gripPointLocal.x << "f, " << weapon.gripPointLocal.y << "f, "
                << weapon.gripPointLocal.z << "f);\n";
            oss << "weapon.attackLocalOffset = hlslpp::float3(" << weapon.attackLocalOffset.x << "f, " << weapon.attackLocalOffset.y << "f, "
                << weapon.attackLocalOffset.z << "f);\n";
            oss << "weapon.attackGripRotationOffset = hlslpp::quaternion(" << weapon.attackGripRotationOffset.x << "f, "
                << weapon.attackGripRotationOffset.y << "f, " << weapon.attackGripRotationOffset.z << "f, " << weapon.attackGripRotationOffset.w << "f);";

            if(ctx->assetManager && registry.HasComponent<Tsukino::BuiltIn::ECS::ModelComponent>(player->weaponEntity)) {
                auto& modelComp = registry.GetComponent<Tsukino::BuiltIn::ECS::ModelComponent>(player->weaponEntity);
                auto  asset      = ctx->assetManager->Get(modelComp.modelHandle);
                if(asset && asset->GetType() == Tsukino::Asset::AssetType::Model) {
                    auto           modelAsset = std::static_pointer_cast<Tsukino::Asset::ModelAsset>(asset);
                    hlslpp::float3 aabbMin, aabbMax;
                    if(ComputeWeaponLocalAABB(modelAsset->modelData, aabbMin, aabbMax)) {
                        oss << "\n// AABB(local): min=(" << aabbMin.x << ", " << aabbMin.y << ", " << aabbMin.z << ") max=(" << aabbMax.x << ", "
                            << aabbMax.y << ", " << aabbMax.z << ")";
                    }
                }
            }

            Tsukino::Core::Log::Info(oss.str());
        }

        //-------------------------------------------------------------
        // HUDテキストの更新
        //-------------------------------------------------------------
        {
            std::wostringstream wss;
            wss << std::fixed << std::setprecision(2);
            wss << L"[武器グリップ調整] F6終了 / F7対象切替 / F8刻み / F9ログ出力 / F10ポーズ固定" << (debugState->poseFrozen ? L"(中)" : L"")
                << L" / F11で1コマ送り\n";
            wss << L"対象: " << EditTargetName(debugState->editTarget) << L"  刻み: " << kPositionSteps[debugState->stepIndex] << L" / "
                << kAngleStepsDeg[debugState->stepIndex] << L"deg  (JL=X IK=Y UO=Z)\n";
            wss << L"GripPoint         = (" << weapon.gripPointLocal.x << L", " << weapon.gripPointLocal.y << L", " << weapon.gripPointLocal.z << L")\n";
            wss << L"AttackLocalOffset = (" << weapon.attackLocalOffset.x << L", " << weapon.attackLocalOffset.y << L", " << weapon.attackLocalOffset.z
                << L")\n";
            wss << L"GripRotation      = (" << weapon.attackGripRotationOffset.x << L", " << weapon.attackGripRotationOffset.y << L", "
                << weapon.attackGripRotationOffset.z << L", " << weapon.attackGripRotationOffset.w << L")";
            hudFont->text = wss.str();
        }
    }
}    // namespace ActionGame::ECS
