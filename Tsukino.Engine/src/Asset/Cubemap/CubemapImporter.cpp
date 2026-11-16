//--------------------------------------------------------------
//! @file   CubemapImporter.cpp
//! @brief  キューブマップインポーターの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Cubemap/CubemapImporter.hpp>
#include <Tsukino/Engine/Asset/Cubemap/CubemapDesc.hpp>

#include "../Texture/ImageCodec.hpp"

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>

#include <array>
#include <sstream>
#include <string>

namespace Tsukino::Asset {

    bool CubemapImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // 拡張子チェック
        //--------------------------------------------------------------
        const std::string ext = Tsukino::Core::Path::ToLower(inputPath.extension());
        if(ext != ".cubemap") {
            Tsukino::Core::Log::Error("CubemapImporter: Unsupported format: " + ext);
            return false;
        }

        //--------------------------------------------------------------
        // .cubemapファイル（JSON）を読み込む
        //--------------------------------------------------------------
        const Tsukino::Core::Path assetRoot     = Tsukino::IO::FileSystem::GetAssetRootPath();
        const Tsukino::Core::Path fullInputPath = assetRoot / inputPath;

        CubemapDesc desc;
        {
            if(!Tsukino::IO::FileSystem::Exists(fullInputPath)) {
                Tsukino::Core::Log::Error("CubemapImporter: Failed to open: " + fullInputPath.string());
                return false;
            }
            std::istringstream       stream(Tsukino::IO::FileSystem::ReadText(fullInputPath));
            cereal::JSONInputArchive archive(stream);
            archive(desc);
        }

        // 面の順番：+X, -X, +Y, -Y, +Z, -Z（Direct3D・Metal・OpenGL で共通の並び）
        const std::array<std::string, 6> facePaths = {desc.px, desc.nx, desc.py, desc.ny, desc.pz, desc.nz};
        const std::array<std::string, 6> faceKeys  = {"px", "nx", "py", "ny", "pz", "nz"};

        //--------------------------------------------------------------
        // 6枚の画像を読み込む（同じ大きさであること）
        //--------------------------------------------------------------
        std::array<Detail::Rgba8Image, 6> faces;
        for(u32 i = 0; i < 6; ++i) {
            if(facePaths[i].empty()) {
                Tsukino::Core::Log::Error("CubemapImporter: Missing face path: " + faceKeys[i]);
                return false;
            }

            const Tsukino::Core::Path       faceFullPath = assetRoot / Tsukino::Core::Path(facePaths[i]);
            const std::vector<std::uint8_t> fileData     = Tsukino::IO::FileSystem::ReadBinary(faceFullPath);
            if(!Detail::DecodeImage(fileData.data(), fileData.size(), faces[i])) {
                Tsukino::Core::Log::Error("CubemapImporter: Failed to load face: " + faceFullPath.string());
                return false;
            }

            if(faces[i].width != faces[0].width || faces[i].height != faces[0].height) {
                Tsukino::Core::Log::Error("CubemapImporter: Face size mismatch at: " + faceKeys[i]);
                return false;
            }
        }

        //--------------------------------------------------------------
        // ブロック圧縮のため、4の倍数の大きさにそろえる
        //--------------------------------------------------------------
        const u32 width  = (faces[0].width + 3u) & ~3u;
        const u32 height = (faces[0].height + 3u) & ~3u;
        if(width != faces[0].width || height != faces[0].height) {
            for(Detail::Rgba8Image& face : faces) {
                face = Detail::ResizeImage(face, width, height, false);
            }
        }

        //--------------------------------------------------------------
        // ミップマップを作り、BC1 で圧縮する（スカイボックスはアルファ不要）。
        // 中身は KTX（拡張子は .tcc のまま。テクスチャと読み分けるため）
        //--------------------------------------------------------------
        Detail::ImageEncodeOptions options;
        options.srgb             = false;
        options.generateMips     = true;
        options.compressedFormat = m_rules ? m_rules->Resolve(Tsukino::GraphicsCommon::TextureFormat::BC1)
                                           : Tsukino::GraphicsCommon::TextureFormat::BC1;

        Tsukino::Core::Path tempPath = Tsukino::IO::FileSystem::ToEngineRelativePath(inputPath);
        tempPath.replace_extension(".tcc");
        const Tsukino::Core::Path outputPath = outputDirectory / tempPath;

        if(!Detail::WriteCubemapKtx(outputPath, faces, options)) {
            Tsukino::Core::Log::Error("CubemapImporter: Failed to save: " + outputPath.string());
            return false;
        }

        Tsukino::Core::Log::Info("CubemapImporter: Successfully imported cubemap: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset
