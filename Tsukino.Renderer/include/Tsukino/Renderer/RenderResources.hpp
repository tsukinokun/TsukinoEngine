//----------------------------------------------------------------------------
//! @file   RenderResources.hpp
//! @brief  描画で共有する資源の宣言
//! @detail どのパスからも、ゲーム側の System からも借りられる資源をまとめて持ちます。
//!         テクスチャ・バッファ・メッシュ・パイプラインの生成と、既定テクスチャ・
//!         プリミティブメッシュの貸し出しを担当します。
//!         資源の実体は描画 API ごとの中身（RenderResources::Gpu）が持ち、外へはハンドル
//!         （GpuHandles.hpp）や MeshBuffer で渡します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/Renderer/GpuHandles.hpp>
#include <Tsukino/Renderer/MeshBuffer.hpp>
#include <Tsukino/Renderer/PipelineFactory.hpp>

#include <Tsukino/GraphicsCommon/Mesh/PrimitiveType.hpp>
#include <Tsukino/GraphicsCommon/Texture/TextureFormat.hpp>

#include <memory>

namespace Tsukino::Asset {
    class TextureAsset;    // 前方宣言
}

// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    struct MeshData;    // 前方宣言
}    // namespace Tsukino::GraphicsCommon

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言（描画デバイスの持ち主。Renderer の内部専用）

    //------------------------------------------------------------------------
    //! 描画で共有する資源を持つクラスです。
    //! @note  Renderer が1つだけ所有し、Renderer::GetResources() で借りる
    //------------------------------------------------------------------------
    class RenderResources {
    public:
        struct Gpu;    //!< 描画 API の資源（Renderer の内部で定義）

        //! 資源を持たない状態で作ります（Initialize() で作成します）。
        RenderResources();

        //! 資源を破棄します。
        ~RenderResources();

        RenderResources(const RenderResources&)            = delete;
        RenderResources& operator=(const RenderResources&) = delete;

        //! 資源を作成します。
        //! @param  [in] graphicsContext デバイスとデバイスコンテキストの持ち主（このクラスより長生きさせること）
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(const GraphicsContext& graphicsContext);

        //! パイプラインの生成器を取得します。
        //! @return PipelineFactory へのポインタ
        [[nodiscard]]
        PipelineFactory* GetPipelineFactory();

        //! プリミティブメッシュを取得します。
        //! @param  [in] type 取得するプリミティブの種類
        //! @return メッシュバッファへのポインタ
        [[nodiscard]]
        MeshBuffer* GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType type);

        //! メッシュデータを GPU へ送り、メッシュバッファを作成します。
        //! @param  [in] meshData メッシュデータ
        //! @return メッシュバッファ（頂点が無いときはバッファの無いもの）
        [[nodiscard]]
        MeshBuffer CreateMesh(const Tsukino::GraphicsCommon::MeshData& meshData) const;

        //! テクスチャアセットのハンドルを取得します（初回はGPUへ転送してキャッシュします）。
        //! @param  [in,out] textureAsset 取得元のテクスチャアセット（GPU へ送ったあとピクセルデータを手放す）
        //! @return テクスチャのハンドル。作れなかったときは無効なハンドル
        [[nodiscard]]
        TextureHandle GetTexture(Tsukino::Asset::TextureAsset& textureAsset);

        //! テクスチャアセットを、描く前に GPU へ転送しておきます。
        //! @param  [in,out] textureAsset 転送するテクスチャアセット（GPU へ送ったあとピクセルデータを手放す）
        //! @note  読み込み画面などで先に送っておくと、初めて描くフレームで転送の待ちが出ない。
        //!        送ったあとは GetTexture() がキャッシュを返す
        void UploadTexture(Tsukino::Asset::TextureAsset& textureAsset) {
            (void)GetTexture(textureAsset);
        }

        //! 白テクスチャのハンドルを取得します。
        //! @return テクスチャのハンドル
        //! @note  マテリアルテクスチャ未設定時の既定値。
        //!        アルベド/MR/エミッシブ/AOはいずれもcbuffer定数との「乗算」で
        //!        合成するため、白(=1.0)を掛ければ定数値がそのまま残る
        [[nodiscard]]
        TextureHandle GetWhiteTexture() const;

        //! フラット法線テクスチャのハンドルを取得します。
        //! @return テクスチャのハンドル
        //! @note  ノーマルマップ未設定時の既定値。接空間の(0,0,1)を
        //!        エンコードした値(R=0x80,G=0x80,B=0xFF)で、これを適用しても
        //!        頂点法線がそのまま保たれる。白を使うと法線が斜めにずれる
        [[nodiscard]]
        TextureHandle GetFlatNormalTexture() const;

        //! 書き換えられる 2D テクスチャ（ミップマップ無し）を作成します。
        //! @param  [in] width    幅（ピクセル）
        //! @param  [in] height   高さ（ピクセル）
        //! @param  [in] format   ピクセルフォーマット（ブロック圧縮は使えない）
        //! @param  [in] data     最初の中身（nullptr ならすべて 0 で埋める）
        //! @param  [in] rowPitch data の1行のバイト数
        //! @return テクスチャのハンドル。作れなかったときは無効なハンドル
        //! @note  文字のアトラスのように、作ったあとで一部ずつ書き足すテクスチャに使う
        [[nodiscard]]
        TextureHandle CreateTexture(u32 width, u32 height, Tsukino::GraphicsCommon::TextureFormat format, const void* data, u32 rowPitch);

        //! CreateTexture() で作ったテクスチャの一部を書き換えます。
        //! @param  [in] texture  書き換えるテクスチャ
        //! @param  [in] x        書き込む矩形の左端（ピクセル）
        //! @param  [in] y        書き込む矩形の上端（ピクセル）
        //! @param  [in] width    書き込む矩形の幅（ピクセル）
        //! @param  [in] height   書き込む矩形の高さ（ピクセル）
        //! @param  [in] data     書き込むピクセル
        //! @param  [in] rowPitch data の1行のバイト数
        void UpdateTexture(TextureHandle texture, u32 x, u32 y, u32 width, u32 height, const void* data, u32 rowPitch);

        //! テクスチャを破棄します。
        //! @param  [in] texture 破棄するテクスチャ（無効なハンドルなら何もしない）
        //! @note  破棄したあとのハンドルは使わないこと（番号は別のテクスチャに使い回される）
        void DestroyTexture(TextureHandle texture);

        //! インスタンス描画用のバッファを作成します（DrawCommand::instanceData へ渡す）。
        //! @param  [in] stride   1インスタンスあたりのバイト数（シェーダー側の構造体と一致させること）
        //! @param  [in] capacity 確保するインスタンス数
        //! @return バッファのハンドル。作れなかったときは無効なハンドル
        [[nodiscard]]
        BufferHandle CreateInstanceBuffer(u32 stride, u32 capacity);

        //! インスタンス描画用のバッファの中身を書き換えます。
        //! @param  [in] buffer 書き込み先
        //! @param  [in] data   書き込むデータの先頭
        //! @param  [in] count  書き込むインスタンス数（確保した数を超えた分は切り捨てる）
        //! @return 実際に書き込んだインスタンス数
        //! @note  毎フレーム丸ごと入れ替える前提。一部だけ書き換える用途には向かない
        u32 UpdateInstanceBuffer(BufferHandle buffer, const void* data, u32 count);

        //! ゲーム定義の定数バッファを作成します（DrawCommand::userConstantBuffer へ渡す）。
        //! @param  [in] byteSize 必要なバイト数。16の倍数へ自動で切り上げる
        //! @return バッファのハンドル。作れなかったときは無効なハンドル
        [[nodiscard]]
        BufferHandle CreateUserConstantBuffer(u32 byteSize);

        //! ゲーム定義の定数バッファの中身を書き換えます。
        //! @param  [in] buffer   書き込み先
        //! @param  [in] data     書き込むデータの先頭
        //! @param  [in] byteSize 書き込むバイト数（確保量を超える分は書かない）
        void UpdateUserConstantBuffer(BufferHandle buffer, const void* data, u32 byteSize);

        //! バッファを破棄します。
        //! @param  [in] buffer 破棄するバッファ（無効なハンドルなら何もしない）
        //! @note  破棄したあとのハンドルは使わないこと（番号は別のバッファに使い回される）
        void DestroyBuffer(BufferHandle buffer);

        //! 描画 API の資源を取得します（Renderer の中で使う。中身の定義は公開しない）。
        //! @return 資源への参照
        [[nodiscard]]
        Gpu& GetGpu() noexcept {
            return *m_gpu;
        }

        //! 描画 API の資源を取得します（Renderer の中で使う。中身の定義は公開しない）。
        //! @return 資源への参照
        [[nodiscard]]
        const Gpu& GetGpu() const noexcept {
            return *m_gpu;
        }

    private:
        std::unique_ptr<Gpu> m_gpu;    // 描画 API の資源
    };
}    // namespace Tsukino::Renderer
