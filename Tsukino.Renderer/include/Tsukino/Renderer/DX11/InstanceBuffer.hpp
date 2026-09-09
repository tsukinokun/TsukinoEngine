//--------------------------------------------------------------
//! @file   InstanceBuffer.hpp
//! @brief  インスタンス描画用バッファ構造体の宣言
//! @detail DrawCommand::instanceData へ渡す StructuredBuffer を作ります。
//!         頂点バッファ方式（PER_INSTANCE_DATA）ではなく StructuredBuffer に
//!         しているのは、入力レイアウトを頂点フォーマットごとに増やさずに済ませる
//!         ためです。VertexFormat は PipelineKey に含まれているので、
//!         インスタンス用の派生を作るとパイプラインのキャッシュが倍々に増えます。
//--------------------------------------------------------------
#pragma once
// windows.h の min/max マクロを避けるため、DX11 / Effekseer より先に通す
#include <Tsukino/Core/WindowsLean.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <d3d11.h>
#include <wrl/client.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! @struct InstanceBuffer
    //! @brief  インスタンスごとのデータを保持するバッファ
    //--------------------------------------------------------------
    struct InstanceBuffer {
        Microsoft::WRL::ComPtr<ID3D11Buffer>             buffer;              // 実体（DYNAMIC の StructuredBuffer）
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;                 // DrawCommand::instanceData へ渡すビュー
        u32                                              stride       = 0;    // 1インスタンスあたりのバイト数
        u32                                              capacity     = 0;    // 確保済みのインスタンス数
        u32                                              activeCount  = 0;    // 直近の Update で書き込んだインスタンス数

        //! バッファが使える状態かを返します。
        //! @return SRV まで作れていれば true
        [[nodiscard]]
        bool IsValid() const { return srv.Get() != nullptr; }
    };

    //--------------------------------------------------------------
    //! インスタンスバッファを作成します。
    //! @param  device     [in] DirectXのデバイス
    //! @param  stride     [in] 1インスタンスあたりのバイト数（シェーダー側の構造体と一致させること）
    //! @param  capacity   [in] 確保するインスタンス数
    //! @return 作成したバッファ。失敗した場合は IsValid() が false のものを返す
    //--------------------------------------------------------------
    [[nodiscard]]
    InstanceBuffer CreateInstanceBuffer(ID3D11Device* device, u32 stride, u32 capacity);

    //--------------------------------------------------------------
    //! インスタンスバッファの中身を書き換えます。
    //! @param  context  [in]     デバイスコンテキスト
    //! @param  buffer   [in,out] 書き込み先。activeCount が更新される
    //! @param  data     [in]     書き込むデータの先頭
    //! @param  count    [in]     書き込むインスタンス数（capacity を超えた分は切り捨てる）
    //! @return 実際に書き込んだインスタンス数
    //! @note   DYNAMIC + Map(WRITE_DISCARD) で毎フレーム丸ごと入れ替える前提です。
    //!         一部だけ書き換える用途には向きません
    //--------------------------------------------------------------
    u32 UpdateInstanceBuffer(ID3D11DeviceContext* context, InstanceBuffer& buffer, const void* data, u32 count);
}    // namespace Tsukino::Renderer
