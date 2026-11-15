//----------------------------------------------------------------------------
//! @file   IPostWorldPass.hpp
//! @brief  Worldパスの後に差し込む描画の受け口
//! @detail Tsukino.Renderer は上位モジュールのシステムを知りません。World パスの
//!         後に独自の描画を差し込みたい側がこのインターフェースを実装し、
//!         Renderer::Render() へ渡します。
//!         Tsukino::Physics::IPhysicsDebugDraw と同じ、下層にインターフェースを
//!         置いて上層が実装する形です。
//!         描画 API には依りません。描く先（3D の絵の上、深度つき）と描く部品（Renderer::GetEffectRenderer()）は
//!         Renderer が用意し、実装する側はカメラを受け取って描くだけです。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Math/Matrix.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! Worldパスの後に実行される追加描画の出力先
    //------------------------------------------------------------------------
    class IPostWorldPass {
    public:
        //! デストラクタ
        virtual ~IPostWorldPass() = default;

        //! 3D の絵を描き終えた後（UI の前）に呼ばれる描画を実行します。
        //! @param  [in] view       ビュー行列（カメラ）
        //! @param  [in] projection 射影行列（リバース Z）
        virtual void RenderPostWorld(const Tsukino::Core::Math::matrix& view, const Tsukino::Core::Math::matrix& projection) = 0;
    };

}    // namespace Tsukino::Renderer
