//--------------------------------------------------------------
//! @file   AudioLoader.cpp
//! @brief  オーディオアセットローダーの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Audio/AudioLoader.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioAsset.hpp>

#include <Tsukino/Core/Log.hpp>

#include <fstream>
#include <vector>

namespace {
    // 型別名は Tsukino 名前空間にあるため、この匿名名前空間へ持ち込む
    using Tsukino::u32;
    using Tsukino::u8;

#pragma pack(push, 1)
    struct Segment {
        u32 offset;
        u32 length;
    };

    struct WaveBankHeader {
        u32 signature;    // 'WBND'
        u32 version;
        u32 headerVersion;
        Segment  segments[5];    // BANKDATA / ENTRYMETADATA / SEEKTABLES / ENTRYNAMES / ENTRYWAVEDATA
    };

    struct WaveBankEntry {
        u32 flagsAndDuration;
        u32 miniFormat;
        u32 playRegionOffset;
        u32 playRegionLength;
        u32 loopRegionOffset;
        u32 loopRegionLength;
    };
#pragma pack(pop)

    constexpr u32 kWaveBankSignature    = 0x444E4257;    // 'WBND' little-endian
    constexpr u32 kWaveBankFlagsCompact = 0x00020000;

    std::string GetBasePath(const std::string& path) {
        const size_t pos = path.find('#');
        return (pos == std::string::npos) ? path : path.substr(0, pos);
    }

    std::string GetFragment(const std::string& path) {
        const size_t pos = path.find('#');
        return (pos == std::string::npos) ? "" : path.substr(pos + 1);
    }

    bool IsNumber(const std::string& text) {
        return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
    }

    u32 ReadU32(const std::vector<u8>& buffer, size_t offset) {
        u32 value = 0;
        std::memcpy(&value, buffer.data() + offset, sizeof(u32));
        return value;
    }
}    // namespace

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {

    //--------------------------------------------------------------
    //! @brief 対応する拡張子か判定する関数
    //--------------------------------------------------------------
    bool AudioLoader::CanLoad(const std::string& ext) const {
        std::string  pureExt = ext;
        const size_t pos     = pureExt.find('#');
        if(pos != std::string::npos) {
            pureExt = pureExt.substr(0, pos);
        }
        return pureExt == ".xwb";
    }

    //--------------------------------------------------------------
    //! @brief オーディオファイルを読み込み AudioAsset を生成する関数
    //--------------------------------------------------------------
    Tsukino::Core::Ref<IAsset> AudioLoader::Load(const Tsukino::Core::Path& path) {
        //--------------------------------------------------------------
        // パスを分解（#以降はサブリソース）
        //--------------------------------------------------------------
        const std::string rawPath     = path.string();
        const std::string basePath    = GetBasePath(rawPath);
        const std::string subResource = GetFragment(rawPath);

        //--------------------------------------------------------------
        // .xwb を開く
        //--------------------------------------------------------------
        std::ifstream file(basePath, std::ios::binary);
        if(!file.is_open()) {
            Tsukino::Core::Log::Error("Failed to open wave bank: " + basePath);
            return nullptr;
        }

        file.seekg(0, std::ios::end);
        const std::streamsize fileSize = file.tellg();
        if(fileSize <= 0) {
            Tsukino::Core::Log::Error("Invalid wave bank (empty): " + basePath);
            return nullptr;
        }

        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

        if(buffer.size() < sizeof(WaveBankHeader)) {
            Tsukino::Core::Log::Error("Invalid wave bank header: " + basePath);
            return nullptr;
        }

        //--------------------------------------------------------------
        // XWBヘッダ解析
        //--------------------------------------------------------------
        WaveBankHeader header{};
        std::memcpy(&header, buffer.data(), sizeof(WaveBankHeader));

        if(header.signature != kWaveBankSignature) {
            Tsukino::Core::Log::Error("Invalid wave bank signature: " + basePath);
            return nullptr;
        }

        const Segment& bankDataSeg      = header.segments[0];
        const Segment& entryMetaDataSeg = header.segments[1];
        const Segment& entryNamesSeg    = header.segments[3];
        const Segment& entryWaveDataSeg = header.segments[4];

        if(bankDataSeg.offset + bankDataSeg.length > buffer.size() || entryMetaDataSeg.offset + entryMetaDataSeg.length > buffer.size()
           || entryWaveDataSeg.offset + entryWaveDataSeg.length > buffer.size()) {
            Tsukino::Core::Log::Error("Corrupted wave bank segment: " + basePath);
            return nullptr;
        }

        // WAVEBANKDATA (必要項目のみ)
        if(bankDataSeg.length < 88) {
            Tsukino::Core::Log::Error("Invalid bank data segment: " + basePath);
            return nullptr;
        }

        const u32 bankFlags            = ReadU32(buffer, bankDataSeg.offset + 0);
        const u32 entryCount           = ReadU32(buffer, bankDataSeg.offset + 4);
        const u32 entryMetaElementSize = ReadU32(buffer, bankDataSeg.offset + 72);
        const u32 entryNameElementSize = ReadU32(buffer, bankDataSeg.offset + 76);

        if(entryCount == 0) {
            Tsukino::Core::Log::Error("Wave bank has no entries: " + basePath);
            return nullptr;
        }

        if((bankFlags & kWaveBankFlagsCompact) != 0) {
            Tsukino::Core::Log::Error("Compact wave bank is not supported yet: " + basePath);
            return nullptr;
        }

        if(entryMetaElementSize < sizeof(WaveBankEntry)) {
            Tsukino::Core::Log::Error("Unexpected entry metadata size: " + basePath);
            return nullptr;
        }

        //--------------------------------------------------------------
        // サブリソース解決（#index または #name）
        //--------------------------------------------------------------
        u32 targetIndex = 0;

        if(!subResource.empty()) {
            if(IsNumber(subResource)) {
                targetIndex = static_cast<u32>(std::stoul(subResource));
            } else {
                if(entryNamesSeg.length == 0 || entryNameElementSize == 0) {
                    Tsukino::Core::Log::Error("Entry names are not available in this wave bank: " + basePath);
                    return nullptr;
                }

                bool found = false;
                for(u32 i = 0; i < entryCount; ++i) {
                    const size_t nameOffset = static_cast<size_t>(entryNamesSeg.offset) + static_cast<size_t>(i) * entryNameElementSize;
                    if(nameOffset + entryNameElementSize > buffer.size())
                        break;

                    std::string  name(reinterpret_cast<const char*>(buffer.data() + nameOffset), entryNameElementSize);
                    const size_t nullPos = name.find('\0');
                    if(nullPos != std::string::npos) {
                        name.resize(nullPos);
                    }

                    if(name == subResource) {
                        targetIndex = i;
                        found       = true;
                        break;
                    }
                }

                if(!found) {
                    Tsukino::Core::Log::Error("Subresource not found in wave bank: " + subResource);
                    return nullptr;
                }
            }
        }

        if(targetIndex >= entryCount) {
            Tsukino::Core::Log::Error("Subresource index out of range: " + std::to_string(targetIndex));
            return nullptr;
        }

        //--------------------------------------------------------------
        // 対象エントリのメタデータ取得
        //--------------------------------------------------------------
        const size_t entryOffset = static_cast<size_t>(entryMetaDataSeg.offset) + static_cast<size_t>(targetIndex) * entryMetaElementSize;
        if(entryOffset + sizeof(WaveBankEntry) > buffer.size()) {
            Tsukino::Core::Log::Error("Invalid entry metadata offset in wave bank: " + basePath);
            return nullptr;
        }

        WaveBankEntry entry{};
        std::memcpy(&entry, buffer.data() + entryOffset, sizeof(WaveBankEntry));

        const u32 absoluteOffset = entryWaveDataSeg.offset + entry.playRegionOffset;
        const u32 dataLength     = entry.playRegionLength;

        if(static_cast<u64>(absoluteOffset) + static_cast<u64>(dataLength) > buffer.size()) {
            Tsukino::Core::Log::Error("Invalid wave data range in wave bank: " + basePath);
            return nullptr;
        }

        //--------------------------------------------------------------
        // mini format を簡易デコード
        //--------------------------------------------------------------
        const u32 miniFormat = entry.miniFormat;

        const u16 formatTagBits = static_cast<u16>(miniFormat & 0x3);
        const u16 channels      = static_cast<u16>(((miniFormat >> 2) & 0x7) + 1);
        const u32 sampleRate    = (miniFormat >> 5) & 0x3FFFF;

        u16 formatTag = 0;    // 1: PCM, 2: ADPCM, 3: WMA
        switch(formatTagBits) {
        case 0:
            formatTag = 0;
            break;
        case 1:
            formatTag = 1;
            break;
        case 2:
            formatTag = 2;
            break;
        case 3:
            formatTag = 0x0161;
            break;
        default:
            formatTag = 0;
            break;
        }

        //--------------------------------------------------------------
        // AudioAsset 生成
        //--------------------------------------------------------------
        Tsukino::Core::Ref<AudioAsset> asset = Tsukino::Core::CreateRef<AudioAsset>();
        asset->waveBankPath                  = basePath;
        asset->waveIndex                     = targetIndex; // DirectXTKへの受け渡し用に追加
        asset->metadata.offset               = absoluteOffset;
        asset->metadata.length               = dataLength;
        asset->metadata.sampleRate           = sampleRate;
        asset->metadata.channels             = channels;
        asset->metadata.formatTag            = formatTag;

        return asset;
    }

}    // namespace Tsukino::Asset
