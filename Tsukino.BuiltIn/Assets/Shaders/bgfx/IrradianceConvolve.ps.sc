$input v_texcoord0

//--------------------------------------------------------------
//! @file   IrradianceConvolve.ps.sc
//! @brief  IBL ベイク：キャプチャキューブから拡散 irradiance キューブへの畳み込み
//! @note   頂点シェーダーは Tonemap.vs を共用するフルスクリーン三角形。面ごとに計6回描く。
//!         出力先の面はシーン定数の invViewProj が決める（Sky.ps.sc のワールド方向の復元と同じ手法。
//!         C++ 側が面ごとに 90°FOV のビュー・プロジェクション行列へ差し替えたシーン定数を送る）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "IBLBake.sh"
#include "TextureStages.sh"

// キャプチャキューブ（大気散乱スカイを6面へ焼いたもの）
SAMPLERCUBE(s_iblCaptureSource, STAGE_IBL_CAPTURE_SOURCE);

//--------------------------------------------------------------
//! @note  出力面の1テクセルぶんの、コサイン重み付き半球積分。
//!        積分区間を phi（方位角）× theta（天頂角）の固定グリッドでサンプリングする。
//!        拡散 irradiance は低周波信号なので、32×8=256 サンプル/テクセルで十分。
//!        /PI はここで済ませておき、消費側（IBL.sh）は albedo を掛けるだけでよい
//--------------------------------------------------------------
void main()
{
    //----------------------------------------------------------
    // このテクセルが担当する方向 N を、Sky.ps.sc と同じ手法で復元する。
    // cameraPos はキャプチャ用に原点固定なので、farPos.xyz がそのまま方向になる
    //----------------------------------------------------------
    vec2 ndc    = v_texcoord0 * 2.0 - 1.0;
    vec4 farPos = mul(vec4(ndc, 0.0, 1.0), invViewProj);
    farPos /= farPos.w;
    vec3 N = normalize(farPos.xyz - cameraPos.xyz);

    // N を中心とした接空間基底を作る
    vec3 up    = (abs(N.z) < 0.999) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = cross(N, right);

    vec3 irradiance = vec3_splat(0.0);

    for(int p = 0; p < 32; ++p) {
        float phi = (2.0 * IBL_PI) * (float(p) + 0.5) / 32.0;

        for(int t = 0; t < 8; ++t) {
            float theta = (0.5 * IBL_PI) * (float(t) + 0.5) / 8.0;

            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec     = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += textureCubeLod(s_iblCaptureSource, sampleVec, 0.0).rgb * cos(theta) * sin(theta);
        }
    }

    irradiance *= IBL_PI / (32.0 * 8.0);

    gl_FragColor = vec4(irradiance, 1.0);
}
