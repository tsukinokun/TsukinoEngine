//--------------------------------------------------------------
//! @file   ShaderImporter.cpp
//! @brief  シェーダーインポーター
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Shader/ShaderImporter.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <d3dcompiler.h>
#include <fstream>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @brief  シェーダーアセットをインポートする関数
    //--------------------------------------------------------------
    bool ShaderImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // 絶対パスの取得
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inputPath;

        //--------------------------------------------------------------
        // 出力パスの決定
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / inputPath;
        outputPath.replace_extension(".cso");

        //--------------------------------------------------------------
        // 親ディレクトリ作成
        //--------------------------------------------------------------
        Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path());

        //------------------------------------------------
        // シェーダー種別を拡張子から判定
        //------------------------------------------------
        std::string ext = inputPath.string();
        std::string target;
        std::string entrypoint;    // エントリポイント名

        // 拡張子に基づいてシェーダーのターゲットを決定
        if(ext.ends_with(".vs.hlsl")) {
            target     = "vs_5_0";
            entrypoint = "VSMain";    // 頂点シェーダーのエントリポイント名
        } else if(ext.ends_with(".ps.hlsl")) {
            target     = "ps_5_0";
            entrypoint = "PSMain";    // ピクセルシェーダーのエントリポイント名
        } else {
            Tsukino::Core::Log::Error("Unknown shader type: " + ext);
            return false;
        }

        //------------------------------------------------
        // 出力パス
        //------------------------------------------------
        std::string name = inputPath.stem();    // xxx.vs + .cso

        //------------------------------------------------
        // コンパイルフラグ
        //------------------------------------------------
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        //------------------------------------------------
        // シェーダーコンパイル
        //------------------------------------------------
        ID3DBlob* shaderBlob = nullptr;
        ID3DBlob* errorBlob  = nullptr;

        HRESULT hr = D3DCompileFromFile(absoluteInputPath.ToWString().c_str(),
                                        nullptr,
                                        D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                        entrypoint.c_str(),
                                        target.c_str(),
                                        flags,
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
        std::ofstream file(outputPath.string(), std::ios::binary);
        if(!file)
            return false;
        file.write((char*)shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

        shaderBlob->Release();

        Tsukino::Core::Log::Info("Shader compiled: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset
