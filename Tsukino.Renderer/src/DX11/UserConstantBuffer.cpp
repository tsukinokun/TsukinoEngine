//--------------------------------------------------------------
//! @file   UserConstantBuffer.cpp
//! @brief  ゲーム定義の定数バッファ生成の実装
//--------------------------------------------------------------
#include <Tsukino/Renderer/DX11/UserConstantBuffer.hpp>

namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! ゲーム定義の定数バッファを作ります。
    //--------------------------------------------------------------
    UserConstantBuffer CreateUserConstantBuffer(ID3D11Device* device, u32 byteSize) {
        UserConstantBuffer constantBuffer{};

        if(!device || byteSize == 0)
            return constantBuffer;

        // D3D11は定数バッファのサイズが16バイト境界であることを要求する
        const u32 alignedSize = (byteSize + 15u) & ~15u;

        //--------------------------------------------------------------
        // エンジン内蔵の定数バッファ（b0〜b9）と同じく DEFAULT + UpdateSubresource
        // で運用する。毎フレーム丸ごと書き換える使い方なので、
        // DYNAMIC + Map にしても得は無く、呼び出し側の手順が増えるだけ
        //--------------------------------------------------------------
        D3D11_BUFFER_DESC desc = {};
        desc.Usage             = D3D11_USAGE_DEFAULT;
        desc.ByteWidth         = alignedSize;
        desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;

        HRESULT hr = device->CreateBuffer(&desc, nullptr, constantBuffer.buffer.GetAddressOf());
        if(FAILED(hr))
            return constantBuffer;

        constantBuffer.byteSize = alignedSize;

        return constantBuffer;
    }

    //--------------------------------------------------------------
    //! ゲーム定義の定数バッファの中身を書き換えます。
    //--------------------------------------------------------------
    void UpdateUserConstantBuffer(ID3D11DeviceContext* context, const UserConstantBuffer& buffer, const void* data, u32 byteSize) {
        if(!context || !buffer.IsValid() || !data || byteSize == 0)
            return;

        // 確保量を超える書き込みは、バッファの外を壊すので行わない
        if(byteSize > buffer.byteSize)
            return;

        context->UpdateSubresource(buffer.buffer.Get(), 0, nullptr, data, 0, 0);
    }
}    // namespace Tsukino::Renderer
