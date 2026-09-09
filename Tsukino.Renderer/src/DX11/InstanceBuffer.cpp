//--------------------------------------------------------------
//! @file   InstanceBuffer.cpp
//! @brief  インスタンス描画用バッファ生成の実装
//--------------------------------------------------------------
#include <Tsukino/Renderer/DX11/InstanceBuffer.hpp>

#include <cstring>

namespace Tsukino::Renderer {
    //--------------------------------------------------------------
    //! インスタンスバッファを作成します。
    //--------------------------------------------------------------
    InstanceBuffer CreateInstanceBuffer(ID3D11Device* device, u32 stride, u32 capacity) {
        InstanceBuffer instanceBuffer{};

        if(!device || stride == 0 || capacity == 0)
            return instanceBuffer;

        //--------------------------------------------------------------
        // バッファ本体の作成。
        // 毎フレーム丸ごと書き換える前提なので DYNAMIC + CPU_ACCESS_WRITE にする
        //--------------------------------------------------------------
        D3D11_BUFFER_DESC desc  = {};
        desc.Usage              = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth          = stride * capacity;
        desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;

        // StructuredBuffer として見せるための2つ。これが無いとSRVの作成で失敗する
        desc.MiscFlags              = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride    = stride;

        HRESULT hr = device->CreateBuffer(&desc, nullptr, instanceBuffer.buffer.GetAddressOf());
        if(FAILED(hr))
            return instanceBuffer;

        //--------------------------------------------------------------
        // シェーダーリソースビューの作成。
        // StructuredBuffer なのでフォーマットは UNKNOWN で、要素数だけを伝える
        //--------------------------------------------------------------
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format                          = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement             = 0;
        srvDesc.Buffer.NumElements              = capacity;

        hr = device->CreateShaderResourceView(instanceBuffer.buffer.Get(), &srvDesc, instanceBuffer.srv.GetAddressOf());
        if(FAILED(hr)) {
            // SRVが作れなければバッファだけ持っていても使えないので、丸ごと捨てる
            instanceBuffer.buffer.Reset();
            return instanceBuffer;
        }

        instanceBuffer.stride   = stride;
        instanceBuffer.capacity = capacity;

        return instanceBuffer;
    }

    //--------------------------------------------------------------
    //! インスタンスバッファの中身を書き換えます。
    //--------------------------------------------------------------
    u32 UpdateInstanceBuffer(ID3D11DeviceContext* context, InstanceBuffer& buffer, const void* data, u32 count) {
        buffer.activeCount = 0;

        if(!context || !buffer.IsValid() || !data || count == 0)
            return 0;

        // 確保量を超える書き込みは切り捨てる（呼び出し側の増減で落ちないようにする）
        const u32 writeCount = (count < buffer.capacity) ? count : buffer.capacity;

        D3D11_MAPPED_SUBRESOURCE mapped = {};

        // WRITE_DISCARD は「古い中身は捨てる」宣言。GPUが前フレームの描画に
        // まだ使っていても、ドライバが別領域を返すので待ちが発生しない
        HRESULT hr = context->Map(buffer.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if(FAILED(hr))
            return 0;

        std::memcpy(mapped.pData, data, static_cast<size_t>(buffer.stride) * writeCount);

        context->Unmap(buffer.buffer.Get(), 0);

        buffer.activeCount = writeCount;

        return writeCount;
    }
}    // namespace Tsukino::Renderer
