//----------------------------------------------------------------------------
//! @file   EffectFileInterface.hpp
//! @brief  Effekseer がテクスチャなどを読むときのファイルの入口（EngineIntegration の内部用）
//! @detail Effekseer が渡してくるパスを、エフェクトのファイルの置き場所とアセットのフォルダから探し、
//!         エンジンの FileSystem で読みます（Android の APK の中も同じ経路で読めます）。
//!         EffectSystem だけが使うので、公開ヘッダには置きません。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/Path.hpp>

#include <Effekseer.h>

#include <cstdint>
#include <vector>

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {

    //------------------------------------------------------------------------
    //! Effekseer のファイルの入口です。読み込みだけに対応します。
    //------------------------------------------------------------------------
    class EffectFileInterface : public Effekseer::FileInterface {
    public:
        //! 探す場所の先頭に足すフォルダを設定します。
        //! @param  [in] baseDirectory 再生するエフェクトのファイルがあるフォルダ
        void SetBaseDirectory(const Tsukino::Core::Path& baseDirectory);

        //! ファイルを探して、中身を丸ごと読んだ読み手を返します。
        //! @param  [in] path Effekseer が渡すパス（UTF-16）
        //! @return 読み手。見つからなければ空
        Effekseer::FileReaderRef OpenRead(const EFK_CHAR* path) override;

        //! 書き込みには対応しません。
        //! @param  [in] path 使わない
        //! @return 常に空
        Effekseer::FileWriterRef OpenWrite(const EFK_CHAR* path) override;

    private:
        Tsukino::Core::Path m_baseDirectory;    // 探す場所の先頭に足すフォルダ
    };

    //------------------------------------------------------------------------
    //! 読んだファイルの中身を、Effekseer へ順に渡す読み手です。
    //------------------------------------------------------------------------
    class EffectFileReader : public Effekseer::FileReader {
    public:
        //! 読んだ中身を受け取ります。
        //! @param  [in] data ファイルの中身
        explicit EffectFileReader(std::vector<uint8_t> data);

        //! 今の位置から読みます。
        //! @param  [out] buffer 読んだ中身の書き込み先
        //! @param  [in]  size   読みたいバイト数
        //! @return 実際に読んだバイト数
        size_t Read(void* buffer, size_t size) override;

        //! 読む位置を移します（範囲の外なら何もしない）。
        //! @param  [in] position 先頭からのバイト数
        void Seek(int position) override;

        //! 今の読む位置を返します。
        //! @return 先頭からのバイト数
        int GetPosition() const override;

        //! 中身の大きさを返します。
        //! @return バイト数
        size_t GetLength() const override;

    private:
        std::vector<uint8_t> m_data;            // ファイルの中身
        size_t               m_position = 0;    // 今の読む位置
    };

}    // namespace Tsukino::EngineIntegration
