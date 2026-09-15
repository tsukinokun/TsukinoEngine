//--------------------------------------------------------------
//! @file   IrradianceConvolve.ps.hlsl
//! @brief  IBLベイク：キャプチャキューブから拡散irradianceキューブへの畳み込み
//! @author 山﨑愛
//! @note   VSはTonemap.vs.hlslを共用するフルスクリーン三角形。
//!         Renderer::ExecuteIBLIrradiancePass()が面ごとに計6回描画する。
//!         出力先の面はCBufferScene(b0)のinvViewProjが決める
//!         （Sky.ps.hlslのワールド方向再構成と同じ手法。面ごとに90°FOVの
//!         ビュー/プロジェクション行列を差し替えたb0を転送してから呼ぶ。IBLBaker参照）。
//--------------------------------------------------------------
#pragma pack_matrix(row_major)
#include "PBR.hlsli"

//--------------------------------------------------------------
//! @brief キャプチャキューブ（大気散乱スカイを6面へ焼いたもの）
//--------------------------------------------------------------
TextureCube  iblCaptureSource : register(t20);
SamplerState iblBakeSampler : register(s10);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief 出力面の1テクセルぶんのコサイン重み付き半球積分
//! @note  積分区間をphi(方位角)×theta(天頂角)の固定グリッドでサンプリングする。
//!        拡散irradianceは低周波信号なので、32×8=256サンプル/テクセルで十分。
//!        /PIはここで済ませておき、消費側(IBL.hlsli)はalbedoを掛けるだけでよい。
//--------------------------------------------------------------
float4 PSMain(PSInput input) : SV_TARGET
{
    //----------------------------------------------------------
    // このテクセルが担当する方向Nを、Sky.ps.hlslと同じ手法で復元する。
    // cameraPosはキャプチャ用に原点固定なので、worldPos.xyzがそのまま方向になる
    //----------------------------------------------------------
    float2 ndc      = input.uv * 2.0f - 1.0f;
    float4 worldPos = mul(float4(ndc, 0.0f, 1.0f), invViewProj);
    worldPos /= worldPos.w;
    float3 N = normalize(worldPos.xyz - cameraPos.xyz);

    //----------------------------------------------------------
    // Nを中心とした接空間基底を作る
    //----------------------------------------------------------
    float3 up    = (abs(N.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    float3 right = normalize(cross(up, N));
    up           = cross(N, right);

    static const uint PHI_SAMPLES   = 32;
    static const uint THETA_SAMPLES = 8;

    float3 irradiance = float3(0.0f, 0.0f, 0.0f);

    [loop]
    for(uint p = 0; p < PHI_SAMPLES; ++p) {
        float phi = (2.0f * PI) * (float(p) + 0.5f) / float(PHI_SAMPLES);

        [loop]
        for(uint t = 0; t < THETA_SAMPLES; ++t) {
            float theta = (0.5f * PI) * (float(t) + 0.5f) / float(THETA_SAMPLES);

            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleVec     = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += iblCaptureSource.SampleLevel(iblBakeSampler, sampleVec, 0.0f).rgb * cos(theta) * sin(theta);
        }
    }

    irradiance *= PI / float(PHI_SAMPLES * THETA_SAMPLES);

    return float4(irradiance, 1.0f);
}
