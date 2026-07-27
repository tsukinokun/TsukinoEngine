#pragma once
#include <Effekseer.h>
#include <Tsukino/Core/Path.hpp>
#include <Tsukino/Core/Log.hpp>
#include <memory>
#include <vector>

namespace Tsukino::EngineIntegration {

class EffectFileInterface : public Effekseer::FileInterface {
public:
    EffectFileInterface() = default;
    ~EffectFileInterface() override = default;

    void SetBaseDirectory(const Tsukino::Core::Path& baseDir);

    Effekseer::FileReaderRef OpenRead(const EFK_CHAR* path) override;
    Effekseer::FileWriterRef OpenWrite(const EFK_CHAR* path) override;

private:
    static std::vector<uint8_t> ReadFile(const std::string& path);
    static bool FileExists(const std::string& path);

    Tsukino::Core::Path m_baseDirectory;
};

class EffectFileReader : public Effekseer::FileReader {
public:
    EffectFileReader(std::vector<uint8_t> data)
        : m_data(std::move(data))
        , m_position(0) {}

    size_t Read(void* buffer, size_t size) override {
        if(m_position >= m_data.size()) {
            Tsukino::Core::Log::Info("[EffectFileReader::Read] position at end, returning 0");
            return 0;
        }
        size_t remaining = m_data.size() - m_position;
        size_t toCopy = std::min(size, remaining);
        std::memcpy(buffer, m_data.data() + m_position, toCopy);
        m_position += toCopy;
        Tsukino::Core::Log::Info("[EffectFileReader::Read] Read " + std::to_string(toCopy) + " bytes, position now " + std::to_string(m_position));
        return toCopy;
    }

    void Seek(int position) override {
        if(position >= 0 && static_cast<size_t>(position) <= m_data.size()) {
            m_position = static_cast<size_t>(position);
        }
    }

    int GetPosition() const override {
        return static_cast<int>(m_position);
    }

    size_t GetLength() const override {
        return m_data.size();
    }

    const std::vector<uint8_t>& GetData() const { return m_data; }

private:
    std::vector<uint8_t> m_data;
    size_t m_position;
};

} // namespace Tsukino::EngineIntegration