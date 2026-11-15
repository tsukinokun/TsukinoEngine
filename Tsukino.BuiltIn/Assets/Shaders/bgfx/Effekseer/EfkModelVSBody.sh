//--------------------------------------------------------------
//! @file   EfkModelVSBody.sh
//! @brief  Effekseer のモデルの頂点シェーダーの本体（main の中で include する）
//! @note   入力: a_position、a_color0（法線）、a_color1（従法線）、a_color2（接線）、a_color3（色）、a_texcoord0（UV）
//--------------------------------------------------------------
    int  index      = gl_InstanceID;
    mat4 model      = mModel(index);
    vec4 uv         = fUV(index);
    vec4 modelColor = fModelColor(index) * a_color3;

    vec4 worldPos = mul(vec4(a_position, 1.0), model);
    gl_Position   = mul(worldPos, mCameraProj);
    v_color       = modelColor;

    vec2 outputUV = a_texcoord0.xy;
    outputUV.x    = outputUV.x * uv.z + uv.x;
    outputUV.y    = outputUV.y * uv.w + uv.y;
    outputUV.y    = mUVInversed.x + mUVInversed.y * outputUV.y;
    v_uv          = vec4(outputUV, 0.0, 0.0);

#if defined(EFK_LIGHTING) || defined(EFK_DISTORTION)
    vec3 worldNormal   = normalize(mul(vec4(a_color0.xyz, 0.0), model).xyz);
    vec3 worldBinormal = normalize(mul(vec4(a_color1.xyz, 0.0), model).xyz);
    vec3 worldTangent  = normalize(mul(vec4(a_color2.xyz, 0.0), model).xyz);
#endif

#if defined(EFK_LIGHTING)
    v_worldN = worldNormal;
    v_worldB = worldBinormal;
    v_worldT = worldTangent;
#elif defined(EFK_DISTORTION)
    v_projBinormal = mul(worldPos + vec4(worldBinormal, 0.0), mCameraProj);
    v_projTangent  = mul(worldPos + vec4(worldTangent, 0.0), mCameraProj);
#elif defined(EFK_ADVANCED)
    v_worldN = normalize(mul(vec4(a_color0.xyz, 0.0), model).xyz);    // advanced の unlit はフォールオフに法線を使う
#endif

#if defined(EFK_ADVANCED)
    vec2 baseUV = a_texcoord0.xy;
    vec4 alphaUV             = fAlphaUV(index);
    vec4 uvDistortionUV      = fUVDistortionUV(index);
    vec4 blendUV             = fBlendUV(index);
    vec4 blendAlphaUV        = fBlendAlphaUV(index);
    vec4 blendUVDistortionUV = fBlendUVDistortionUV(index);

    v_alphaDistUV.xy      = baseUV * alphaUV.zw + alphaUV.xy;                            // アルファ
    v_alphaDistUV.zw      = baseUV * uvDistortionUV.zw + uvDistortionUV.xy;              // UV の歪み
    v_blendNextUV.xy      = baseUV * blendUV.zw + blendUV.xy;                            // ブレンド
    v_blendAlphaDistUV.xy = baseUV * blendAlphaUV.zw + blendAlphaUV.xy;                  // ブレンドのアルファ
    v_blendAlphaDistUV.zw = baseUV * blendUVDistortionUV.zw + blendUVDistortionUV.xy;    // ブレンドの UV の歪み

    // パラパラ漫画の次のコマ
    float flipbookRate        = 0.0;
    vec2  flipbookNextIndexUV = vec2_splat(0.0);
    ApplyFlipbookVS(flipbookRate, flipbookNextIndexUV, flipbookParameter1, flipbookParameter2, fFlipbookIndexAndNextRate(index).x, v_uv.xy,
                    mUVInversed.xy);

    v_blendNextUV.zw = flipbookNextIndexUV;
    v_uv.z           = flipbookRate;
    v_uv.w           = fModelAlphaThreshold(index).x;

    // 上下の反転
    v_alphaDistUV.y      = mUVInversed.x + mUVInversed.y * v_alphaDistUV.y;
    v_alphaDistUV.w      = mUVInversed.x + mUVInversed.y * v_alphaDistUV.w;
    v_blendNextUV.y      = mUVInversed.x + mUVInversed.y * v_blendNextUV.y;
    v_blendAlphaDistUV.y = mUVInversed.x + mUVInversed.y * v_blendAlphaDistUV.y;
    v_blendAlphaDistUV.w = mUVInversed.x + mUVInversed.y * v_blendAlphaDistUV.w;
#endif

    v_posP = gl_Position;
