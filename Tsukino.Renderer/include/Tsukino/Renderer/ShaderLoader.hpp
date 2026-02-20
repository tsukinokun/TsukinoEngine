//------------------------------------------------------------
//! @file   ShaderLoader.hpp
//! @brief  HLSLシェーダを読み込み、コンパイルし、Shaderオブジェクトを構築するクラス
//! @author 山﨑愛
//------------------------------------------------------------
#pragma once
#include <string>
#include <wrl/client.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include "Shader.hpp"    // Shader クラスを利用する

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------
    //! @class  ShaderLoader
    //! @brief  HLSL ファイルを読み込み、Shader を構築するユーティリティクラス
    //------------------------------------------------------------
    class ShaderLoader {
    public:
        //------------------------------------------------------------
        // HLSLファイルからVS/PS を読み込み、Shader を構築する
        //! @param device    [in] DirectX11 デバイス
        //! @param vsPath    [in] 頂点シェーダー（.hlsl）へのパス
        //! @param psPath    [in] ピクセルシェーダー（.hlsl）へのパス
        //! @param outShader [in] 出力先の Shader オブジェクト
        //! @return true: 成功, false: 失敗
        //------------------------------------------------------------
        static bool LoadFromFile(ID3D11Device* device, const std::wstring& vsPath, const std::wstring& psPath, Shader& outShader);

    private:
        //------------------------------------------------------------
        // HLSL をコンパイルする内部関数
        //! @param filePath   [in] HLSL ファイルパス
        //! @param entryPoint [in] エントリーポイント（例: "main"）
        //! @param target     [in]ターゲットプロファイル（例: "vs_5_0"）
        //! @param outBlob    [in]コンパイル結果のバイトコード
        //! @return true: 成功, false: 失敗
        //------------------------------------------------------------
        static bool CompileShader(const std::wstring& filePath, const char* entryPoint, const char* target, Microsoft::WRL::ComPtr<ID3DBlob>& outBlob);
    };

}    // namespace Tsukino::Renderer
