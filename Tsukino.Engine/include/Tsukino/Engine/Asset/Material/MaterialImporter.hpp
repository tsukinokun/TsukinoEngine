//----------------------------------------------------------------------------
//! @file   MaterialImporter.hpp
//! @brief  マテリアルファイル（.tmat）のインポーター
//! @detail .tmat は「キー = 値」のテキストで書いたマテリアルです（.dfont と同じ書き方）。
//!         読み取った値を MaterialData にして、キャッシュへバイナリで書き出します。
//!
//!         # 書き方の例（# から行末まではコメント）
//!         ShadingModel = PBR              (PBR / Unlit / Toon)
//!         BaseColor    = 1.0, 0.92, 0.6, 1.0
//!         Emissive     = 0, 0, 0
//!         Metallic     = 1.0
//!         Roughness    = 0.3
//!         Specular     = 0.5
//!         AlphaCutoff  = 0
//!         AlbedoMap    = Assets/Textures/Albedo.png   (NormalMap / MetallicRoughnessMap / EmissiveMap / AoMap も同じ)
//!
//!         # ShadingModel = Toon のときだけ使う項目（0〜1）
//!         ToonThreshold    = 0.5               (明るい側と暗い側の境目)
//!         ToonSmoothness   = 0.05              (境目のぼかし幅。0 でくっきり)
//!         ToonShadeColor   = 0.5, 0.5, 0.5     (暗い側に掛ける色)
//!         ToonSpecularSize = 0                 (くっきりしたハイライトの大きさ。0 で無し)
//!
//!         テクスチャのパスはアセットのルートからのパスで書きます。書いていない項目は MaterialData の既定値です。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetImporter.hpp>

// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {

    //! マテリアルファイル（.tmat）を読み取り、MaterialData をキャッシュへ書き出すインポーターです。
    class MaterialImporter : public IAssetImporter {
    public:

        //! マテリアルファイルをインポートします。
        //! @param  [in] inPutPath       インポートする .tmat ファイルのパス
        //! @param  [in] outPutDirectory 出力先ディレクトリ
        //! @return 成功した場合は true
        [[nodiscard]]
        bool Import(const Tsukino::Core::Path& inPutPath, const Tsukino::Core::Path& outPutDirectory) override;
    };
}    // namespace Tsukino::Asset
