//--------------------------------------------------------------
//! @file   EfkDistortionPSBody.sh
//! @brief  Effekseer の歪みのピクセルシェーダーの本体（main の中で include する）
//! @note   テクスチャの赤と緑で、背景（s_backTex）を読む位置をずらす。
//!         EFK_ADVANCED でアルファ・UV の歪み・ブレンド・パラパラ漫画を足す
//--------------------------------------------------------------
#if defined(EFK_ADVANCED)
    vec2  alphaUV             = v_alphaDistUV.xy;
    vec2  uvDistortionUV      = v_alphaDistUV.zw;
    vec2  blendUV             = v_blendNextUV.xy;
    vec2  blendAlphaUV        = v_blendAlphaDistUV.xy;
    vec2  blendUVDistortionUV = v_blendAlphaDistUV.zw;
    vec2  flipbookNextIndexUV = v_blendNextUV.zw;
    float flipbookRate        = v_uv.z;
    float alphaThreshold      = v_uv.w;

    vec2 uvOffset = UVDistortionOffset(texture2D(s_uvDistortionTex, uvDistortionUV), fUVDistortionParameter.zw) * fUVDistortionParameter.x;
#else
    vec2 uvOffset = vec2_splat(0.0);
#endif

    vec4 color = texture2D(s_colorTex, v_uv.xy + uvOffset);
    color.a    = color.a * v_color.a;

#if defined(EFK_ADVANCED)
    // パラパラ漫画（次のコマと混ぜる）
    if(fFlipbookParameter.x > 0.0) {
        vec4 nextColor = texture2D(s_colorTex, flipbookNextIndexUV + uvOffset) * v_color;
        if(fFlipbookParameter.y == 1.0)
            color = mix(color, nextColor, flipbookRate);
    }

    // アルファのテクスチャ
    vec4 alphaTexColor = texture2D(s_alphaTex, alphaUV + uvOffset);
    color.a *= alphaTexColor.r * alphaTexColor.a;

    // ブレンドのテクスチャ
    vec2 blendUVOffset = UVDistortionOffset(texture2D(s_blendUVDistortionTex, blendUVDistortionUV), fUVDistortionParameter.zw) * fUVDistortionParameter.y;
    vec4 blendTextureColor      = texture2D(s_blendTex, blendUV + blendUVOffset);
    vec4 blendAlphaTextureColor = texture2D(s_blendAlphaTex, blendAlphaUV + blendUVOffset);
    blendTextureColor.a *= blendAlphaTextureColor.r * blendAlphaTextureColor.a;

    ApplyTextureBlending(color, blendTextureColor, fBlendTextureParameter.x);

    if(color.a <= max(0.0, alphaThreshold))
        discard;
#endif

    // 接線と従法線の向きへ、テクスチャの色のぶんだけずらした画面の位置
    vec2  pos    = v_posP.xy / v_posP.w;
    vec2  posR   = v_projTangent.xy / v_projTangent.w;
    vec2  posU   = v_projBinormal.xy / v_projBinormal.w;
    float xscale = (color.x * 2.0 - 1.0) * v_color.x * g_scale.x;
    float yscale = (color.y * 2.0 - 1.0) * v_color.y * g_scale.x;

    vec2 uv = pos + (posR - pos) * xscale + (posU - pos) * yscale;
    uv.x    = (uv.x + 1.0) * 0.5;
    uv.y    = 1.0 - (uv.y + 1.0) * 0.5;
    uv.y    = mUVInversedBack.x + mUVInversedBack.y * uv.y;
#if BGFX_SHADER_LANGUAGE_GLSL
    uv.y = 1.0 - uv.y;
#endif

    color.xyz = texture2D(s_backTex, uv).xyz;

    // ソフトパーティクル（背景の深度に近いほど薄くする）
    if(softParticleParam.w != 0.0) {
        float backgroundZ = texture2D(s_depthTex, EfkScreenUV(v_posP)).x;
        color.a *= SoftParticle(backgroundZ, v_posP.z / v_posP.w, softParticleParam, reconstructionParam1, reconstructionParam2);
    }

#if !defined(EFK_ADVANCED)
    if(color.a == 0.0)
        discard;
#endif

    gl_FragColor = color;
