$input v_texcoord0

//--------------------------------------------------------------
//! @file   Lighting.ps.sc
//! @brief  G-Buffer を読んで照らすディファードライティング（Lighting.ps.hlsl の bgfx 版）
//! @note   太陽（シーン定数の lightDir / lightColor）と点光源・スポットライト（u_lights）を当て、
//!         環境光（IBL）を足して HDR へ書く。影は段階 1-4 c で移す（今はいつも照らされている扱い）
//--------------------------------------------------------------
#include <bgfx_shader.sh>
#include "Scene.sh"
#include "Lighting.sh"
#include "IBL.sh"
#include "Toon.sh"

void main()
{
    vec2 screenUV = vec2(v_texcoord0.x, 1.0 - v_texcoord0.y);

    // リバース Z なので、何も描かれていない画素の深度は 0（背景はそのまま残す）
    float depth = texture2D(s_gbufferDepth, screenUV).r;
    if(depth <= 0.0)
        discard;

    vec4 worldPosSample = texture2D(s_gbufferWorldPos, screenUV);
    vec3 worldPos       = worldPosSample.xyz;
    vec4 albedoSample   = texture2D(s_gbufferAlbedo, screenUV);
    vec4 normalSample   = texture2D(s_gbufferNormal, screenUV);
    vec4 materialSample = texture2D(s_gbufferMaterial, screenUV);
    vec3 emissiveSample = texture2D(s_gbufferEmissive, screenUV).rgb;

    vec3  albedo = albedoSample.rgb;
    vec3  N      = DecodeNormal(normalSample.rgb);
    float met    = materialSample.r;
    float rough  = materialSample.g;
    float spec   = materialSample.b;
    float ao     = materialSample.a;

    int shadingModel = DecodeShadingModel(normalSample.a);
    if(shadingModel == SHADING_MODEL_UNLIT) {
        gl_FragColor = vec4(albedo + emissiveSample, 1.0);
        return;
    }

    bool  isToon         = (shadingModel == SHADING_MODEL_TOON);
    float toonThreshold  = materialSample.r;
    float toonSmoothness = materialSample.g;
    float toonSpecular   = materialSample.b;
    vec3  toonShade      = UnpackShadeColor(worldPosSample.a);

    vec3 V   = normalize(cameraPos.xyz - worldPos);
    vec3 lit = vec3_splat(0.0);

    //----------------------------------------------------------
    // 太陽
    //----------------------------------------------------------
    {
        vec3  L      = normalize(-lightDir.xyz);
        float shadow = GetShadowPCF(worldPos, N, L);
        if(isToon) {
            lit += EvaluateToon(N, V, L, albedo, lightColor.rgb * lightColor.w, shadow, toonThreshold, toonSmoothness, toonShade, toonSpecular, true);
        } else {
            shadow        = max(shadow, kShadowMinLit);
            vec3 radiance = lightColor.rgb * lightColor.w * shadow;
            lit += EvaluatePBR(N, V, L, albedo, met, rough, spec, radiance);
        }
    }

    //----------------------------------------------------------
    // 点光源・スポットライト
    //----------------------------------------------------------
    int count = min(lightCount, TSUKINO_MAX_LIGHTS);
    for(int i = 0; i < TSUKINO_MAX_LIGHTS; ++i) {
        if(i >= count)
            break;

        vec4  positionRange = lightPositionRange(i);
        vec3  toLight       = positionRange.xyz - worldPos;
        float dist          = length(toLight);
        vec3  L             = toLight / max(dist, 1e-5);

        float atten = Attenuate(dist, positionRange.w);

        vec4 directionType = lightDirectionType(i);
        if(directionType.w > 0.5) {
            vec4  spotParams = lightSpotParams(i);
            vec3  spotDir    = normalize(directionType.xyz);
            float cosAngle   = dot(-L, spotDir);
            float spotAtten  = saturate((cosAngle - spotParams.y) / max(spotParams.x - spotParams.y, 1e-4));
            atten *= spotAtten * spotAtten;
        }

        vec4 colorIntensity = lightColorIntensity(i);
        vec3 radiance       = colorIntensity.rgb * colorIntensity.a * atten;
        if(isToon)
            lit += EvaluateToon(N, V, L, albedo, radiance, 1.0, toonThreshold, toonSmoothness, toonShade, toonSpecular, false);
        else
            lit += EvaluatePBR(N, V, L, albedo, met, rough, spec, radiance);
    }

    //----------------------------------------------------------
    // 環境光（トゥーンは拡散だけ）
    //----------------------------------------------------------
    vec3 ambient = isToon ? EvaluateIBL(N, V, albedo, 0.0, 1.0, 0.0, ao) : EvaluateIBL(N, V, albedo, met, rough, spec, ao);

    gl_FragColor = vec4(ambient + lit + emissiveSample, 1.0);
}
