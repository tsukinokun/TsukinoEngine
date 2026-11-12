//----------------------------------------------------------------------------
//! @file   RenderResourcesGpu.hpp
//! @brief  RenderResources の DX11 の中身（Renderer の内部用）
//! @detail 共通ステート・サンプラー・既定テクスチャ・テクスチャとバッファの表・
//!         プリミティブメッシュ・PipelineFactory と、SpriteFont / SpriteBatch の生成を持ちます。
//!         RenderResources の公開関数は、ここへそのまま委ねます。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <Tsukino/Renderer/RenderResources.hpp>
#include <Tsukino/Renderer/DX11/InstanceBuffer.hpp>
#include <Tsukino/Renderer/DX11/UserConstantBuffer.hpp>

#include <Tsukino/GraphicsCommon/State/SamplerType.hpp>

#include <wrl/client.h>
#include <d3d11.h>
#include <SpriteFont.h>
#include <CommonStates.h>

#include <array>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

// 名前空間 : Tsukino::GraphicsCommon
namespace Tsukino::GraphicsCommon {
    struct MeshData;    // 前方宣言
}    // namespace Tsukino::GraphicsCommon

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class GraphicsContext;    // 前方宣言

    //------------------------------------------------------------------------
    //! RenderResources の DX11 の資源です。
    //! @note  デバイスとデバイスコンテキストは GraphicsContext が所有しており、
    //!        ここでは借りているだけ（Renderer がこのクラスより長生きさせる）
    //------------------------------------------------------------------------
    struct RenderResources::Gpu {

        //! 資源を作成します。
        //! @param  [in] graphicsContext デバイスとデバイスコンテキストの持ち主
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(const GraphicsContext& graphicsContext);

        //! パイプラインの生成器を取得します。
        //! @return PipelineFactory へのポインタ
        [[nodiscard]]
        PipelineFactory* GetPipelineFactory() {
            return &m_pipelineFactory.value();
        }

        //! DirectXTK の共通ステートを取得します。
        //! @return 共通ステートへのポインタ
        [[nodiscard]]
        DirectX::CommonStates* GetCommonStatesTK() const {
            return m_commonStates.get();
        }

        //! シザー（切り取り）を有効にしたラスタライザステートを取得します。
        //! @return カリング無し・シザー有効のラスタライザステート
        //! @note   DrawCommand::hasClipRect のコマンドと、文字の切り取りで使う
        [[nodiscard]]
        ID3D11RasterizerState* GetScissorRasterizerState() const {
            return m_scissorRasterizer.Get();
        }

        //! サンプラーを取得します。
        //! @param  [in] type 取得するサンプラーの種類
        //! @return サンプラーステートへのポインタ
        [[nodiscard]]
        ID3D11SamplerState* GetSampler(Tsukino::GraphicsCommon::SamplerType type) const {
            return m_samplers[static_cast<size_t>(type)].Get();
        }

        //! プリミティブメッシュを取得します。
        //! @param  [in] type 取得するプリミティブの種類
        //! @return メッシュバッファへのポインタ
        [[nodiscard]]
        MeshBuffer* GetPrimitiveMesh(Tsukino::GraphicsCommon::PrimitiveType type) {
            return &m_primitiveMeshes[static_cast<size_t>(type)];
        }

        //! テクスチャアセットのハンドルを取得します（初回はGPUへ転送してキャッシュします）。
        //! @param  [in,out] textureAsset 取得元のテクスチャアセット（GPU へ送ったあとピクセルデータを手放す）
        //! @return テクスチャのハンドル。作れなかったときは無効なハンドル
        [[nodiscard]]
        TextureHandle GetTexture(Tsukino::Asset::TextureAsset& textureAsset);

        //! 白テクスチャのハンドルを取得します。
        //! @return テクスチャのハンドル
        //! @note  マテリアルテクスチャ未設定時の既定値。
        //!        アルベド/MR/エミッシブ/AOはいずれもcbuffer定数との「乗算」で
        //!        合成するため、白(=1.0)を掛ければ定数値がそのまま残る
        [[nodiscard]]
        TextureHandle GetWhiteTexture() const {
            return m_whiteTexture;
        }

        //! フラット法線テクスチャのハンドルを取得します。
        //! @return テクスチャのハンドル
        //! @note  ノーマルマップ未設定時の既定値。接空間の(0,0,1)を
        //!        エンコードした値(R=0x80,G=0x80,B=0xFF)で、これを適用しても
        //!        頂点法線がそのまま保たれる。白を使うと法線が斜めにずれる
        [[nodiscard]]
        TextureHandle GetFlatNormalTexture() const {
            return m_flatNormalTexture;
        }

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

        //! 外で作られたテクスチャのビューを、ハンドルで指せるように登録します（DX11 版だけの機能）。
        //! @param  [in] view 登録するビュー（参照を1つ持つので、元の持ち主が先に手放してもよい）
        //! @return テクスチャのハンドル
        //! @note  DirectXTK の SpriteFont が持つ文字の画像に使う。段階的に無くす予定の口
        [[nodiscard]]
        TextureHandle RegisterExternalTexture(ID3D11ShaderResourceView* view);

        //! ハンドルが指すテクスチャのビューを取得します（Renderer の中で描くときに使う）。
        //! @param  [in] texture テクスチャのハンドル
        //! @return ビュー。無効なハンドルなら nullptr
        [[nodiscard]]
        ID3D11ShaderResourceView* ResolveTexture(TextureHandle texture) const;

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

        //! インスタンス描画用のバッファのビューを取得します（Renderer の中で描くときに使う）。
        //! @param  [in] buffer バッファのハンドル
        //! @return ビュー。無効なハンドルか、インスタンス用でないときは nullptr
        [[nodiscard]]
        ID3D11ShaderResourceView* ResolveInstanceBuffer(BufferHandle buffer) const;

        //! ゲーム定義の定数バッファを取得します（Renderer の中で描くときに使う）。
        //! @param  [in] buffer バッファのハンドル
        //! @return 定数バッファ。無効なハンドルか、定数バッファでないときは nullptr
        [[nodiscard]]
        ID3D11Buffer* ResolveUserConstantBuffer(BufferHandle buffer) const;

        //! スプライトフォントを作成します。
        //! @param  [in] data フォントデータのバイナリ
        //! @param  [in] size フォントデータのサイズ
        //! @return 作成した SpriteFont
        [[nodiscard]]
        std::unique_ptr<DirectX::SpriteFont> CreateSpriteFont(const u8* data, size_t size) const;

        //! メッシュデータを GPU へ送り、メッシュバッファを作成します。
        //! @param  [in] meshData メッシュデータ
        //! @return メッシュバッファ
        [[nodiscard]]
        MeshBuffer CreateMesh(const Tsukino::GraphicsCommon::MeshData& meshData) const;

        //! スプライトバッチを作成します。
        //! @return 作成した SpriteBatch
        [[nodiscard]]
        std::unique_ptr<DirectX::SpriteBatch> CreateSpriteBatch() const;


        //! サンプラーを作成します。
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool CreateSamplers();

        //! プリミティブメッシュを作成します。
        void CreatePrimitiveMeshes();

        //! マテリアル用の既定テクスチャ（白・フラット法線）を作成します。
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool CreateDefaultTextures();

        //! 1x1 のテクスチャを作成します。
        //! @param  [in]  rgba       ピクセル値。R8G8B8A8_UNORM はメモリ上のバイト順が
        //!                          R,G,B,A なので、リトルエンディアンでは 0xAABBGGRR と書く
        //!                          （例: フラット法線 R=0x80,G=0x80,B=0xFF,A=0xFF → 0xFFFF8080）
        //! @param  [out] outTexture 作成したテクスチャのハンドル
        //! @param  [in]  debugName  失敗時のログに出す名前
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Create1x1Texture(u32 rgba, TextureHandle& outTexture, const char* debugName);

        //! テクスチャのビューを表へ入れ、ハンドルを割り当てます。
        //! @param  [in] view 入れるビュー（表が参照を1つ持つ）
        //! @return テクスチャのハンドル。表が一杯なら無効なハンドル
        [[nodiscard]]
        TextureHandle AddTextureSlot(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view);

        //------------------------------------------------------------------------
        //! バッファの表の1枠です。インスタンス用と定数用のどちらか一方だけが有効になります。
        //------------------------------------------------------------------------
        struct BufferSlot {
            InstanceBuffer     instance;    // インスタンス描画用のバッファ
            UserConstantBuffer constant;    // ゲーム定義の定数バッファ
        };

        //! バッファを表へ入れ、ハンドルを割り当てます。
        //! @param  [in] slot 入れるバッファ
        //! @return バッファのハンドル。表が一杯なら無効なハンドル
        [[nodiscard]]
        BufferHandle AddBufferSlot(BufferSlot slot);

        //! ハンドルが指すバッファの枠を取得します。
        //! @param  [in] buffer バッファのハンドル
        //! @return 枠へのポインタ。無効なハンドルなら nullptr
        [[nodiscard]]
        const BufferSlot* FindBufferSlot(BufferHandle buffer) const;

        ID3D11Device*        m_device  = nullptr;    // 描画デバイス（GraphicsContext から借りている）
        ID3D11DeviceContext* m_context = nullptr;    // デバイスコンテキスト（同上）

        std::optional<PipelineFactory>         m_pipelineFactory;    // デバイスが決まってから構築するため optional
        std::unique_ptr<DirectX::CommonStates> m_commonStates;       // DirectXTK の共通ステート
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_scissorRasterizer;    // カリング無し・シザー有効のラスタライザ

        std::array<Microsoft::WRL::ComPtr<ID3D11SamplerState>, static_cast<size_t>(Tsukino::GraphicsCommon::SamplerType::Count)> m_samplers;
        std::array<MeshBuffer, static_cast<size_t>(Tsukino::GraphicsCommon::PrimitiveType::Count)>                              m_primitiveMeshes;

        TextureHandle m_whiteTexture;         // 白テクスチャ
        TextureHandle m_flatNormalTexture;    // フラット法線テクスチャ

        std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_textures;        // テクスチャの表（ハンドルの番号で引く。空きは nullptr）
        std::vector<u16>                                              m_freeTextures;    // 空いているテクスチャの番号
        std::vector<BufferSlot>                                       m_buffers;         // バッファの表（ハンドルの番号で引く）
        std::vector<u16>                                              m_freeBuffers;     // 空いているバッファの番号

        std::unordered_map<u64, TextureHandle> m_textureCache;    // AssetHandle の値をキーにしたテクスチャのキャッシュ
    };
}    // namespace Tsukino::Renderer
