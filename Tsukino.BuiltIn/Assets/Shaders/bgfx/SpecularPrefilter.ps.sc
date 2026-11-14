$input v_texcoord0

//--------------------------------------------------------------
//! @file   SpecularPrefilter.ps.sc
//! @brief  IBL ベイク：キャプチャキューブからプレフィルタ済みスペキュラキューブへの畳み込み（SpecularPrefilter.ps.hlsl の bgfx 版）
//! @note   頂点シェーダーは Tonemap.vs を共用するフルスクリーン三角形。面×ミップの回数だけ描く。
//!         出力先の面はシーン定数の invViewProj が、ラフネスとサンプル数は u_iblBake が決める
//!         （どちらもミップ・面ごとに C++ 側が送る）。
//!         split-sum 近似（Karis, "Real Shading in Unreal Engine 4"）のプレフィルタ項
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "IBLBake.sh"
#include "TextureStages.sh"

// キャプチャキューブ（大気散乱スカイを6面へ焼いたもの）
SAMPLERCUBE(s_iblCaptureSource, STAGE_IBL_CAPTURE_SOURCE);

// このミップのベイクパラメータ（DX11 版の b11）。x: ラフネス, y: サンプル数（float で届く）
uniform vec4 u_iblBake;
#define bakeRoughness   u_iblBake.x
#define bakeSampleCount uint(u_iblBake.y + 0.5)

void main()
{
    //----------------------------------------------------------
    // このテクセルが担当する方向を復元する（IrradianceConvolve.ps.sc と同じ手法）。
    // split-sum 近似の慣例に従い N = V = R とする
    //----------------------------------------------------------
    vec2 ndc    = v_texcoord0 * 2.0 - 1.0;
    vec4 farPos = mul(vec4(ndc, 0.0, 1.0), invViewProj);
    farPos /= farPos.w;
    vec3 N = normalize(farPos.xyz - cameraPos.xyz);
    vec3 V = N;

    vec3  prefilteredColor = vec3_splat(0.0);
    float totalWeight      = 0.0;

    uint sampleCount = bakeSampleCount;
    for(uint i = 0u; i < sampleCount; ++i) {
        vec2 xi = Hammersley(i, sampleCount);
        vec3 H  = ImportanceSampleGGX(xi, N, bakeRoughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = saturate(dot(N, L));
        if(NdotL > 0.0) {
            prefilteredColor += textureCubeLod(s_iblCaptureSource, L, 0.0).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor /= max(totalWeight, 1e-4);

    gl_FragColor = vec4(prefilteredColor, 1.0);
}
