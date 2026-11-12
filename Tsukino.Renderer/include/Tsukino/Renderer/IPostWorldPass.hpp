//----------------------------------------------------------------------------
//! @file   IPostWorldPass.hpp
//! @brief  Worldパスの後に差し込む描画の受け口
//! @detail Tsukino.Renderer は上位モジュールのシステムを知りません。World パスの
//!         後に独自の描画を差し込みたい側がこのインターフェースを実装し、
//!         Renderer::Render() へ渡します。
//!         Tsukino::Physics::IPhysicsDebugDraw と同じ、下層にインターフェースを
//!         置いて上層が実装する形です。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>
#include <Tsukino/Renderer/NativeHandles.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! Worldパスの後に実行される追加描画の出力先
    //------------------------------------------------------------------------
    class IPostWorldPass {
    public:
        //! デストラクタ
        virtual ~IPostWorldPass() = default;

        //! Worldパスの直後に呼ばれる描画を実行します。
        //! @param  [in] graphics   描画 API の生のデバイス（DX11 では ID3D11Device* と ID3D11DeviceContext*）
        //! @param  [in] view       ビュー行列（カメラ）
        //! @param  [in] projection 射影行列
        virtual void RenderPostWorld(const NativeGraphicsContext&       graphics,
                                     const Tsukino::Core::Math::matrix& view,
                                     const Tsukino::Core::Math::matrix& projection) = 0;
    };

}    // namespace Tsukino::Renderer
