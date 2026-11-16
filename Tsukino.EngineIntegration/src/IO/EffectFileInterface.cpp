//----------------------------------------------------------------------------
//! @file   EffectFileInterface.cpp
//! @brief  Effekseer がテクスチャなどを読むときのファイルの入口の実装
//----------------------------------------------------------------------------
#include "EffectFileInterface.hpp"

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <algorithm>
#include <cstring>
#include <string>

// 名前空間 : Tsukino::EngineIntegration
namespace Tsukino::EngineIntegration {

    //------------------------------------------------------------------------
    //! 探す場所の先頭に足すフォルダを設定します。
    //------------------------------------------------------------------------
    void EffectFileInterface::SetBaseDirectory(const Tsukino::Core::Path& baseDirectory) {
        m_baseDirectory = baseDirectory;
    }

    //------------------------------------------------------------------------
    //! ファイルを探して、中身を丸ごと読んだ読み手を返します。
    //------------------------------------------------------------------------
    Effekseer::FileReaderRef EffectFileInterface::OpenRead(const EFK_CHAR* path) {
        if(!path) {
            Tsukino::Core::Log::Error("EffectFileInterface: The path is null.");
            return nullptr;
        }

        const size_t      utf16Length = std::char_traits<char16_t>::length(path);
        std::vector<char> utf8(utf16Length * 4 + 1, 0);
        Effekseer::ConvertUtf16ToUtf8(utf8.data(), static_cast<int32_t>(utf8.size()), path);

        const Tsukino::Core::Path requested(utf8.data());
        const std::string         fileName = requested.filename();

        //--------------------------------------------------------------------
        // エフェクトのファイルは、テクスチャをエフェクト作成時のパスで指すので、
        // そのままのパスとファイル名だけの両方を、エフェクトの隣とアセットのフォルダで探す
        //--------------------------------------------------------------------
        std::vector<Tsukino::Core::Path> candidates;
        if(!m_baseDirectory.string().empty()) {
            candidates.push_back(m_baseDirectory / requested);
            candidates.push_back(m_baseDirectory / Tsukino::Core::Path(fileName));
        }
        candidates.emplace_back("Assets/Effects/Texture/" + fileName);
        candidates.emplace_back("Assets/Texture/" + fileName);
        candidates.emplace_back("Texture/" + fileName);
        candidates.push_back(Tsukino::Core::Path("Assets") / requested);
        candidates.push_back(requested);

        for(const Tsukino::Core::Path& candidate : candidates) {
            if(Tsukino::IO::FileSystem::Exists(candidate)) {
                return Effekseer::MakeRefPtr<EffectFileReader>(Tsukino::IO::FileSystem::ReadBinary(candidate));
            }
        }

        Tsukino::Core::Log::Error("EffectFileInterface: File not found: " + requested.string());
        return nullptr;
    }

    //------------------------------------------------------------------------
    //! 書き込みには対応しません。
    //------------------------------------------------------------------------
    Effekseer::FileWriterRef EffectFileInterface::OpenWrite(const EFK_CHAR* /*path*/) {
        return nullptr;
    }

    //------------------------------------------------------------------------
    //! 読んだ中身を受け取ります。
    //------------------------------------------------------------------------
    EffectFileReader::EffectFileReader(std::vector<uint8_t> data)
        : m_data(std::move(data)) {
    }

    //------------------------------------------------------------------------
    //! 今の位置から読みます。
    //------------------------------------------------------------------------
    size_t EffectFileReader::Read(void* buffer, size_t size) {
        if(m_position >= m_data.size()) {
            return 0;
        }
        const size_t count = std::min(size, m_data.size() - m_position);
        std::memcpy(buffer, m_data.data() + m_position, count);
        m_position += count;
        return count;
    }

    //------------------------------------------------------------------------
    //! 読む位置を移します（範囲の外なら何もしない）。
    //------------------------------------------------------------------------
    void EffectFileReader::Seek(int position) {
        if(position >= 0 && static_cast<size_t>(position) <= m_data.size()) {
            m_position = static_cast<size_t>(position);
        }
    }

    //------------------------------------------------------------------------
    //! 今の読む位置を返します。
    //------------------------------------------------------------------------
    int EffectFileReader::GetPosition() const {
        return static_cast<int>(m_position);
    }

    //------------------------------------------------------------------------
    //! 中身の大きさを返します。
    //------------------------------------------------------------------------
    size_t EffectFileReader::GetLength() const {
        return m_data.size();
    }

}    // namespace Tsukino::EngineIntegration
