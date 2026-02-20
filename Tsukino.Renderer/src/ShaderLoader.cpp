//------------------------------------------------------------
//! @file   ShaderLoader.cpp
//! @brief  HLSLシェーダを読み込み、コンパイルし、Shaderオブジェクトを構築するクラス
//------------------------------------------------------------
#include "Tsukino/Renderer/ShaderLoader.hpp"
#include <fstream>
// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------
    //! @brief HLSL をコンパイルする内部関数
    //------------------------------------------------------------
    bool ShaderLoader::CompileShader(const std::wstring& filePath, const char* entryPoint, const char* target, Microsoft::WRL::ComPtr<ID3DBlob>& outBlob) {
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;    // 厳密なコンパイルを有効にするフラグ

#if defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG;    // デバッグ情報を埋め込むフラグ（デバッグビルドのみ）
#endif
        // コンパイルエラーの出力用バッファ
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        // HLSL ファイルをコンパイル
        HRESULT hr = D3DCompileFromFile(filePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target, flags, 0, &outBlob, &errorBlob);
        // コンパイル失敗時はエラーメッセージを出力して false を返す
        if(FAILED(hr)) {
            if(errorBlob) {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            }
            return false;
        }

        return true;
    }

    //------------------------------------------------------------
    //! @brief HLSLファイルからVS/PS を読み込み、Shader を構築する
    //------------------------------------------------------------
    bool ShaderLoader::LoadFromFile(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath, Shader& outShader) {
        //------------------------------------------------------------
        // VSコンパイル
        //------------------------------------------------------------
        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
        if(!CompileShader(vsPath, "VSMain", "vs_5_0", vsBlob)) {
            OutputDebugStringA("Vertex shader compile failed.\n");
            return false;
        }

        //------------------------------------------------------------
        // PSコンパイル
        //------------------------------------------------------------
        Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
        if(!CompileShader(psPath, "PSMain", "ps_5_0", psBlob)) {
            OutputDebugStringA("Pixel shader compile failed.\n");
            return false;
        }

        //------------------------------------------------------------
        // VS作成
        //------------------------------------------------------------
        HRESULT hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, outShader.m_vertexShader.GetAddressOf());
        if(FAILED(hr))
            return false;

        //------------------------------------------------------------
        // PS作成
        //------------------------------------------------------------
        hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, outShader.m_pixelShader.GetAddressOf());
        if(FAILED(hr))
            return false;

        //------------------------------------------------------------
        // 入力レイアウト作成（POSITION + COLOR の例）
        //------------------------------------------------------------
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
        };
        // 入力レイアウトを作成
        hr = device->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), outShader.m_inputLayout.GetAddressOf());
        if(FAILED(hr))
            return false;

        return true;
    }

}    // namespace Tsukino::Renderer
