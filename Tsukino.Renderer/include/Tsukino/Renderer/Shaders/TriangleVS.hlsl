//--------------------------------
//! @file TriangleVS.hlsl
//! @brief 三角形描画用頂点シェーダー
//--------------------------------

//--------------------------------
// 入力構造体
//--------------------------------
struct VSInput
{
    float3 position : POSITION; // 3D座標（今回はZ=0）
    float4 color : COLOR; // 頂点カラー
};

//--------------------------------
// 出力構造体
//--------------------------------
struct VSOutput
{
    float4 position : SV_POSITION; // 画面に向けての最終位置
    float4 color : COLOR; // ピクセルへ渡すカラー
};

//--------------------------------
//! @brief 三角形描画用頂点シェーダー
//! @param IN [in] 頂点シェーダー入力
//! @return 出力構造体
//--------------------------------
VSOutput VSMain(VSInput IN)
{
    VSOutput OUT; // 出力構造体を宣言
    OUT.position = float4(IN.position, 1.0); // 位置情報を設定
    OUT.color = IN.color; // 頂点カラーを設定
    return OUT; // 出力構造体を返す
}
