//--------------------------------------------------------------
//! @file   AudioAsset.hpp
//! @brief  音声のアセットを管理するクラス
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/IAsset.hpp>
#include <Tsukino/Core/typedef.hpp>

#include <string>
// Tsukino::Asset 名前空間
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @class AudioAsset
    //! @brief .xwbファイル内の音声データのメタデータを表す構造体
    //--------------------------------------------------------------
    struct XWBEntry {
        u32 offset;
        u32 length;
        u32 sampleRate;
        u16 channels;
        u16 formatTag;
    };

    //--------------------------------------------------------------
    //! @class AudioAsset
    //! @brief 音声のアセットを管理するクラス
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

        XWBEntry    metadata;        // 音声データのオフセット、長さ、サンプルレート、チャンネル数、フォーマットタグなど
        std::string waveBankPath;    // どの .xwb ファイルに属しているか

    private:
        AssetHandle m_handle;    // アセットハンドル
    };
}    // namespace Tsukino::Asset
