//--------------------------------------------------------------
//! @file   AudioAsset.hpp
//! @brief  音声のアセットを管理するクラス
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <string>
// Tsukino::Asset 名前空間
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class AudioAsset
    //! @brief 音声のアセットを管理するクラス
    //! @note  音のファイル（.wav / .mp3 / .flac）は変換せずにそのまま使う。
    //!        読み込みと復号は再生する側（Tsukino::Audio::AudioManager）が行う
    //--------------------------------------------------------------
    class AudioAsset : public IAsset {
    public:
        //--------------------------------------------------------------
        //! @brief  ハンドルを取得する関数
        //! @return アセットのハンドル
        //--------------------------------------------------------------
        AssetHandle GetHandle() const override { return m_handle; }

        //--------------------------------------------------------------
        //! @brief  ハンドルを設定する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        void SetHandle(const AssetHandle& h) override { m_handle = h; }

        //--------------------------------------------------------------
        //! @brief  タイプを取得する関数
        //! @return アセットの種類
        //--------------------------------------------------------------
        AssetType GetType() const override { return AssetType::Audio; }

        std::string filePath;    // 音のファイルのパス（UTF-8）。AudioManager がここから読む

    private:
        AssetHandle m_handle;    // アセットハンドル
    };
}    // namespace Tsukino::Asset
