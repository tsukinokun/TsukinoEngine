//--------------------------------------------------------------
//! @file   EfkSpriteVSBody.sh
//! @brief  Effekseer のスプライトの頂点シェーダーの本体（main の中で include する）
//! @note   入力の名前は、include する側が頂点形式に合わせて #define する。
//!         EFK_IN_UV（UV）、EFK_IN_NORMAL・EFK_IN_TANGENT（EFK_LIGHTING / EFK_DISTORTION のとき）、
//!         EFK_IN_ALPHA_DIST_UV・EFK_IN_BLEND_UV・EFK_IN_BLEND_ALPHA_DIST_UV・EFK_IN_FLIPBOOK_INDEX・
//!         EFK_IN_ALPHA_THRESHOLD（EFK_ADVANCED のとき）
//--------------------------------------------------------------
    vec4 worldPos = vec4(a_position, 1.0);
    gl_Position   = mul(worldPos, mCameraProj);
    v_color       = a_color0;

    vec2 uv1 = EFK_IN_UV.xy;
    uv1.y    = mUVInversed.x + mUVInversed.y * uv1.y;
    v_uv     = vec4(uv1, 0.0, 0.0);

#if defined(EFK_LIGHTING) || defined(EFK_DISTORTION)
    // 法線と接線は 0〜1 に詰めてある
    vec3  worldNormal       = (EFK_IN_NORMAL.xyz - 0.5) * 2.0;
    vec3  worldTangent      = (EFK_IN_TANGENT.xyz - 0.5) * 2.0;
    float tangentHandedness = EFK_IN_TANGENT.w * 2.0 - 1.0;
    vec3  worldBinormal     = cross(worldNormal, worldTangent) * tangentHandedness;
#endif

#if defined(EFK_LIGHTING)
    v_worldN = worldNormal;
    v_worldB = worldBinormal;
    v_worldT = worldTangent;
#elif defined(EFK_DISTORTION)
    v_projTangent  = mul(worldPos + vec4(worldTangent, 0.0), mCameraProj);
    v_projBinormal = mul(worldPos + vec4(worldBinormal, 0.0), mCameraProj);
#elif defined(EFK_ADVANCED)
    v_worldN = vec3_splat(0.0);    // advanced の unlit も法線を渡す（スプライトには無いので 0）
#endif

#if defined(EFK_ADVANCED)
    // アルファと UV の歪みのテクスチャ
    v_alphaDistUV   = EFK_IN_ALPHA_DIST_UV;
    v_alphaDistUV.y = mUVInversed.x + mUVInversed.y * EFK_IN_ALPHA_DIST_UV.y;
    v_alphaDistUV.w = mUVInversed.x + mUVInversed.y * EFK_IN_ALPHA_DIST_UV.w;

    // ブレンドのテクスチャ
    v_blendNextUV.xy = EFK_IN_BLEND_UV.xy;
    v_blendNextUV.y  = mUVInversed.x + mUVInversed.y * EFK_IN_BLEND_UV.y;

    // ブレンドのアルファと UV の歪みのテクスチャ
    v_blendAlphaDistUV   = EFK_IN_BLEND_ALPHA_DIST_UV;
    v_blendAlphaDistUV.y = mUVInversed.x + mUVInversed.y * EFK_IN_BLEND_ALPHA_DIST_UV.y;
    v_blendAlphaDistUV.w = mUVInversed.x + mUVInversed.y * EFK_IN_BLEND_ALPHA_DIST_UV.w;

    // パラパラ漫画の次のコマ
    float flipbookRate        = 0.0;
    vec2  flipbookNextIndexUV = vec2_splat(0.0);
    ApplyFlipbookVS(flipbookRate, flipbookNextIndexUV, flipbookParameter1, flipbookParameter2, EFK_IN_FLIPBOOK_INDEX.x, v_uv.xy, mUVInversed.xy);

    v_blendNextUV.zw = flipbookNextIndexUV;
    v_uv.z           = flipbookRate;
    v_uv.w           = EFK_IN_ALPHA_THRESHOLD.x;
#endif

    v_posP = gl_Position;
