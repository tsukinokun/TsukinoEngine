//----------------------------------------------------------------------------
//! @file   FullscreenPass.hpp
//! @brief  画面全体を覆う三角形の描画の宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。ライティング・フォグ・モーションブラー・
//!         トーンマップ・IBLベイクが共用する、頂点バッファを持たないフルスクリーン三角形の
//!         頂点シェーダーと描画処理を持ちます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>

#include <wrl/client.h>
#include <d3d11.h>

namespace Tsukino::Asset {
    class ShaderAsset;    // 前方宣言
}

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 画面全体を覆う三角形を描くクラスです。
    //! @note  頂点シェーダーは SV_VertexID だけで3頂点を作るので、頂点バッファも
    //!        インデックスバッファも持たない
    //------------------------------------------------------------------------
    class FullscreenPass {
    public:

        //! 頂点シェーダーを作成します。
        //! @param  [in] device 描画デバイス
        //! @param  [in] vs     フルスクリーン三角形用の頂点シェーダー（ビルトインの tonemapVS）
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(ID3D11Device* device, const Tsukino::Asset::ShaderAsset* vs);

        //! 頂点シェーダーが使える状態かを取得します。
        //! @return true: 使える, false: 作成に失敗している
        [[nodiscard]]
        bool IsValid() const noexcept {
            return m_vs != nullptr;
        }

        //! 頂点シェーダーを取得します。
        //! @return フルスクリーン三角形用の頂点シェーダー
        [[nodiscard]]
        ID3D11VertexShader* GetVertexShader() const noexcept {
            return m_vs.Get();
        }

        //! 頂点バッファ・インデックスバッファを外し、トポロジーを三角形リストにします。
        //! @param  [in] context デバイスコンテキスト
        //! @note   同じ設定のまま何度も描く（IBLベイクの面ごとのループ）ときは、
        //!         これを1回呼んでから context->Draw(3, 0) を繰り返す
        static void BindGeometry(ID3D11DeviceContext* context);

        //! フルスクリーン三角形を1枚描きます（BindGeometry の後に Draw(3, 0) します）。
        //! @param  [in] context デバイスコンテキスト
        //! @note   シェーダー・入力テクスチャ・ステートは呼び出し側で設定しておくこと
        static void Draw(ID3D11DeviceContext* context);

    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vs;    // フルスクリーン三角形用の頂点シェーダー
    };
}    // namespace Tsukino::Renderer
