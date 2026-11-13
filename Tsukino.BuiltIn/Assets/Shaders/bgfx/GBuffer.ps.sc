$input v_worldPos, v_normal, v_texcoord0, v_curClip, v_prevClip

//--------------------------------------------------------------
//! @file   GBuffer.ps.sc
//! @brief  不透明なモデルを G-Buffer へ書くピクセルシェーダー（GBuffer.ps.hlsl の bgfx 版）
//! @note   書き先は6枚（DX11 版と同じ並び）。
//!         0: albedo, 1: 法線 + ShadingModel, 2: metallic/roughness/specular/AO（トゥーンは境目/ぼかし/ハイライト/AO）,
//!         3: emissive + リム, 4: ワールド座標 + トゥーンの暗い色, 5: 速度（UV / フレーム）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Transform.sh"
#include "Material.sh"
#include "PBR.sh"
#include "Toon.sh"
#include "Dither.sh"
#include "TextureStages.sh"

SAMPLER2D(s_albedo, STAGE_ALBEDO);
SAMPLER2D(s_normalMap, STAGE_NORMAL_MAP);
SAMPLER2D(s_metallicRoughness, STAGE_METALLIC_ROUGHNESS);    // G=roughness, B=metallic（glTF の決まり）
SAMPLER2D(s_emissiveMap, STAGE_EMISSIVE_MAP);
SAMPLER2D(s_aoMap, STAGE_AO_MAP);

void main()
{
    //----------------------------------------------------------
    // 網点フェードと遮り抜き（GBuffer.ps.hlsl と同じ）
    //----------------------------------------------------------
    float visible = motionFlags.y;
    if(motionFlags.z > 0.5 && occlusionCutout0.w > 0.5) {
        float distanceFromCamera = length(v_worldPos - cameraPos.xyz);
        if(distanceFromCamera < occlusionCutout1.x - occlusionCutout1.y) {
            float r = length(gl_FragCoord.xy - occlusionCutout0.xy) / max(occlusionCutout0.z, 1.0);
            if(r < 1.0)
                visible *= mix(occlusionCutout1.z, 1.0, smoothstep(occlusionCutout1.w, 1.0, r));
        }
    }
    ClipDither(gl_FragCoord.xy, visible);

    vec4 albedoSample = texture2D(s_albedo, v_texcoord0);
    if(albedoSample.a - alphaCutoff < 0.0)
        discard;

    vec3 albedo = albedoSample.rgb * baseColor.rgb;

    vec3 vertexN  = normalize(v_normal);
    vec3 tangentN = DecodeNormal(texture2D(s_normalMap, v_texcoord0).rgb);
    vec3 N        = ApplyNormalMap(vertexN, v_worldPos, v_texcoord0, tangentN);

    vec4  mrSample = texture2D(s_metallicRoughness, v_texcoord0);
    float met      = metallic * mrSample.b;
    float rough    = roughness * mrSample.g;
    float ao       = texture2D(s_aoMap, v_texcoord0).r;

    vec3 V              = normalize(cameraPos.xyz - v_worldPos);
    vec3 emissiveSample = texture2D(s_emissiveMap, v_texcoord0).rgb;
    vec3 emissiveTotal  = emissive * emissiveSample + EvaluateEmissiveBoost(N, V);

    // 画面の UV（左上原点）の動き
    vec2 curUV  = (v_curClip.xy / v_curClip.w) * vec2(0.5, -0.5) + 0.5;
    vec2 prevUV = (v_prevClip.xy / v_prevClip.w) * vec2(0.5, -0.5) + 0.5;

    int   shadingModel = int(floor(shading.x + 0.5));
    vec4  materialOut  = vec4(met, rough, specular, ao);
    float shadeOut     = 0.0;
    if(shadingModel == SHADING_MODEL_TOON) {
        materialOut = vec4(shading.y, shading.z, shading.w, ao);
        shadeOut    = PackShadeColor(toonShadeColor.rgb);
    }

    gl_FragData[0] = vec4(albedo, 1.0);
    gl_FragData[1] = vec4(EncodeNormal(N), EncodeShadingModel(shadingModel));
    gl_FragData[2] = materialOut;
    gl_FragData[3] = vec4(emissiveTotal, 0.0);
    gl_FragData[4] = vec4(v_worldPos, shadeOut);
    gl_FragData[5] = vec4(curUV - prevUV, 0.0, 0.0);
}
