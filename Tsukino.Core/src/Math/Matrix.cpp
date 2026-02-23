//--------------------------------------------------------------
//! @file   Matrix.cpp
//! @brief  行列クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include "Tsukino/Core/Math/Matrix.hpp"
// 名前空間 : Tsukino::Core::Math
namespace Tsukino::Core::Math {
    //---------------------------------------------------------------------------
    //! 単位行列
    //---------------------------------------------------------------------------
    matrix matrix::identity() {
        hlslpp::float4 m[4]{
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    //---------------------------------------------------------------------------
    //!  平行移動行列
    //---------------------------------------------------------------------------
    matrix matrix::translate(const hlslpp::float3& v) {
        hlslpp::float4 m[4]{
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {v.x,  v.y,  v.z,  1.0f}
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    matrix matrix::translate(float x, float y, float z) {
        return translate(hlslpp::float3(x, y, z));
    }

    //---------------------------------------------------------------------------
    //! スケール行列
    //---------------------------------------------------------------------------
    matrix matrix::scale(const hlslpp::float3& s) {
        hlslpp::float4 m[4]{
            {s.x,  0.0f, 0.0f, 0.0f},
            {0.0f, s.y,  0.0f, 0.0f},
            {0.0f, 0.0f, s.z,  0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    matrix matrix::scale(float sx, float sy, float sz) {
        return scale(hlslpp::float3(sx, sy, sz));
    }

    matrix matrix::scale(float s) {
        return matrix::scale(hlslpp::float3(s, s, s));
    }

    //---------------------------------------------------------------------------
    //! X軸中心の回転行列
    //---------------------------------------------------------------------------
    matrix matrix::rotateX(float radian) {
        float s = std::sinf(radian);
        float c = std::cosf(radian);

        hlslpp::float4 m[4]{
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, c,    s,    0.0f},
            {0.0f, -s,   c,    0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    //---------------------------------------------------------------------------
    //! Y軸中心の回転行列
    //---------------------------------------------------------------------------
    matrix matrix::rotateY(float radian) {
        float s = std::sinf(radian);
        float c = std::cosf(radian);

        hlslpp::float4 m[4]{
            {c,    0.0f, -s,   0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {s,    0.0f, c,    0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    //---------------------------------------------------------------------------
    //! Z軸中心の回転行列
    //---------------------------------------------------------------------------
    matrix matrix::rotateZ(float radian) {
        float s = std::sinf(radian);
        float c = std::cosf(radian);

        hlslpp::float4 m[4]{
            {c,    s,    0.0f, 0.0f},
            {-s,   c,    0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    //---------------------------------------------------------------------------
    //! 任意軸中心の回転行列
    //---------------------------------------------------------------------------
    matrix matrix::rotateAxis(const hlslpp::float3& axis, float radian) {
        float s    = std::sinf(radian);
        float c    = std::cosf(radian);
        float invc = 1.0f - c;

        hlslpp::float3 v = normalize(axis);
        float          x = v.x;
        float          y = v.y;
        float          z = v.z;

        hlslpp::float4 m[4]{
            {c + x * x * invc,     x * y * invc + z * s, z * x * invc - y * s, 0.0f},
            {x * y * invc - z * s, c + y * y * invc,     y * z * invc + x * s, 0.0f},
            {z * x * invc + y * s, y * z * invc - x * s, c + z * z * invc,     0.0f},
            {0.0f,                 0.0f,                 0.0f,                 1.0f}
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    //---------------------------------------------------------------------------
    //!  [左手座標系] ビュー行列
    //---------------------------------------------------------------------------
    matrix matrix::lookAtLH(const hlslpp::float3& eye, const hlslpp::float3& lookAt, const hlslpp::float3& worldUp) {
        hlslpp::float3 axis_z = normalize(lookAt - eye);
        hlslpp::float3 axis_x = normalize(cross(worldUp, axis_z));
        hlslpp::float3 axis_y = cross(axis_z, axis_x);

        float tx = -dot(axis_x, eye);
        float ty = -dot(axis_y, eye);
        float tz = -dot(axis_z, eye);

        hlslpp::float4 m[4]{
            {axis_x.x, axis_y.x, axis_z.x, 0.0f},
            {axis_x.y, axis_y.y, axis_z.y, 0.0f},
            {axis_x.z, axis_y.z, axis_z.z, 0.0f},
            {tx,       ty,       tz,       1.0f},
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    //---------------------------------------------------------------------------
    //! [左手座標系] 投影行列
    //---------------------------------------------------------------------------
    matrix matrix::perspectiveFovLH(float fovy, float aspect_ratio, float near_z, float far_z) {
        float s = std::sinf(fovy * 0.5f);
        float c = std::cosf(fovy * 0.5f);

        float height = c / s;
        float width  = height / aspect_ratio;
        float range  = far_z / (far_z - near_z);

        hlslpp::float4 m[4]{
            {width, 0.0f,   0.0f,            0.0f},
            {0.0f,  height, 0.0f,            0.0f},
            {0.0f,  0.0f,   range,           1.0f},
            {0.0f,  0.0f,   -range * near_z, 0.0f}
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    //---------------------------------------------------------------------------
    //! [左手座標系] 無限遠投影行列
    //---------------------------------------------------------------------------
    matrix matrix::perspectiveFovInfiniteFarPlaneLH(float fovy, float aspect_ratio, float near_z) {
        float s = std::sinf(fovy * 0.5f);
        float c = std::cosf(fovy * 0.5f);

        float height = c / s;
        float width  = height / aspect_ratio;

        hlslpp::float4 m[4]{
            {width, 0.0f,   0.0f,   0.0f},
            {0.0f,  height, 0.0f,   0.0f},
            {0.0f,  0.0f,   0.0f,   1.0f},
            {0.0f,  0.0f,   near_z, 0.0f}
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }

    //---------------------------------------------------------------------------
    //! [左手座標系] 平行投影行列
    //---------------------------------------------------------------------------
    matrix matrix::orthographicOffCenterLH(float left, float right, float bottom, float top, float near_z, float far_z) {
        float rcp_width  = 1.0f / (right - left);
        float rcp_height = 1.0f / (top - bottom);
        float range      = 1.0f / (far_z - near_z);

        hlslpp::float4 m[4]{
            {rcp_width * 2.0f,            0.0f,                         0.0f,            0.0f},
            {0.0f,                        rcp_height * 2.0f,            0.0f,            0.0f},
            {0.0f,                        0.0f,                         range,           0.0f},
            {-(left + right) * rcp_width, -(top + bottom) * rcp_height, -range * near_z, 1.0f}
        };
        return matrix(m[0], m[1], m[2], m[3]);
    }
}    // namespace Tsukino::Core::Math
