//----------------------------------------------------------------------------
//! @file   PhysicsWorld.hpp
//! @brief  物理シミュレーションのファサード
//! @detail Jolt Physics をこのクラスの内側に完全に閉じ込めます。ヘッダには
//!         Jolt の型が一切現れないため、上位モジュールは Jolt をインクルード
//!         せずに物理を扱えます。ECS は関知せず、エンティティの同一性は
//!         uint64_t のユーザーデータとして受け渡します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Physics/BodyHandle.hpp>
#include <Tsukino/Physics/PhysicsTypes.hpp>

#include <hlsl++.h>

#include <cstdint>
#include <vector>

// 名前空間 : Tsukino::Physics
namespace Tsukino::Physics {

    class IPhysicsDebugDraw;    // 前方宣言

    //------------------------------------------------------------------------
    //! 物理ワールド
    //------------------------------------------------------------------------
    class PhysicsWorld {
    public:
        //! コンストラクタ
        PhysicsWorld();

        //! デストラクタ
        ~PhysicsWorld();

        PhysicsWorld(const PhysicsWorld&)            = delete;
        PhysicsWorld& operator=(const PhysicsWorld&) = delete;

        //--------------------------------------------------------------------
        // ボディの生成と破棄
        //--------------------------------------------------------------------

        //! ボディを生成してワールドへ追加します。
        //! @param  [in] desc          生成内容
        //! @param  [in] shapeCacheKey ハイトフィールド形状の使い回しに使うキー（同じキーなら形状を再利用する）
        //! @return 生成されたボディのハンドル。失敗した場合は無効なハンドル
        BodyHandle CreateBody(const BodyDesc& desc, uint64_t shapeCacheKey = 0);

        //! ボディをワールドから取り除いて破棄します。
        //! @param  [in] handle 破棄するボディ
        void DestroyBody(BodyHandle handle);

        //! キャッシュ済みのハイトフィールド形状を解放します。
        //! @param  [in] shapeCacheKey CreateBody() に渡したキー
        void ForgetShapeCache(uint64_t shapeCacheKey);

        //--------------------------------------------------------------------
        // ボディへの書き込み
        //--------------------------------------------------------------------

        //! ボディの位置と向きを直接設定します。
        //! @param  [in] handle   対象のボディ
        //! @param  [in] position 設定する位置
        //! @param  [in] rotation 設定する向き
        void SetPositionAndRotation(BodyHandle handle, const hlslpp::float3& position, const hlslpp::quaternion& rotation);

        //! Kinematic ボディを、次の Step() の終わりに目標の位置と向きへ着くように動かします。
        //! @param  [in] handle         対象のボディ（Kinematic）
        //! @param  [in] targetPosition 到達させる位置
        //! @param  [in] targetRotation 到達させる向き
        //! @param  [in] deltaTime      次の Step() に渡す経過時間（秒）
        //! @note   SetPositionAndRotation() と違い、移動を速度として扱うので、途中にある Dynamic ボディを接触として押す。
        //!         瞬間移動させると相手にめり込んだ状態から押し出しが始まり、薄い物は押し出す向きが乱れて
        //!         Kinematic の下や裏へ抜けてしまう。
        void MoveKinematic(BodyHandle handle, const hlslpp::float3& targetPosition, const hlslpp::quaternion& targetRotation, float deltaTime);

        //! ボディの並進速度を設定します。
        //! @param  [in] handle   対象のボディ
        //! @param  [in] velocity 設定する速度
        void SetLinearVelocity(BodyHandle handle, const hlslpp::float3& velocity);

        //! ボディへ撃力を加えます。
        //! @param  [in] handle  対象のボディ
        //! @param  [in] impulse 加える撃力
        void AddImpulse(BodyHandle handle, const hlslpp::float3& impulse);

        //! ボディへ角撃力を加えます。
        //! @param  [in] handle         対象のボディ
        //! @param  [in] angularImpulse 加える角撃力
        void AddAngularImpulse(BodyHandle handle, const hlslpp::float3& angularImpulse);

        //! ボディへ力を加えます。
        //! @param  [in] handle 対象のボディ
        //! @param  [in] force  加える力
        void AddForce(BodyHandle handle, const hlslpp::float3& force);

        //! ボディへトルクを加えます。
        //! @param  [in] handle 対象のボディ
        //! @param  [in] torque 加えるトルク
        void AddTorque(BodyHandle handle, const hlslpp::float3& torque);

        //! ボディの運動タイプを変更します。
        //! @param  [in] handle 対象のボディ
        //! @param  [in] motion 設定する運動タイプ
        void SetMotionType(BodyHandle handle, MotionType motion);

        //! 移動・回転の許可軸を変更します。
        //! @param  [in] handle 対象のボディ
        //! @param  [in] dofs   許可する軸
        //! @param  [in] mass   変更後も維持したい質量
        //! @note   Dynamic 以外のボディでは何もしません。凍結した軸の残存速度はゼロにします
        void SetAllowedDofs(BodyHandle handle, DofMask dofs, float mass);

        //--------------------------------------------------------------------
        // ボディからの読み出し
        //--------------------------------------------------------------------

        //! ボディの運動タイプを取得します。
        //! @param  [in] handle 対象のボディ
        //! @return 現在の運動タイプ
        MotionType GetMotionType(BodyHandle handle) const;

        //! ボディの位置・向き・速度をまとめて取得します。
        //! @param  [in] handle 対象のボディ
        //! @return 現在値
        BodyState GetBodyState(BodyHandle handle) const;

        //! ワールドの重力加速度を取得します。
        //! @return 重力加速度
        hlslpp::float3 GetGravity() const;

        //--------------------------------------------------------------------
        // ワールドの設定
        //--------------------------------------------------------------------

        //! 1メートルが何単位かを設定し、重力と接触判定の許容値をその長さに合わせます。
        //! @param  [in] unitsPerMeter 1メートルあたりの単位数（1unit=1cm なら 100）。既定は 1
        //! @note   Jolt の既定値（重力 9.81、接触を作り始める距離 2cm など）はメートル単位を前提にしている。
        //!         cm 単位のまま使うと重力が 1/100 になり、接触が 1/100 の距離まで近づかないと作られないため、
        //!         薄い物や速い物が床やほかの物にめり込みやすい。
        //!         速度のしきい値（スリープ判定・反発の最低速度）と重力もこの値で拡大する。
        //!         呼ぶたびに既定値から計算し直すので、何度呼んでも倍率は累積しない。
        //!         拡大した許容値（めり込みを直さずに許す量・接触を作り始める距離は 2cm 相当）は、
        //!         薄い物（厚み1cm前後）を積むゲームでは大きすぎ、上の物が沈んで見える。
        //!         その場合はこの後で SetContactTolerances を呼んで小さくする
        void SetUnitsPerMeter(float unitsPerMeter);

        //! 接触判定の許容値を、ワールドの長さの単位で直接設定します。
        //! @param  [in] penetrationSlop            めり込みを直さずに許す量。0 以下なら変えない
        //! @param  [in] speculativeContactDistance 離れていても接触を作り始める距離。0 以下なら変えない
        //! @note   SetUnitsPerMeter は許容値を既定値から作り直すので、SetUnitsPerMeter の後に呼ぶこと。
        //!         めり込みの許容値を物の厚みより小さくすると、積んだ物が沈まなくなる。
        //!         接触を作り始める距離を小さくしすぎると、速い物が薄い物をすり抜けやすくなる
        void SetContactTolerances(float penetrationSlop, float speculativeContactDistance);

        //--------------------------------------------------------------------
        // シミュレーション
        //--------------------------------------------------------------------

        //! 物理シミュレーションを1ステップ進めます。
        //! @param  [in] deltaTime 進める時間（秒）
        void Step(float deltaTime);

        //--------------------------------------------------------------------
        // 形状クエリ
        //--------------------------------------------------------------------

        //! 指定のカプセル形状と現在重なっている全ボディのユーザーデータを取得します。
        //! @param  [in] center     カプセル中心のワールド座標
        //! @param  [in] rotation   カプセルの向き（内部のカプセルはローカルY軸方向が軸）
        //! @param  [in] radius     カプセル半径
        //! @param  [in] halfHeight カプセル円柱部分の半分の高さ
        //! @return 重なっているボディのユーザーデータの一覧
        //! @note   センサー的な即時オーバーラップ判定であり、物理的な反発は起きません
        std::vector<uint64_t> OverlapCapsule(const hlslpp::float3&     center,
                                             const hlslpp::quaternion& rotation,
                                             float                     radius,
                                             float                     halfHeight) const;

        //! 指定の直方体と重なっているボディがあるかどうかを調べます。
        //! @param  [in] center     直方体中心のワールド座標
        //! @param  [in] halfExtent 直方体の各軸の半分サイズ
        //! @param  [in] ignore     判定から除外するボディ
        //! @return 1つでも重なっていれば true
        bool OverlapBox(const hlslpp::float3& center, const hlslpp::float3& halfExtent, BodyHandle ignore) const;

        //--------------------------------------------------------------------
        // キャラクターコントローラー
        //--------------------------------------------------------------------

        //! キャラクターコントローラーを生成します。
        //! @param  [in] desc 生成内容
        //! @return 生成されたキャラクターのハンドル
        CharacterHandle CreateCharacter(const CharacterDesc& desc);

        //! キャラクターコントローラーを破棄します。
        //! @param  [in] handle 破棄するキャラクター
        void DestroyCharacter(CharacterHandle handle);

        //! キャラクターが接地しているかどうかを返します。
        //! @param  [in] handle 対象のキャラクター
        //! @return 接地していれば true
        bool IsCharacterSupported(CharacterHandle handle) const;

        //! キャラクターを1ステップ進めます。
        //! @param  [in]  handle    対象のキャラクター
        //! @param  [in]  input     今フレームの入力
        //! @param  [out] output    更新後の位置・向き・接地状態
        //! @param  [in]  deltaTime 進める時間（秒）
        //! @return 更新できたら true（ハンドルが無効なら false）
        bool StepCharacter(CharacterHandle handle, const CharacterInput& input, CharacterOutput& output, float deltaTime);

        //--------------------------------------------------------------------
        // 接触
        //--------------------------------------------------------------------

        //! 直前の Step() で溜まった接触を取り出して空にします。
        //! @param  [out] out 取り出し先。呼び出し前の内容は破棄されます
        //! @note   Step() から戻った後、メインスレッドから呼んでください
        void DrainContacts(std::vector<ContactRecord>& out);

        //--------------------------------------------------------------------
        // デバッグ描画
        //--------------------------------------------------------------------

        //! ボディの形状をワイヤーフレームで描画します。
        //! @param  [in,out] sink   描画の出力先
        //! @param  [in]     handle 対象のボディ
        void DebugDrawBody(IPhysicsDebugDraw& sink, BodyHandle handle) const;

        //! 生存している全キャラクターの形状をワイヤーフレームで描画します。
        //! @param  [in,out] sink 描画の出力先
        //! @note   接地しているキャラクターは緑、していないものは黄で描きます
        void DebugDrawCharacters(IPhysicsDebugDraw& sink) const;

    private:
        struct Impl;
        Impl* m_impl;
    };

}    // namespace Tsukino::Physics
