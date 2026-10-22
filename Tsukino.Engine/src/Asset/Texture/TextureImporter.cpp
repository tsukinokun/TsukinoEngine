//--------------------------------------------------------------
//! @file   TextureImporter.cpp
//! @brief  テクスチャのインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Texture/TextureImporter.hpp>
#include <Tsukino/Engine/Asset/Texture/TextureAsset.hpp>

#include "TextureAlphaUtility.hpp"

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <DirectXTex/DirectXTex.h>

#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {

    namespace {
        //--------------------------------------------------------------
        //! 線形のまま扱うべき（色ではない）テクスチャかを、ファイル名の末尾で判定します。
        //! 法線・メタリック/ラフネス・AO などは値そのものに意味があり、
        //! sRGB として読むとガンマで値が歪むため、名前の慣習（_n / _normal など）で見分けます。
        //! @param  [in] path テクスチャのパス
        //! @return 線形として扱うなら true
        //--------------------------------------------------------------
        bool IsLinearTexture(const Tsukino::Core::Path& path) {
            std::string stem = path.stem();
            for(char& c : stem) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            static const char* const kLinearSuffixes[] = {
                "_n", "_nrm", "_norm", "_normal", "_mr", "_orm", "_ao", "_metallic", "_roughness", "_rough", "_height", "_disp", "_mask",
            };
            for(const char* suffix : kLinearSuffixes) {
                const size_t length = std::strlen(suffix);
                if(stem.size() >= length && stem.compare(stem.size() - length, length, suffix) == 0) {
                    return true;
                }
            }
            return false;
        }
    }    // namespace
    //--------------------------------------------------------------
    //! @brief  テクスチャのインポート関数
    //--------------------------------------------------------------
    bool TextureImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // ルートからの絶対パスで開く
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inputPath;

        //--------------------------------------------------------------
        // テクスチャをDirectXTexで読み込むための変数
        //--------------------------------------------------------------
        DirectX::ScratchImage image;
        DirectX::TexMetadata  metadata;

        //--------------------------------------------------------------
        // 色のテクスチャは sRGB、法線などのデータのテクスチャは線形として扱う（ここが最重要）
        //--------------------------------------------------------------
        const bool        isLinear = IsLinearTexture(inputPath);
        const DirectX::WIC_FLAGS wicFlags = isLinear ? DirectX::WIC_FLAGS_IGNORE_SRGB : DirectX::WIC_FLAGS_FORCE_SRGB;

        HRESULT hr = DirectX::LoadFromWICFile(absoluteInputPath.ToWString().c_str(), wicFlags, &metadata, image);

        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to load texture: " + absoluteInputPath.string());
            return false;
        }

        //--------------------------------------------------------------
        // 透明ピクセルのRGBを近傍色で塗り広げる（バイリニア補間時の黒縁対策）
        //--------------------------------------------------------------
        Tsukino::Asset::Detail::DilateTransparentEdges(image);

        //--------------------------------------------------------------
        // outputDirectory(Cache/) に inputPath(相対) を結合して階層を維持
        // (inputPathが絶対パスの場合はToEngineRelativePath()で相対パスに戻してから結合する)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(inputPath);
        outputPath.replace_extension(".dds");    // 拡張子をDDSに変更

        //--------------------------------------------------------------
        // 親ディレクトリの生成
        //--------------------------------------------------------------
        // 出力先を作れないまま書き込みへ進むと、失敗が原因から遠い場所で
        // 「キャッシュが無い」として現れるため、ここで止める
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("TextureImporter: Failed to create the output directory: "
                                      + outputPath.parent_path().string());
            return false;
        }

        //--------------------------------------------------------------
        // DDSファイルとして保存
        //--------------------------------------------------------------
        //--------------------------------------------------------------
        // 色空間をフォーマットに刻んでから保存する。
        // DDS の旧形式のヘッダには sRGB を書く場所が無く、DDS_FLAGS_NONE で
        // 保存すると読み込み時に UNORM として扱われる。すると sRGB の値を
        // 線形の値として読んでしまい、sRGB のバックバッファへ書くときに
        // もう一度ガンマが掛かって、すべての色が白っぽく浮いて見えていた。
        // DX10 拡張ヘッダなら sRGB を保持できるので、常にそちらで書く
        //--------------------------------------------------------------
        const DXGI_FORMAT storedFormat = isLinear ? DirectX::MakeLinear(metadata.format) : DirectX::MakeSRGB(metadata.format);
        image.OverrideFormat(storedFormat);
        metadata.format = storedFormat;

        hr = DirectX::SaveToDDSFile(image.GetImages(), image.GetImageCount(), metadata, DirectX::DDS_FLAGS_FORCE_DX10_EXT, outputPath.ToWString().c_str());

        // 保存に失敗した場合はエラーログを出力して false を返す
        if(FAILED(hr)) {
            Tsukino::Core::Log::Error("Failed to save DDS: " + outputPath.string());
            return false;
        }

        // 成功した場合は情報ログを出力して true を返す
        Tsukino::Core::Log::Info("Texture imported: " + outputPath.string());

        return true;
    }

}    // namespace Tsukino::Asset
