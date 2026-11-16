# Third-Party Notices

TsukinoEngine 本体は MIT ライセンスで配布しています（`LICENSE` を参照）。
本体とは別に、以下の第三者ソフトウェアを利用しています。
それぞれの著作権表示とライセンス条文を以下に掲載します。

## 再配布するもの

このリポジトリを clone すると手元に入るものです。ライセンス条文を全文掲載します。

| コンポーネント | 用途 | 取得方法 | 固定版 | ライセンス |
|---|---|---|---|---|
| [Effekseer](https://github.com/effekseer/Effekseer) | パーティクル / エフェクトのランタイム | submodule（`tsukinokun/Effekseer` のフォーク） | `ec5eb6f` | MIT |
| [Effekseer — DirectX ランタイム](https://github.com/effekseer/Effekseer) | 同上（DirectX 版ランタイムに付属する追加ライセンス。標準シェーダーはその .fx から bgfx の .sc へ移した） | submodule | `ec5eb6f` | MIT |
| [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | 剛体シミュレーション（`Tsukino.Physics`） | submodule | `3c1f4ba (v5.5.0+)` | MIT |
| [EnTT](https://github.com/skypjack/entt) | ECS の基盤（`Tsukino.Core` の Registry / View） | submodule | `e08302e (v3.16.0+)` | MIT |
| [hlslpp](https://github.com/redorav/hlslpp) | SIMD 数学型（ベクトル / 行列 / クォータニオン） | submodule | `cd09302 (3.8+)` | MIT |
| [cereal](https://github.com/USCiLab/cereal) | シーンとアセットのシリアライズ | submodule | `22a1b36` | BSD 3-Clause |
| [miniaudio](https://github.com/mackron/miniaudio) | 音の再生と復号（wav / mp3 / flac。`Tsukino.Audio`） | submodule | `9634bed (0.11.25)` | Public Domain または MIT No Attribution（選べる） |
| [Premake](https://github.com/premake/premake-core) | プロジェクトファイルの生成 | `vendor/premake5.exe` にバイナリを直接コミット | `5.0.0-beta8` | BSD 3-Clause |
| [SDL](https://github.com/libsdl-org/SDL) | ウィンドウ・入力・IME・ゲームパッド・保存先などのプラットフォーム層 | submodule | `829a65d (3.4.18)` | zlib |
| [bgfx](https://github.com/bkaradzic/bgfx) | 描画の翻訳レイヤー | submodule | `77b6e84` | BSD 2-Clause |
| [bx](https://github.com/bkaradzic/bx) | bgfx の土台のライブラリ | submodule | `57548da` | BSD 2-Clause |
| [bimg](https://github.com/bkaradzic/bimg) | bgfx の画像ライブラリ。テクスチャの読み込み（stb_image ほか）と圧縮（libsquish・astc-encoder ほか。条文は `External/bimg/3rdparty/`）にも使う | submodule | `77bebb2` | BSD 2-Clause |
| [bgfx.cmake](https://github.com/bkaradzic/bgfx.cmake) | bgfx・bx・bimg を CMake でビルドする手順（ビルドにだけ使い、配布物には入らない） | submodule | `be0ef0d (v1.168.9624-612)` | CC0 1.0 |

組み込みシェーダーを変換するために bgfx の `shaderc` もビルドします。
`shaderc` はビルドにだけ使う道具で、exe にも配布物にも入りません（入るのは変換したシェーダーだけで、
その中に取り込まれる `bgfx_shader.sh` は上の bgfx の条文に含まれます）。`shaderc` が使う次のものは
bgfx の submodule の中（`External/bgfx/3rdparty/`）にあり、条文もそこにあります。

| コンポーネント | 用途 | 場所 | ライセンス |
|---|---|---|---|
| [glslang](https://github.com/KhronosGroup/glslang) | GLSL / HLSL の構文解析と SPIR-V の生成 | `3rdparty/glslang`（`LICENSE.txt`） | BSD 3-Clause ほか（ファイルごと。条文に記載） |
| [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools) | SPIR-V の最適化 | `3rdparty/spirv-tools`（`LICENSE`） | Apache 2.0 |
| [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) | SPIR-V から各シェーダー言語への変換 | `3rdparty/spirv-cross`（`LICENSE`） | Apache 2.0 |
| [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) | SPIR-V の定義 | `3rdparty/spirv-headers`（`LICENSE`） | MIT ほか（条文に記載） |
| [Dawn / Tint](https://dawn.googlesource.com/dawn) | WGSL への変換 | `3rdparty/dawn`（`LICENSE`） | BSD 3-Clause |
| D3DCompiler / DXC（`d3dcompiler_47.dll`・`dxcompiler.dll`） | Direct3D 向けのシェーダーのコンパイル | `tools/bin/windows` | Microsoft の再頒布条件（bgfx に同梱） |

`vendor/premake5.exe` は、
ソースではなくビルド済みバイナリをこのリポジトリへ直接コミットしています。
そのため条文の掲載義務はこのファイルが担っています。

## ビルド時に取得するもの

NuGet がビルド時に `.build/packages/` へ復元します。
このリポジトリが再配布しているわけではないため、条文は各パッケージに付属するものを参照してください。

| パッケージ | 上流 | 用途 | 版 | ライセンス |
|---|---|---|---|---|
| `AssimpCpp` | [Assimp](https://github.com/assimp/assimp) | FBX などのモデル読み込み | 5.0.1.6 | BSD 3-Clause |

Release の出力には、Visual Studio に付属する Microsoft Visual C++ ランタイム（`vcruntime140.dll`・`vcruntime140_1.dll`・`msvcp140.dll`）を
exe の隣へコピーします。これらは Visual Studio のライセンス条項に基づく再頒布可能コードで、上記のオープンソースライセンスの対象ではありません。

---

## ライセンス条文

### Effekseer

MIT — https://github.com/effekseer/Effekseer

```
The MIT License (MIT)

Copyright (c) 2011 Effekseer Project

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

### Effekseer — DirectX ランタイム

MIT — https://github.com/effekseer/Effekseer

```
The MIT License (MIT)

Copyright (c) 2011 Effekseer Project

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

DirectX Tool Kit	
https://directxtk.codeplex.com/	
Microsoft Public License (Ms-PL)
```

### Jolt Physics

MIT — https://github.com/jrouwe/JoltPhysics

```
Copyright 2021 Jorrit Rouwe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

### EnTT

MIT — https://github.com/skypjack/entt

```
The MIT License (MIT)

Copyright (c) 2017-2026 Michele Caini, author of EnTT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copy of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copy or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### hlslpp

MIT — https://github.com/redorav/hlslpp

```
MIT License

Copyright (c) 2017-2025 Emilio López

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### cereal

BSD 3-Clause — https://github.com/USCiLab/cereal

```
Copyright (c) 2013-2022, Randolph Voorhies, Shane Grant
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

### miniaudio

Public Domain (Unlicense) または MIT No Attribution — https://github.com/mackron/miniaudio

```
This software is available as a choice of the following licenses. Choose
whichever you prefer.

===============================================================================
ALTERNATIVE 1 - Public Domain (www.unlicense.org)
===============================================================================
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

===============================================================================
ALTERNATIVE 2 - MIT No Attribution
===============================================================================
Copyright 2025 David Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### Premake

BSD 3-Clause — https://github.com/premake/premake-core

### SDL

zlib License — https://github.com/libsdl-org/SDL

```
Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
```

```
BSD 3-Clause License

Copyright (c) 2002-2025 Jess Perkins and the Premake Project
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

### bgfx / bx / bimg

BSD 2-Clause License — https://github.com/bkaradzic/bgfx ・ https://github.com/bkaradzic/bx ・ https://github.com/bkaradzic/bimg
（3 つとも同じ条文です）

```
Copyright 2010-2026 Branimir Karadzic

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
```

### bgfx.cmake

CC0 1.0 Universal — https://github.com/bkaradzic/bgfx.cmake
（著作権を放棄したもので、条文の掲載義務はありません。全文は `External/bgfx.cmake/LICENSE` にあります）
