//--------------------------------------------------------------
//! @file   Matrix.hpp
//! @brief  行列クラスの定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <hlsl++.h>
//--------------------------------------------------------------
//! @class  matrix
//! @brief  行列クラス
//--------------------------------------------------------------
class matrix : public hlslpp::float4x4 {
public:
    //--------------------------------------------------------------
    //@brief  継承コンストラクタ
    //--------------------------------------------------------------
    using float4x4::float4x4;

    //--------------------------------------------------------------
    //@brief  コンストラクタ
    //--------------------------------------------------------------
    matrix(const float4x4& m)
        : float4x4(m) {}

    //----------------------------------------------------------
    //! @name   行列作成
    //----------------------------------------------------------
    //@{

    //--------------------------------------------------------------
    //@brief  単位行列
    //--------------------------------------------------------------
    static [[nodiscard]] matrix identity();

    //--------------------------------------------------------------
    // 平行移動行列
    //! @param  [in]    v   移動ベクトル
    //--------------------------------------------------------------
    static [[nodiscard]] matrix translate(const hlslpp::float3& v);
    static [[nodiscard]] matrix translate(float x, float y, float z);

    //--------------------------------------------------------------
    // スケール行列
    //! @param  [in]    s   スケール値
    //--------------------------------------------------------------
    static [[nodiscard]] matrix scale(const hlslpp::float3& s);
    static [[nodiscard]] matrix scale(float sx, float sy, float sz);
    static [[nodiscard]] matrix scale(float s);

    //--------------------------------------------------------------
    // X軸中心の回転行列
    //! @param  [in]    radian  回転角度
    //! @see https://ja.wikipedia.org/wiki/%E5%9B%9E%E8%BB%A2%E8%A1%8C%E5%88%97
    //--------------------------------------------------------------
    static [[nodiscard]] matrix rotateX(float radian);

    //--------------------------------------------------------------
    // Y軸中心の回転行列
    //! @param  [in]    radian  回転角度
    //--------------------------------------------------------------
    static [[nodiscard]] matrix rotateY(float radian);

    //--------------------------------------------------------------
    // Z軸中心の回転行列
    //! @param  [in]    radian  回転角度
    //--------------------------------------------------------------
    static [[nodiscard]] matrix rotateZ(float radian);

    //--------------------------------------------------------------
    // 任意軸中心の回転行列
    //! @param  [in]    axis    回転の中心軸
    //! @param  [in]    radian  回転角度
    //--------------------------------------------------------------
    static [[nodiscard]] matrix rotateAxis(const hlslpp::float3& axis, float radian);

    //--------------------------------------------------------------
    // [左手座標系] ビュー行列
    //! @param  [in]    eye         視点座標
    //! @param  [in]    look_at     注視点
    //! @param  [in]    world_up    世界の上方向のベクトル(default:(0.0f, 1.0f, 0.0f))
    //--------------------------------------------------------------
    static [[nodiscard]] matrix lookAtLH(const hlslpp::float3& eye,
                                         const hlslpp::float3& look_at,
                                         const hlslpp::float3& world_up = hlslpp::float3(0.0f, 1.0f, 0.0f));

    //--------------------------------------------------------------
    // [左手座標系] 投影行列
    //! @param  [in]    fovy            画角(単位:radian)
    //! @param  [in]    aspect_ratio    アスペクト比
    //! @param  [in]    near_z          近クリップZ値
    //! @param  [in]    far_z           遠クリップZ値
    //! @note InverseZにしたい場合はnearZの値とfarZの値を交換して指定。
    //--------------------------------------------------------------
    static [[nodiscard]] matrix perspectiveFovLH(float fovy, float aspect_ratio, float near_z, float far_z);

    //--------------------------------------------------------------
    // [左手座標系] 無限遠投影行列
    //!
    //! 遠クリップ面を廃止して無限遠まで描画可能にする行列。
    //! 浮動小数点の丸め誤差を低減。Z=0.0fで描画すると無限遠。
    //!
    //! @param  [in]    fovy         画角(単位:radian)
    //! @param  [in]    aspect_ratio アスペクト比
    //! @param  [in]    near_z       近クリップZ値
    //!
    //! @see GDC'07 「Projection Matrix Tricks」
    //! @attention InverseZ前提の投影にになるため注意。
    //--------------------------------------------------------------
    static [[nodiscard]] matrix perspectiveFovInfiniteFarPlaneLH(float fovy, float aspect_ratio, float near_z);

    //--------------------------------------------------------------
    // [左手座標系] 平行投影行列
    //! @param  [in]    left        左側の幅
    //! @param  [in]    right       右側の幅
    //! @param  [in]    bottom      下側の幅
    //! @param  [in]    top         上側の幅
    //! @param  [in]    near_z      近クリップZ値
    //! @param  [in]    far_z       遠クリップZ値
    //! @note InverseZにしたい場合はnearZの値とfarZの値を交換して指定。
    //--------------------------------------------------------------
    static [[nodiscard]] matrix orthographicOffCenterLH(float left, float right, float bottom, float top, float near_z, float far_z);
    //@}
};
