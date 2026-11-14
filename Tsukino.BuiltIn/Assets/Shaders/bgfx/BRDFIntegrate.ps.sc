$input v_texcoord0

//--------------------------------------------------------------
//! @file   BRDFIntegrate.ps.sc
//! @brief  IBL ベイク：split-sum 近似の BRDF 積分 LUT 生成（BRDFIntegrate.ps.hlsl の bgfx 版）
//! @note   頂点シェーダーは Tonemap.vs を共用するフルスクリーン三角形。
//!         スカイに依存しないので、起動時に一度だけ描けばよい。
//!         入力 uv.x = NdotV、uv.y = roughness として、出力 rg = (スケール A, バイアス B) を書く
//!         （Karis, "Real Shading in Unreal Engine 4" の split-sum 近似）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "IBLBake.sh"

//--------------------------------------------------------------
//! @brief  IBL 用 Smith 幾何減衰（k = roughness^2 / 2。直接照明用の G_Smith とは k の式が異なる）
//--------------------------------------------------------------
float GeometrySchlickGGX_IBL(float ndotX, float rough)
{
    float a = rough * rough;
    float k = (a * a) / 2.0;
    return ndotX / (ndotX * (1.0 - k) + k);
}

float GeometrySmith_IBL(float ndotV, float ndotL, float rough)
{
    return GeometrySchlickGGX_IBL(ndotV, rough) * GeometrySchlickGGX_IBL(ndotL, rough);
}

//--------------------------------------------------------------
//! @brief  (NdotV, roughness) 1点ぶんの split-sum 積分
//--------------------------------------------------------------
vec2 IntegrateBRDF(float ndotV, float rough)
{
    vec3 V = vec3(sqrt(1.0 - ndotV * ndotV), 0.0, ndotV);

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    for(uint i = 0u; i < 1024u; ++i) {
        vec2 xi = Hammersley(i, 1024u);
        vec3 H  = ImportanceSampleGGX(xi, N, rough);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float ndotL = max(L.z, 0.0);
        float ndotH = max(H.z, 0.0);
        float vdotH = max(dot(V, H), 0.0);

        if(ndotL > 0.0) {
            float g    = GeometrySmith_IBL(ndotV, ndotL, rough);
            float gVis = (g * vdotH) / max(ndotH * ndotV, 1e-4);
            float fc   = pow(1.0 - vdotH, 5.0);

            A += (1.0 - fc) * gVis;
            B += fc * gVis;
        }
    }

    return vec2(A, B) / 1024.0;
}

void main()
{
    float ndotV = saturate(v_texcoord0.x) + 1e-4;
    float rough = saturate(v_texcoord0.y);

    gl_FragColor = vec4(IntegrateBRDF(ndotV, rough), 0.0, 1.0);
}
