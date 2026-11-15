$input v_texcoord0

//--------------------------------------------------------------
//! @file   Fog.ps.sc
//! @brief  フォグパス用のピクセルシェーダー（Fog.ps.hlsl の bgfx 版。距離フォグ＋高さフォグ＋ノイズ揺らぎ）
//! @note   頂点シェーダーは Tonemap.vs を共用するフルスクリーン三角形。
//!         HDR バッファは読まず、深度だけを読んで HDR へ直接 over 合成する。式は Fog.sh にある。
//!         出力はプリマルチプライ済み（フォグ色 × 不透明度, 不透明度）で、
//!         ブレンドは ONE / INV_SRC_ALPHA を前提にする
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Fog.sh"
#include "TextureStages.sh"

// 深度（ライティングと同じステージ。点で読む）
SAMPLER2D(s_gbufferDepth, STAGE_GBUFFER_DEPTH);

void main()
{
    // NDC はそのまま、テクスチャ座標（Y 下向き）は反転して使う
    vec2 ndc      = v_texcoord0 * 2.0 - 1.0;
    vec2 screenUV = vec2(v_texcoord0.x, 1.0 - v_texcoord0.y);

    float depth = texture2D(s_gbufferDepth, screenUV).r;

    // 視線方向はリバース Z の最遠点（z = 0）から求める（Sky.ps.sc と同じ式）
    vec4 farPos = mul(vec4(ndc, 0.0, 1.0), invViewProj);
    farPos /= farPos.w;

    vec3 rayDir = normalize(farPos.xyz - cameraPos.xyz);

    // リバース Z：深度 0 = 何も描かれていない背景（スカイパスの結果）
    bool isBackground = (depth <= 0.0);

    vec4 worldPos = mul(vec4(ndc, depth, 1.0), invViewProj);
    worldPos /= worldPos.w;

    vec4 fog = ComputeFog(worldPos.xyz, rayDir, isBackground);

    // プリマルチプライ済みで返す（ブレンドは ONE / INV_SRC_ALPHA）
    gl_FragColor = vec4(fog.rgb * fog.a, fog.a);
}
