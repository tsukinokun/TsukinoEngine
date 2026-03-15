//------------------------------------------------------------
//! @file   SpriteRenderer.hpp
//! @brief  スプライト描画クラスの宣言
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    // 前方宣言
    class GraphicsContext;    
    class Material;          
    class MeshBuffer;
    //------------------------------------------------------------
    //! @class  SpriteRenderer
    //! @brief  スプライト描画クラス
    //------------------------------------------------------------
    class SpriteRenderer {
    public:
        //------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //------------------------------------------------------------
        SpriteRenderer() = default;

        //------------------------------------------------------------
        //! @brief sprite描画を行う関数
        //! @param gfx       [in] グラフィックスコンテキストへのポインタ
        //! @param material  [in] 描画に使用するマテリアルへのポインタ
        //! @param mesh      [in] 描画に使用するメッシュデータへのポインタ
        //------------------------------------------------------------
        void Draw(GraphicsContext* gfx, Material* material, MeshBuffer* mesh);
    };

}    // namespace Tsukino::Renderer
