//-------------------------------------------------------------
//! @file   DiceFaceReadSystem.cpp
//! @brief  DiceFaceReadSystemクラスの実装
//! @author 山﨑愛
//-------------------------------------------------------------
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/System/DiceFaceReadSystem.hpp>
#include <Tsukino/Sandbox/LuckGameSampleScene/ECS/Component/DiceComponent.hpp>

#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>
#include <Tsukino/BuiltIn/ECS/Component/RigidbodyComponent.hpp>
#include <Tsukino/Core/Log.hpp>
#include <string>

#ifdef _DEBUG
#include <fstream>
#include <entt/entt.hpp>
#endif

// 名前空間 : LuckGameSampleScene::ECS
namespace LuckGameSampleScene::ECS {
    namespace {
        // ワールド空間での重力方向（上向き判定の基準）
        const hlslpp::float3 kUpDirection = hlslpp::float3(0.0f, 1.0f, 0.0f);

#ifdef _DEBUG
        //-------------------------------------------------------------
        // 【再検証用・一時コード】表示(見た目)とconfirmedValueの計算タイミングが
        // ズレていないかを確認するための、フレーム単位のダンプログ。
        // Settled中のサイコロについて、毎フレームの6面dot値・選ばれた面・
        // confirmedValueをそのままファイルへ書き出す（外部スクリーンショットのタイミング
        // ズレの影響を受けない、コード内部の生データを見るのが目的）。
        // 検証が終わったら削除すること。
        //-------------------------------------------------------------
        std::ofstream& DebugDumpStream() {
            static std::ofstream stream("C:\\Users\\tamami197508\\Desktop\\TsukinoEngine\\dice_face_debug.log", std::ios::trunc);
            return stream;
        }
#endif
    }

    //-------------------------------------------------------------
    //! @brief  システムの更新
    //-------------------------------------------------------------
    void DiceFaceReadSystem::Update(Tsukino::ECS::Registry& registry, float deltaTime) {
        (void)deltaTime;

        auto view = registry.View<Tsukino::BuiltIn::ECS::TransformComponent, Tsukino::BuiltIn::ECS::RigidbodyComponent, DiceComponent>();

        view.each([&](entt::entity entity, Tsukino::BuiltIn::ECS::TransformComponent& transform,
                       Tsukino::BuiltIn::ECS::RigidbodyComponent& rigidbody, DiceComponent& dice) {
            // 静止確定していないサイコロは何もしない
            if(dice.state != DiceRollState::Settled) {
                return;
            }

            // 位置フリーズ中（Hovering中にDiceDebugOverrideSystemでSettledへ強制された
            // だけのもの等）は、実際の物理姿勢とconfirmedValueが対応していないため、
            // 上書きしてデバッグ機能を壊さないよう対象外にする。
            if(dice.confirmed && rigidbody.freezePositionX) {
                return;
            }

            float bestDot   = -1.0f;
            u8    bestValue = dice.faceValue[0];
#ifdef _DEBUG
            float allDots[6] = {};
#endif

            for(int i = 0; i < 6; ++i) {
                // ローカル法線をワールド回転で変換し、上向きベクトルとの内積を見る
                // 注意: hlslpp::mul(quaternion, float3) は逆回転（inverse）を返す。
                // 正しい順方向回転は mul(float3, quaternion)（このプロジェクトの行列mul規約と一致）。
                // 逆にすると、Y軸に沿う面(1,6)は回転方向に依らず値が変わらないため偶然正しく見える一方、
                // X/Z軸の面(2,3,4,5)は誤判定になっていた。
                hlslpp::float3 worldNormal = hlslpp::mul(dice.faceNormal[i], transform.rotation);
                float          dotValue    = hlslpp::dot(worldNormal, kUpDirection).x;
#ifdef _DEBUG
                allDots[i] = dotValue;
#endif

                if(dotValue > bestDot) {
                    bestDot   = dotValue;
                    bestValue = dice.faceValue[i];
                }
            }

            // Settled中は毎フレーム読み直す。まだ転がっている他のサイコロにぶつかられたり、
            // 不安定な着地姿勢からゆっくり傾いたりして、静止確定後にわずかに姿勢が変わることが
            // あり、1回読んだきりだと表示上の出目と実際に見えている目がズレる不具合になっていた。
#ifdef _DEBUG
            bool changed = dice.confirmed && dice.confirmedValue != bestValue;
            if(changed) {
                Tsukino::Core::Log::Info("[LuckGameSampleScene] Dice face changed after settle: "
                                          + std::to_string(dice.confirmedValue) + " -> " + std::to_string(bestValue));
            }

            {
                std::ofstream& log = DebugDumpStream();
                log << "entity=" << static_cast<uint32_t>(entity) << " pos=(" << transform.position.x << "," << transform.position.y
                    << "," << transform.position.z << ")"
                    << " dots=[" << allDots[0] << "," << allDots[1] << "," << allDots[2] << "," << allDots[3] << "," << allDots[4] << ","
                    << allDots[5] << "]"
                    << " bestValue=" << static_cast<int>(bestValue) << " prevConfirmed=" << static_cast<int>(dice.confirmedValue)
                    << " changed=" << (changed ? 1 : 0) << " linVel=" << hlslpp::length(rigidbody.linearVelocity).x
                    << " angVel=" << hlslpp::length(rigidbody.angularVelocity).x << "\n";
                log.flush();
            }
#endif

            dice.confirmedValue = bestValue;
            dice.confirmed      = true;
        });
    }

}    // namespace LuckGameSampleScene::ECS
