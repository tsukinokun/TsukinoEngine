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

struct ID3D11DeviceContext;    // 前方宣言

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
        //! @param  [in] dc         D3D11 デバイスコンテキスト
        //! @param  [in] view       ビュー行列（カメラ）
        //! @param  [in] projection 射影行列
        virtual void RenderPostWorld(ID3D11DeviceContext*               dc,
                                     const Tsukino::Core::Math::matrix& view,
                                     const Tsukino::Core::Math::matrix& projection) = 0;
    };

}    // namespace Tsukino::Renderer
