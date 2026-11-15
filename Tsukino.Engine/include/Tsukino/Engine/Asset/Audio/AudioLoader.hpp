//--------------------------------------------------------------
//! @file   AudioLoader.hpp
//! @brief  オーディオアセットローダーの宣言
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAssetLoader.hpp>

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class  AudioLoader
    //! @brief  オーディオアセットのローダー（.wav / .mp3 / .flac をそのまま使う。変換もキャッシュもしない）
    //--------------------------------------------------------------
    class AudioLoader : public IAssetLoader {
    public:
        //--------------------------------------------------------------
        //! @brief デフォルトコンストラクタ
        //--------------------------------------------------------------
        AudioLoader() = default;

        //--------------------------------------------------------------
        // 対応する拡張子か判定する
        //! @param  ext [in] 拡張子
        //! @return 対応している場合は true
        //--------------------------------------------------------------
        [[nodiscard]]
        bool CanLoad(const std::string& ext) const override;

        //--------------------------------------------------------------
        // オーディオファイルから AudioAsset を生成する
        //! @param  path [in] オーディオファイルのパス
        //! @return 生成した AudioAsset（ファイルが無いときは nullptr）
        //--------------------------------------------------------------
        [[nodiscard]]
        Tsukino::Core::Ref<IAsset> Load(const Tsukino::Core::Path& path) override;
    };

}    // namespace Tsukino::Asset
