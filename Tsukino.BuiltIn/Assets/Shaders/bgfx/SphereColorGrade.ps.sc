$input v_texcoord0

//--------------------------------------------------------------
//! @file   SphereColorGrade.ps.sc
//! @brief  球形の色調補正パス用のピクセルシェーダー
//! @note   頂点シェーダーは Tonemap.vs を共用するフルスクリーン三角形。
//!         HDR バッファは読まず、深度だけを読む。結果は「描かれている色 × 乗算色 ＋ 加算色」。
//!         2つの色を同時に出すブレンド（デュアルソース）が使えない描画 API があるので、同じシェーダーを2回描く
//!         （1回目は乗算色を出して掛け、2回目は加算色を出して足す。gradeEdgeParams.y で切り替える）。
//!         球の外側は (加算 0, 乗算 1) なので何も変わらない
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "TextureStages.sh"

// 色調補正のパラメータ（C++ の CBufferSphereColorGrade）
#define TSUKINO_SPHERE_COLOR_GRADE_VEC4_COUNT 5    // sizeof(CBufferSphereColorGrade) / 16

uniform vec4 u_sphereColorGrade[TSUKINO_SPHERE_COLOR_GRADE_VEC4_COUNT];

#define gradeSphere     u_sphereColorGrade[0]    // xyz: 中心, w: 半径
#define gradeTint       u_sphereColorGrade[1]    // xyz: 乗算色, w: 全体の効き
#define gradeFogColor   u_sphereColorGrade[2]    // xyz: 霧の色, w: 霧の濃さ
#define gradeEdgeColor  u_sphereColorGrade[3]    // xyz: 境界の光の色, w: 光の帯の幅
#define gradeEdgeParams u_sphereColorGrade[4]    // x: 境界のぼかし幅, y: 出す色（0 = 乗算色, 1 = 加算色。SphereColorGradePass が入れる）

// 深度（ライティングと同じステージ。点で読む）
SAMPLER2D(s_gbufferDepth, STAGE_GBUFFER_DEPTH);

void main()
{
    vec2 ndc      = v_texcoord0 * 2.0 - 1.0;
    vec2 screenUV = vec2(v_texcoord0.x, 1.0 - v_texcoord0.y);

    float depth = texture2D(s_gbufferDepth, screenUV).r;

    // リバース Z：深度 0 = 何も描かれていない背景（空）
    bool isBackground = (depth <= 0.0);

    vec4 worldPos = mul(vec4(ndc, depth, 1.0), invViewProj);
    worldPos /= worldPos.w;

    vec3  center   = gradeSphere.xyz;
    float radius   = gradeSphere.w;
    float softness = max(gradeEdgeParams.x, 1.0e-3);

    //----------------------------------------------------------
    // 画素から中心までの距離。空には位置が無いので、カメラが球の内側に入ったら
    // 空も内側とみなす（カメラ位置の距離で判定する）
    //----------------------------------------------------------
    float distanceToCenter = isBackground ? distance(cameraPos.xyz, center) : distance(worldPos.xyz, center);

    // 内側ほど1。境界は softness の幅でなめらかに切り替える
    float inside = 1.0 - smoothstep(radius - softness, radius, distanceToCenter);
    inside *= saturate(gradeTint.w);

    //----------------------------------------------------------
    // 内側：色を掛けてから霧の色へ寄せる。
    // 霧はカメラからの距離で濃くなる（空は最も濃い扱い）
    //----------------------------------------------------------
    float viewDistance = isBackground ? 1.0e6 : distance(worldPos.xyz, cameraPos.xyz);
    float fogAmount    = 1.0 - exp(-gradeFogColor.w * viewDistance);

    vec3 insideMultiply = gradeTint.rgb * (1.0 - fogAmount);
    vec3 insideAdd      = gradeFogColor.rgb * fogAmount;

    //----------------------------------------------------------
    // 境界の光：半径のすぐ外側に細い帯を足す（空には出さない）
    //----------------------------------------------------------
    float edgeWidth = max(gradeEdgeColor.w, 1.0e-3);
    float edge      = isBackground ? 0.0 : saturate(1.0 - abs(distanceToCenter - radius) / edgeWidth);
    edge *= saturate(gradeTint.w);

    vec3 multiplyColor = mix(vec3_splat(1.0), insideMultiply, inside);
    vec3 addColor      = insideAdd * inside + gradeEdgeColor.rgb * edge * edge;

    gl_FragColor = (gradeEdgeParams.y > 0.5) ? vec4(addColor, 0.0) : vec4(multiplyColor, 1.0);
}
