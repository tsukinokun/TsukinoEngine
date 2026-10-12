//--------------------------------------------------------------
//! @file   Dither.hlsli
//! @brief  網点（ディザ）でピクセルを抜き、物を少しずつ消す・出す共通関数
//! @note   半透明にせず「描く・描かない」で抜くので、ディファードの GBuffer パスと
//!         シャドウパスのどちらでもそのまま使え、深度も正しく書ける。
//!         見えている割合は DrawCommand::ditherFade（b1 の motionFlags.y）から渡る
//--------------------------------------------------------------
#ifndef TSUKINO_DITHER_HLSLI
#define TSUKINO_DITHER_HLSLI

//--------------------------------------------------------------
//! 4×4 の Bayer 行列から、画素ごとのしきい値（0〜1）を求めます。
//! @param  [in] pixel 画面上の画素の位置（SV_POSITION.xy）
//! @return しきい値。隣り合う画素ほど値が大きく離れるので、抜いた画素が均等に散る
//--------------------------------------------------------------
float BayerThreshold4x4(float2 pixel)
{
    static const float kBayer[16] = {
         0.0f,  8.0f,  2.0f, 10.0f,
        12.0f,  4.0f, 14.0f,  6.0f,
         3.0f, 11.0f,  1.0f,  9.0f,
        15.0f,  7.0f, 13.0f,  5.0f
    };
    const uint2 p = uint2(pixel) & 3u;
    return (kBayer[p.y * 4u + p.x] + 0.5f) / 16.0f;
}

//--------------------------------------------------------------
//! 見えている割合に応じて、網点でピクセルを抜きます。
//! @param  [in] pixel 画面上の画素の位置（SV_POSITION.xy）
//! @param  [in] fade  見えている割合（1 以上なら何もしない、0 以下なら全部抜ける）
//--------------------------------------------------------------
void ClipDither(float2 pixel, float fade)
{
    if(fade >= 1.0f)
        return;
    clip(fade - BayerThreshold4x4(pixel));
}

#endif    // TSUKINO_DITHER_HLSLI
