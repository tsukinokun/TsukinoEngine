//--------------------------------------------------------------
//! @file   ShaderImporter.cpp
//! @brief  シェーダーインポーター
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Shader/ShaderImporter.hpp>

#include <Tsukino/Core/Log.hpp>

#include <d3dcompiler.h>
#include <fstream>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief  シェーダーアセットをインポートする関数
    //--------------------------------------------------------------
    bool ShaderImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //------------------------------------------------
        // シェーダー種別を拡張子から判定
        //------------------------------------------------
        std::string ext = inputPath.extension();
        std::string target;

        // 拡張子に基づいてシェーダーのターゲットを決定
        if(ext == ".vs.hlsl") {
            target = "vs_5_0";
        } else if(ext == ".ps.hlsl") {
            target = "ps_5_0";
        } else {
            Tsukino::Core::Log::Error("Unknown shader type: " + ext);
            return false;
        }

        //------------------------------------------------
        // 出力パス
        //------------------------------------------------
        auto name = inputPath.stem();    // xxx.vs になるので注意
        // stem() は ".vs.hlsl" の ".vs" までしか取れないので修正
        name = name.substr(0, name.find_last_of('.'));

        Tsukino::Core::Path outputPath = outputDirectory / (name + ".cso");

        //------------------------------------------------
        // シェーダーコンパイル
        //------------------------------------------------
        ID3DBlob* shaderBlob = nullptr;
        ID3DBlob* errorBlob  = nullptr;

        HRESULT hr = D3DCompileFromFile(inputPath.ToWString().c_str(),
                                        nullptr,
                                        D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                        "main",
                                        target.c_str(),    // ← 自動判別したターゲット
                                        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
                                        0,
                                        &shaderBlob,
                                        &errorBlob);

        if(FAILED(hr)) {
            if(errorBlob) {
                Tsukino::Core::Log::Error((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }
            return false;
        }

        //------------------------------------------------
        // .cso 保存
        //------------------------------------------------
        std::ofstream file(outputPath.string(), std::ios::binary);                         // 書き込むファイルを開く
        file.write((char*)shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());    // ファイルに内容を書き込む
        file.close();

        shaderBlob->Release();

        Tsukino::Core::Log::Info("Shader imported: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset
