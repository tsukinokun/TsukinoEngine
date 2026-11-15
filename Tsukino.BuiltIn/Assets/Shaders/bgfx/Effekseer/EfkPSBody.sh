//--------------------------------------------------------------
//! @file   EfkPSBody.sh
//! @brief  Effekseer の unlit / lit のピクセルシェーダーの本体（main の中で include する）
//! @note   EFK_LIGHTING で法線マップの照明、EFK_ADVANCED でアルファ・UV の歪み・ブレンド・パラパラ漫画・
//!         フォールオフ・縁の色を足す。サンプラーの名前（s_colorTex など）は include する側が #define する
//--------------------------------------------------------------
    bool convertColorSpace = miscFlags.x != 0.0;

#if defined(EFK_ADVANCED)
    vec2  alphaUV             = v_alphaDistUV.xy;
    vec2  uvDistortionUV      = v_alphaDistUV.zw;
    vec2  blendUV             = v_blendNextUV.xy;
    vec2  blendAlphaUV        = v_blendAlphaDistUV.xy;
    vec2  blendUVDistortionUV = v_blendAlphaDistUV.zw;
    vec2  flipbookNextIndexUV = v_blendNextUV.zw;
    float flipbookRate        = v_uv.z;
    float alphaThreshold      = v_uv.w;

    vec2 uvOffset = UVDistortionOffset(ConvertFromSRGBTexture(texture2D(s_uvDistortionTex, uvDistortionUV), convertColorSpace), fUVDistortionParameter.zw);
    uvOffset *= fUVDistortionParameter.x;
#else
    vec2 uvOffset = vec2_splat(0.0);
#endif

    vec4 color = ConvertFromSRGBTexture(texture2D(s_colorTex, v_uv.xy + uvOffset), convertColorSpace) * v_color;

#if defined(EFK_LIGHTING)
    vec3 texNormal   = (texture2D(s_normalTex, v_uv.xy + uvOffset).xyz - 0.5) * 2.0;
    vec3 localNormal = normalize(v_worldT * texNormal.x + v_worldB * texNormal.y + v_worldN * texNormal.z);
#endif

#if defined(EFK_ADVANCED)
    // パラパラ漫画（次のコマと混ぜる）
    if(fFlipbookParameter.x > 0.0) {
        vec4 nextColor = ConvertFromSRGBTexture(texture2D(s_colorTex, flipbookNextIndexUV + uvOffset), convertColorSpace) * v_color;
        if(fFlipbookParameter.y == 1.0)
            color = mix(color, nextColor, flipbookRate);
    }

    // アルファのテクスチャ
    vec4 alphaTexColor = ConvertFromSRGBTexture(texture2D(s_alphaTex, alphaUV + uvOffset), convertColorSpace);
    color.a *= alphaTexColor.r * alphaTexColor.a;

    // ブレンドのテクスチャ（UV の歪みは別の強さ）
    vec2 blendUVOffset = UVDistortionOffset(ConvertFromSRGBTexture(texture2D(s_blendUVDistortionTex, blendUVDistortionUV), convertColorSpace),
                                            fUVDistortionParameter.zw);
    blendUVOffset *= fUVDistortionParameter.y;

    vec4 blendTextureColor      = ConvertFromSRGBTexture(texture2D(s_blendTex, blendUV + blendUVOffset), convertColorSpace);
    vec4 blendAlphaTextureColor = ConvertFromSRGBTexture(texture2D(s_blendAlphaTex, blendAlphaUV + blendUVOffset), convertColorSpace);
    blendTextureColor.a *= blendAlphaTextureColor.r * blendAlphaTextureColor.a;

    ApplyTextureBlending(color, blendTextureColor, fBlendTextureParameter.x);
#endif

#if defined(EFK_LIGHTING)
    float diffuse = max(dot(fLightDirection.xyz, localNormal), 0.0);
    color.xyz     = color.xyz * (fLightColor.xyz * diffuse + fLightAmbient.xyz);
#endif

#if defined(EFK_ADVANCED)
    // フォールオフ（視線と法線の角度で色を変える）
    if(fFalloffParameter.x == 1.0) {
        vec3 cameraVec = normalize(-fCameraFrontDirection.xyz);
#if defined(EFK_LIGHTING)
        float cdotN = saturate(dot(cameraVec, localNormal));
#else
        float cdotN = saturate(dot(cameraVec, normalize(v_worldN)));
#endif
        vec4 falloffBlendColor = mix(fFalloffEndColor, fFalloffBeginColor, pow(cdotN, fFalloffParameter.z));

        if(fFalloffParameter.y == 0.0)
            color.rgb += falloffBlendColor.rgb;
        else if(fFalloffParameter.y == 1.0)
            color.rgb -= falloffBlendColor.rgb;
        else if(fFalloffParameter.y == 2.0)
            color.rgb *= falloffBlendColor.rgb;

        color.a *= falloffBlendColor.a;
    }
#endif

    color.rgb *= fEmissiveScaling.x;

    // ソフトパーティクル（背景の深度に近いほど薄くする）
    vec2 screenUV = EfkScreenUV(v_posP);
    screenUV.y    = mUVInversedBack.x + mUVInversedBack.y * screenUV.y;
    if(softParticleParam.w != 0.0) {
        float backgroundZ = texture2D(s_depthTex, screenUV).x;
        color.a *= SoftParticle(backgroundZ, v_posP.z / v_posP.w, softParticleParam, reconstructionParam1, reconstructionParam2);
    }

#if defined(EFK_ADVANCED)
    // 0 と、アルファの閾値以下は描かない。閾値のすぐ上は縁の色にする
    if(color.a <= max(0.0, alphaThreshold))
        discard;

    color.rgb = mix(fEdgeColor.rgb * fEdgeParameter.y, color.rgb, ceil((color.a - alphaThreshold) - fEdgeParameter.x));
#else
    if(color.a == 0.0)
        discard;
#endif

    gl_FragColor = ConvertToScreen(color, convertColorSpace);
