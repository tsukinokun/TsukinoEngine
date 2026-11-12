//--------------------------------------------------------------
//! @file   UserConstantBuffer.hpp
//! @brief  ゲーム定義の定数バッファ構造体の宣言
//! @detail DrawCommand::userConstantBuffer へ渡す定数バッファを作ります。
//!         ゲームが自前のシェーダーで何かを描くとき、そのシェーダーへ
//!         パラメータを渡すための唯一の口です。
//!
//!         これが無かった頃は、演出パラメータを1つ渡すだけで
//!         「CBSlotを1本足す・構造体を足す・Set関数を足す・Rendererに
//!         メンバとUpload関数を足す・Render()にフックを足す」という
//!         5点セットをエンジン側に書く必要があり、ゲーム固有の演出まで
//!         エンジンへ流れ込む原因になっていました。
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
    //! @struct UserConstantBuffer
    //! @brief  ゲームが自由に使える定数バッファ
    //--------------------------------------------------------------
    struct UserConstantBuffer {
        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;              // 実体
        u32                                  byteSize = 0;        // 確保したバイト数（16の倍数へ切り上げ済み）

        //! バッファが使える状態かを返します。
        //! @return 作成に成功していれば true
        [[nodiscard]]
        bool IsValid() const { return buffer.Get() != nullptr; }
    };

    //--------------------------------------------------------------
    //! ゲーム定義の定数バッファを作ります。
    //! @param  device   [in] DirectXのデバイス
    //! @param  byteSize [in] 必要なバイト数。16の倍数へ自動で切り上げる
    //! @return 作成したバッファ。失敗した場合は IsValid() が false のものを返す
    //! @note   D3D11は定数バッファのサイズが16バイト境界であることを要求する。
    //!         呼び出し側がそれを意識せずに済むよう、ここで切り上げる
    //--------------------------------------------------------------
    [[nodiscard]]
    UserConstantBuffer CreateUserConstantBuffer(ID3D11Device* device, u32 byteSize);

    //--------------------------------------------------------------
    //! ゲーム定義の定数バッファの中身を書き換えます。
    //! @param context  [in] デバイスコンテキスト
    //! @param buffer   [in] 書き込み先
    //! @param data     [in] 書き込むデータの先頭
    //! @param byteSize [in] 書き込むバイト数（確保量を超える分は書かない）
    //--------------------------------------------------------------
    void UpdateUserConstantBuffer(ID3D11DeviceContext* context, const UserConstantBuffer& buffer, const void* data, u32 byteSize);
}    // namespace Tsukino::Renderer
