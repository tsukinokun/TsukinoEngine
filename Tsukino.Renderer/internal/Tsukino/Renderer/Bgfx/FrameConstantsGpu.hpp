//----------------------------------------------------------------------------
//! @file   FrameConstantsGpu.hpp
//! @brief  bgfx 版の FrameConstants の中身（Renderer の内部用）
//! @detail シーン定数（CBufferScene）を送るユニフォームと、今送るべき値を持ちます。
//!         DX11 版は Upload で定数バッファを書き換えて張っておけば済みましたが、
//!         bgfx のユニフォームは描画1回ごとの値なので、Upload では値を覚えるだけにして、
//!         描画コマンドを Submit する直前に Apply で送ります。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/FrameConstants.hpp>
#include <Tsukino/Renderer/Bgfx/UniformBlock.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! FrameConstants の中身です。
    //------------------------------------------------------------------------
    struct FrameConstants::Gpu {
        UniformBlock<CBufferScene> scene;      // シーン定数のユニフォーム（シェーダーの u_scene。Shaders/bgfx/Scene.sh）
        CBufferScene               current{};  // 最後に Upload した値（次の描画で送る）

        //! 最後に Upload した値を、次に Submit する描画へ送ります。
        void Apply() const {
            scene.Set(current);
        }
    };

}    // namespace Tsukino::Renderer
