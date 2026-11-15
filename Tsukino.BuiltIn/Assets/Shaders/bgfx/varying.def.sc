//--------------------------------------------------------------
// bgfx 版の組み込みシェーダーが使う、頂点の入力と、頂点からピクセルへ渡す値の定義。
// このフォルダの .sc はすべてこの1つのファイルを使う（Tools/Shaders/CMakeLists.txt）。
// 頂点の入力（a_*）の名前は、C++ 側の bgfx::VertexLayout の Attrib と対応している。
//--------------------------------------------------------------
vec4 v_color0    : COLOR0    = vec4(1.0, 1.0, 1.0, 1.0);
vec2 v_texcoord0 : TEXCOORD0 = vec2(0.0, 0.0);
vec3 v_worldPos  : TEXCOORD1 = vec3(0.0, 0.0, 0.0);
vec3 v_normal    : NORMAL    = vec3(0.0, 0.0, 1.0);
vec4 v_curClip   : TEXCOORD2 = vec4(0.0, 0.0, 0.0, 1.0);    // 今フレームのクリップ座標（速度の計算用）
vec4 v_prevClip  : TEXCOORD3 = vec4(0.0, 0.0, 0.0, 1.0);    // 前フレームのクリップ座標（同上）

vec3 a_position  : POSITION;
vec3 a_normal    : NORMAL;
vec4 a_color0    : COLOR0;
vec2 a_texcoord0 : TEXCOORD0;
vec4 a_indices   : BLENDINDICES;    // ボーンの番号（2本目の頂点バッファ。float で持つ）
vec4 a_weight    : BLENDWEIGHT;     // ボーンのウェイト（同上）

// インスタンスごとのデータ（DrawCommand::instanceData）。1インスタンスは vec4 が16個まで。
// bgfx の決まりで、i_data0 から順に TEXCOORD31・30・29… で届く。使う分だけを $input に書く
vec4 i_data0     : TEXCOORD31;
vec4 i_data1     : TEXCOORD30;
vec4 i_data2     : TEXCOORD29;
vec4 i_data3     : TEXCOORD28;
vec4 i_data4     : TEXCOORD27;
vec4 i_data5     : TEXCOORD26;
vec4 i_data6     : TEXCOORD25;
vec4 i_data7     : TEXCOORD24;
vec4 i_data8     : TEXCOORD23;
vec4 i_data9     : TEXCOORD22;
vec4 i_data10    : TEXCOORD21;
vec4 i_data11    : TEXCOORD20;
vec4 i_data12    : TEXCOORD19;
vec4 i_data13    : TEXCOORD18;
vec4 i_data14    : TEXCOORD17;
vec4 i_data15    : TEXCOORD16;
