//--------------------------------------------------------------
// Effekseer の標準シェーダー（bgfx 版）が使う、頂点の入力と、頂点からピクセルへ渡す値の定義。
// このフォルダの .sc はすべてこの1つのファイルを使う（tsukino_shaders() がフォルダごとに変換する）。
//
// 頂点の入力は、Effekseer の頂点形式（EffekseerRenderer::GetVertexLayout）の
// セマンティクスを決まった名前へ当てる（src/Bgfx/Effekseer/GraphicsDevice.cpp と揃えること）。
//   POSITION → a_position、NORMAL i → a_color{i}、TEXCOORD i → a_texcoord{i}
// 型は頂点形式によって float2〜float4・UNORM と変わるので、どれも vec4 で受ける（足りない成分は 0, 0, 0, 1）。
//
// 頂点シェーダーの出力とピクセルシェーダーの入力は、組ごとに同じ並びにしてある
// （スプライトとモデルが同じピクセルシェーダーを使うため）。
//--------------------------------------------------------------
vec4 v_color            : COLOR0    = vec4(1.0, 1.0, 1.0, 1.0);
vec4 v_uv               : TEXCOORD0 = vec4(0.0, 0.0, 0.0, 0.0);    // xy: UV、z: パラパラ漫画の補間率、w: アルファの閾値（advanced のとき）
vec3 v_worldN           : TEXCOORD1 = vec3(0.0, 0.0, 1.0);
vec3 v_worldB           : TEXCOORD2 = vec3(0.0, 1.0, 0.0);
vec3 v_worldT           : TEXCOORD3 = vec3(1.0, 0.0, 0.0);
vec4 v_projBinormal     : TEXCOORD4 = vec4(0.0, 0.0, 0.0, 1.0);
vec4 v_projTangent      : TEXCOORD5 = vec4(0.0, 0.0, 0.0, 1.0);
vec4 v_posP             : TEXCOORD6 = vec4(0.0, 0.0, 0.0, 1.0);
vec4 v_alphaDistUV      : TEXCOORD7 = vec4(0.0, 0.0, 0.0, 0.0);    // xy: アルファ、zw: UV の歪み
vec4 v_blendAlphaDistUV : TEXCOORD8 = vec4(0.0, 0.0, 0.0, 0.0);    // xy: ブレンドのアルファ、zw: ブレンドの UV の歪み
vec4 v_blendNextUV      : TEXCOORD9 = vec4(0.0, 0.0, 0.0, 0.0);    // xy: ブレンド、zw: パラパラ漫画の次のコマ

vec3 a_position  : POSITION;
vec4 a_color0    : COLOR0;
vec4 a_color1    : COLOR1;
vec4 a_color2    : COLOR2;
vec4 a_color3    : COLOR3;
vec4 a_texcoord0 : TEXCOORD0;
vec4 a_texcoord1 : TEXCOORD1;
vec4 a_texcoord2 : TEXCOORD2;
vec4 a_texcoord3 : TEXCOORD3;
vec4 a_texcoord4 : TEXCOORD4;
vec4 a_texcoord5 : TEXCOORD5;
vec4 a_texcoord6 : TEXCOORD6;
