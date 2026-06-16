//--------------------------------------------------------------
//! @file   Tonemap.vs.hlsl
//! @brief  トーンマッピング用頂点シェーダー
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma pack_matrix(row_major)

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

//--------------------------------------------------------------
//! @brief メイン関数
//! @note  頂点バッファ不要、SV_VertexIDから3頂点でフルスクリーン三角形を生成
//--------------------------------------------------------------
VSOutput VSMain(uint id : SV_VertexID)
{
    VSOutput output;

    // id=0 → (0,0), id=1 → (2,0), id=2 → (0,2)
    float2 uv = float2((id << 1) & 2, id & 2);

    output.uv = uv;
    output.position = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f); // リバースZ：z=0が最遠

    return output;
}
