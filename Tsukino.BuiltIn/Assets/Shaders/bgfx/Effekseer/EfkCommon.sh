//--------------------------------------------------------------
//! @file   EfkCommon.sh
//! @brief  Effekseer の標準シェーダーの共通関数（Effekseer の DX11 版の Linear_sRGB.fx・SoftParticle_PS.fx・
//!         FlipbookInterpolationUtils.fx・UVDistortionUtils_PS.fx・TextureBlendingUtils_PS.fx の bgfx 版）
//! @note   テクスチャを読む関数は、読んだ色を引数で受け取る形にしてある（サンプラーを関数へ渡さないため）。
//!         整数の剰余は、どの描画 API でも同じ結果になるよう浮動小数で計算する（値は 0 以上）
//--------------------------------------------------------------
#ifndef TSUKINO_EFK_COMMON_SH
#define TSUKINO_EFK_COMMON_SH

//--------------------------------------------------------------
// 色空間（ガンマのテクスチャを、リニアの画面へ描くときだけ変換する）
//--------------------------------------------------------------
vec3 EfkPositivePow(vec3 base, vec3 power)
{
    return pow(max(abs(base), vec3_splat(1.192092896e-07)), power);
}

vec3 EfkSRGBToLinear(vec3 c)
{
    return min(c, c * (c * (c * 0.305306011 + 0.682171111) + 0.012522878));
}

vec3 EfkLinearToSRGB(vec3 c)
{
    return max(1.055 * EfkPositivePow(c, vec3_splat(0.416666667)) - 0.055, vec3_splat(0.0));
}

vec4 ConvertFromSRGBTexture(vec4 c, bool isValid)
{
    if(!isValid)
        return c;
    return vec4(EfkLinearToSRGB(c.rgb), c.a);
}

vec4 ConvertToScreen(vec4 c, bool isValid)
{
    if(!isValid)
        return c;
    return vec4(EfkSRGBToLinear(c.rgb), c.a);
}

//--------------------------------------------------------------
// ソフトパーティクル（背景の深度に近いほど薄くする）
//--------------------------------------------------------------
float SoftParticle(float backgroundZ, float meshZ, vec4 softparticleParam, vec4 reconstruct1, vec4 reconstruct2)
{
    float distanceFar        = softparticleParam.x;
    float distanceNear       = softparticleParam.y;
    float distanceNearOffset = softparticleParam.z;

    vec2 rescale = reconstruct1.xy;
    vec4 params  = reconstruct2;

    vec2 zs = vec2(backgroundZ * rescale.x + rescale.y, meshZ);

    vec2  depth = (zs * params.w - params.y) / (params.x - zs * params.z);
    float dir   = sign(depth.x);
    depth *= dir;
    float alphaFar  = (depth.x - depth.y) / distanceFar;
    float alphaNear = (depth.y - distanceNearOffset) / distanceNear;
    return min(max(min(alphaFar, alphaNear), 0.0), 1.0);
}

//--------------------------------------------------------------
// パラパラ漫画（コマの補間）
//--------------------------------------------------------------
float EfkMod(float x, float y)
{
    return x - y * floor(x / y);
}

//! コマ番号 index の左上の UV のずれ
vec2 EfkFlipbookOffset(float index, float divideX, vec2 oneSize, vec2 offset)
{
    float i = floor(index);
    return vec2(EfkMod(i, divideX), floor(i / divideX)) * oneSize + offset;
}

void ApplyFlipbookVS(inout float flipbookRate, inout vec2 flipbookUV, vec4 flipbookParameter1, vec4 flipbookParameter2, float flipbookIndex,
                     vec2 uv, vec2 uvInversed)
{
    float flipbookEnabled  = flipbookParameter1.x;
    float flipbookLoopType = flipbookParameter1.y;
    float divideX          = flipbookParameter1.z;
    float divideY          = flipbookParameter1.w;

    vec2 flipbookOneSize = flipbookParameter2.xy;
    vec2 flipbookOffset  = flipbookParameter2.zw;

    if(flipbookEnabled > 0.0)
    {
        flipbookRate = fract(flipbookIndex);

        float Index     = floor(flipbookIndex);
        float NextIndex = Index + 1.0;
        float maxCount  = divideX * divideY;

        if(flipbookLoopType == 0.0)
        {
            // ループしない
            if(NextIndex >= maxCount)
            {
                NextIndex = maxCount - 1.0;
                Index     = maxCount - 1.0;
            }
        }
        else if(flipbookLoopType == 1.0)
        {
            // ループ
            Index     = EfkMod(Index, maxCount);
            NextIndex = EfkMod(NextIndex, maxCount);
        }
        else if(flipbookLoopType == 2.0)
        {
            // 往復
            bool reverse = EfkMod(floor(Index / maxCount), 2.0) == 1.0;
            Index        = EfkMod(Index, maxCount);
            if(reverse)
                Index = maxCount - 1.0 - floor(Index);

            reverse   = EfkMod(floor(NextIndex / maxCount), 2.0) == 1.0;
            NextIndex = EfkMod(NextIndex, maxCount);
            if(reverse)
                NextIndex = maxCount - 1.0 - floor(NextIndex);
        }

        vec2 notInversedUV = uv;
        notInversedUV.y    = uvInversed.x + uvInversed.y * notInversedUV.y;
        vec2 originUV      = notInversedUV - EfkFlipbookOffset(Index, divideX, flipbookOneSize, flipbookOffset);
        flipbookUV         = originUV + EfkFlipbookOffset(NextIndex, divideX, flipbookOneSize, flipbookOffset);
        flipbookUV.y       = uvInversed.x + uvInversed.y * flipbookUV.y;
    }
}

//--------------------------------------------------------------
// UV の歪み（歪みのテクスチャの色から、UV のずれを作る）
//--------------------------------------------------------------
vec2 UVDistortionOffset(vec4 sampledColor, vec2 uvInversed)
{
    vec2 uvOffset = sampledColor.rg * 2.0 - 1.0;
    uvOffset.y *= -1.0;
    uvOffset.y = uvInversed.x + uvInversed.y * uvOffset.y;
    return uvOffset;
}

//--------------------------------------------------------------
// ブレンドのテクスチャを重ねる（0: アルファ、1: 加算、2: 減算、3: 乗算）
//--------------------------------------------------------------
void ApplyTextureBlending(inout vec4 dstColor, vec4 blendColor, float blendType)
{
    if(blendType == 0.0)
        dstColor.rgb = blendColor.a * blendColor.rgb + (1.0 - blendColor.a) * dstColor.rgb;
    else if(blendType == 1.0)
        dstColor.rgb += blendColor.rgb * blendColor.a;
    else if(blendType == 2.0)
        dstColor.rgb -= blendColor.rgb * blendColor.a;
    else if(blendType == 3.0)
        dstColor.rgb *= blendColor.rgb * blendColor.a;
}

//--------------------------------------------------------------
// 画面の UV（クリップ座標から）。OpenGL 系は上下が逆なので戻す（Effekseer の __OPENGL__ と同じ）
//--------------------------------------------------------------
vec2 EfkScreenUV(vec4 posP)
{
    vec2 screenUV = (posP.xy / posP.w + 1.0) * 0.5;
    screenUV.y    = 1.0 - screenUV.y;
#if BGFX_SHADER_LANGUAGE_GLSL
    screenUV.y = 1.0 - screenUV.y;
#endif
    return screenUV;
}

#endif    // TSUKINO_EFK_COMMON_SH
