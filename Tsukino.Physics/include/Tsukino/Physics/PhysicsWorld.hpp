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
