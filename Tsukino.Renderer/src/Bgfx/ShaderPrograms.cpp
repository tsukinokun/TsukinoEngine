//----------------------------------------------------------------------------
//! @file   ShaderPrograms.cpp
//! @brief  bgfx のシェーダーとプログラムを作る関数の実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/Bgfx/ShaderPrograms.hpp>

#include <Tsukino/Engine/Asset/Shader/ShaderAsset.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <fstream>
#include <iterator>
#include <string>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! シェーダーのバイナリが bgfx の形式（shaderc の出力）かを確かめます。
    //------------------------------------------------------------------------
    bool IsBgfxShaderBinary(const std::vector<u8>& binary) {
        if(binary.size() < 4)
            return false;
        const bool known = binary[0] == 'V' || binary[0] == 'F' || binary[0] == 'C';
        return known && binary[1] == 'S' && binary[2] == 'H';
    }

    //------------------------------------------------------------------------
    //! シェーダーアセット2本（bgfx の形式）から、プログラムを作ります。
    //------------------------------------------------------------------------
    bgfx::ProgramHandle CreateBgfxProgram(const Tsukino::Asset::ShaderAsset* vs, const Tsukino::Asset::ShaderAsset* ps) {
        if(!vs || !ps || !IsBgfxShaderBinary(vs->binary) || !IsBgfxShaderBinary(ps->binary))
            return BGFX_INVALID_HANDLE;

        // 同じバイナリのシェーダーは bgfx がまとめて1つにするので、パスごとに作ってよい
        const bgfx::ShaderHandle vsHandle = bgfx::createShader(bgfx::copy(vs->binary.data(), static_cast<u32>(vs->binary.size())));
        const bgfx::ShaderHandle psHandle = bgfx::createShader(bgfx::copy(ps->binary.data(), static_cast<u32>(ps->binary.size())));
        return bgfx::createProgram(vsHandle, psHandle, true);    // シェーダーはプログラムと一緒に破棄する
    }

    //------------------------------------------------------------------------
    //! 組み込みの変換済みシェーダーを2本ファイルから読み込んで、プログラムを作ります。
    //------------------------------------------------------------------------
    bgfx::ProgramHandle LoadBuiltInBgfxProgram(const char* vsName, const char* psName) {
        const Tsukino::Core::Path folder =
            Tsukino::IO::FileSystem::GetEngineAssetRootPath() / ("Tsukino.BuiltIn/Assets/Shaders/bgfx/" + std::string(kBgfxShaderProfileDir));

        // 1本読んで bgfx のシェーダーにする
        auto loadShader = [&folder](const char* name) -> bgfx::ShaderHandle {
            const Tsukino::Core::Path path = folder / (std::string(name) + ".sc.bin");
            std::ifstream             file(path.string(), std::ios::binary);
            if(!file) {
                Tsukino::Core::Log::Error("ShaderPrograms: Shader not found: " + path.string());
                return BGFX_INVALID_HANDLE;
            }
            const std::vector<u8> binary{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
            if(!IsBgfxShaderBinary(binary)) {
                Tsukino::Core::Log::Error("ShaderPrograms: Not a bgfx shader binary: " + path.string());
                return BGFX_INVALID_HANDLE;
            }
            return bgfx::createShader(bgfx::copy(binary.data(), static_cast<u32>(binary.size())));
        };

        const bgfx::ShaderHandle vs = loadShader(vsName);
        const bgfx::ShaderHandle ps = loadShader(psName);
        if(!bgfx::isValid(vs) || !bgfx::isValid(ps)) {
            if(bgfx::isValid(vs))
                bgfx::destroy(vs);
            if(bgfx::isValid(ps))
                bgfx::destroy(ps);
            return BGFX_INVALID_HANDLE;
        }
        return bgfx::createProgram(vs, ps, true);
    }

}    // namespace Tsukino::Renderer
