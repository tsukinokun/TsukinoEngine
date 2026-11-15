//--------------------------------------------------------------
//! @file   AudioManager.cpp
//! @brief  オーディオ管理システムの実装（miniaudio）
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Audio/AudioManager.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioAsset.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/Path.hpp>

#include <miniaudio.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <unordered_map>
#include <vector>

// 名前空間 : Tsukino::Audio
namespace Tsukino::Audio {

    namespace {
        //--------------------------------------------------------------
        // miniaudio のファイルの読み込みを、エンジンの FileSystem へ回す。
        // パスは UTF-8 のまま渡り、OS ごとの置き場所（Android の APK の中など）の違いは
        // FileSystem が吸収する。開いたときにファイル全体を読み、以降はメモリから返す
        //--------------------------------------------------------------

        //! 開いているファイル1つ
        struct MemoryFile {
            std::vector<u8> data;        //!< ファイルの中身
            size_t          cursor = 0;  //!< 次に読む位置
        };

        //! miniaudio へ渡す VFS（先頭が ma_vfs_callbacks であること）
        struct EngineVfs {
            ma_vfs_callbacks callbacks;
        };

        ma_result VfsOpen(ma_vfs* /*vfs*/, const char* filePath, ma_uint32 openMode, ma_vfs_file* file) {
            if(file == nullptr || filePath == nullptr)
                return MA_INVALID_ARGS;
            *file = nullptr;
            if((openMode & MA_OPEN_MODE_WRITE) != 0)
                return MA_NOT_IMPLEMENTED;    // 書き込みはしない

            std::vector<u8> data = Tsukino::IO::FileSystem::ReadBinary(Tsukino::Core::Path(filePath));
            if(data.empty())
                return MA_DOES_NOT_EXIST;

            auto* memoryFile = new MemoryFile{std::move(data), 0};
            *file            = memoryFile;
            return MA_SUCCESS;
        }

        ma_result VfsOpenW(ma_vfs* /*vfs*/, const wchar_t* /*filePath*/, ma_uint32 /*openMode*/, ma_vfs_file* /*file*/) {
            return MA_NOT_IMPLEMENTED;    // パスは UTF-8 の char で渡す
        }

        ma_result VfsClose(ma_vfs* /*vfs*/, ma_vfs_file file) {
            delete static_cast<MemoryFile*>(file);
            return MA_SUCCESS;
        }

        ma_result VfsRead(ma_vfs* /*vfs*/, ma_vfs_file file, void* dst, size_t sizeInBytes, size_t* bytesRead) {
            auto*        memoryFile = static_cast<MemoryFile*>(file);
            const size_t available  = memoryFile->data.size() - memoryFile->cursor;
            const size_t count      = std::min(sizeInBytes, available);
            if(count > 0)
                std::memcpy(dst, memoryFile->data.data() + memoryFile->cursor, count);
            memoryFile->cursor += count;
            if(bytesRead != nullptr)
                *bytesRead = count;
            return (count == 0 && sizeInBytes > 0) ? MA_AT_END : MA_SUCCESS;
        }

        ma_result VfsWrite(ma_vfs* /*vfs*/, ma_vfs_file /*file*/, const void* /*src*/, size_t /*sizeInBytes*/, size_t* /*bytesWritten*/) {
            return MA_NOT_IMPLEMENTED;
        }

        ma_result VfsSeek(ma_vfs* /*vfs*/, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin) {
            auto*          memoryFile = static_cast<MemoryFile*>(file);
            const ma_int64 size       = static_cast<ma_int64>(memoryFile->data.size());
            ma_int64       base       = 0;
            if(origin == ma_seek_origin_current)
                base = static_cast<ma_int64>(memoryFile->cursor);
            else if(origin == ma_seek_origin_end)
                base = size;

            const ma_int64 target = base + offset;
            if(target < 0 || target > size)
                return MA_BAD_SEEK;
            memoryFile->cursor = static_cast<size_t>(target);
            return MA_SUCCESS;
        }

        ma_result VfsTell(ma_vfs* /*vfs*/, ma_vfs_file file, ma_int64* cursor) {
            *cursor = static_cast<ma_int64>(static_cast<MemoryFile*>(file)->cursor);
            return MA_SUCCESS;
        }

        ma_result VfsInfo(ma_vfs* /*vfs*/, ma_vfs_file file, ma_file_info* info) {
            info->sizeInBytes = static_cast<ma_uint64>(static_cast<MemoryFile*>(file)->data.size());
            return MA_SUCCESS;
        }

        //--------------------------------------------------------------
        //! 音のファイルを鳴らすときの読み方を返します。
        //! @param  [in] filePath 音のファイルのパス
        //! @return MA_SOUND_FLAG_* の組み合わせ
        //! @note   圧縮した形式（.mp3 / .flac）は少しずつ復号して流す（全部を復号すると元の10倍ほどのメモリを使うため）。
        //!         .wav はもともと復号済みなので、全部を読んで持つ（同じ音は miniaudio が1つを共有する）
        //--------------------------------------------------------------
        ma_uint32 SoundFlags(const std::string& filePath) {
            const std::string ext = Tsukino::Core::Path::ToLower(Tsukino::Core::Path(filePath).extension());
            return (ext == ".wav") ? MA_SOUND_FLAG_DECODE : MA_SOUND_FLAG_STREAM;
        }
    }    // namespace

    //--------------------------------------------------------------
    //! @class  AudioContext
    //! @brief  miniaudio のエンジンを隠蔽するコンテキスト
    //--------------------------------------------------------------
    class AudioContext {
    public:
        //--------------------------------------------------------------
        //! @struct PlayingSound
        //! @brief  再生中の音1つぶん。どのアセットの音かを覚えておき、Stop / IsPlaying で引き当てる
        //--------------------------------------------------------------
        struct PlayingSound {
            std::string               filePath;          //!< 鳴らしているファイル（AudioAsset::filePath）
            std::unique_ptr<ma_sound> sound;             //!< miniaudio の音（初期化した後は動かせないので、ヒープに置く）
            float                     volume = 1.0f;     //!< Play・SetVolume で指定された音量（バスの音量を掛ける前）
            AudioBus                  bus    = 0;        //!< 属するバス
        };

        EngineVfs vfs{};    //!< ファイルの読み込み（engine より先に作り、後に壊す）
        ma_engine engine{};
        bool      engineReady = false;

        //! バスごとの音のグループ（音量をまとめて掛ける）
        std::array<ma_sound_group, kAudioBusCount> buses{};

        //! Prepare で読み込みを始めた音。読み終えるまで、またその後も中身を持っておくために1つずつ保つ
        std::unordered_map<std::string, std::unique_ptr<ma_sound>> prepared;

        //! 再生中の音。engine より後に宣言して先に破棄させる
        std::vector<PlayingSound> playing;

        //--------------------------------------------------------------
        //! @brief 初期化
        //--------------------------------------------------------------
        bool Initialize() {
            vfs.callbacks = {VfsOpen, VfsOpenW, VfsClose, VfsRead, VfsWrite, VfsSeek, VfsTell, VfsInfo};

            ma_engine_config config  = ma_engine_config_init();
            config.pResourceManagerVFS = &vfs;
            if(ma_engine_init(&config, &engine) != MA_SUCCESS) {
                Tsukino::Core::Log::Error("AudioManager: Failed to initialize the audio engine.");
                return false;
            }
            engineReady = true;

            for(ma_sound_group& bus : buses) {
                if(ma_sound_group_init(&engine, 0, nullptr, &bus) != MA_SUCCESS) {
                    Tsukino::Core::Log::Error("AudioManager: Failed to create an audio bus.");
                    return false;
                }
            }
            return true;
        }

        //--------------------------------------------------------------
        //! @brief 後始末（音 → バス → エンジンの順）
        //--------------------------------------------------------------
        ~AudioContext() {
            for(PlayingSound& sound : playing)
                ma_sound_uninit(sound.sound.get());
            playing.clear();
            for(auto& [path, sound] : prepared)
                ma_sound_uninit(sound.get());
            prepared.clear();
            if(engineReady) {
                for(ma_sound_group& bus : buses)
                    ma_sound_group_uninit(&bus);
                ma_engine_uninit(&engine);
            }
        }

        //--------------------------------------------------------------
        //! @brief 鳴り終わった音を一覧から外す
        //--------------------------------------------------------------
        void Update() {
            // 撃ちっぱなしの効果音（ヒット音など）が溜まり続けないようにする
            std::erase_if(playing, [](PlayingSound& sound) {
                if(ma_sound_is_playing(sound.sound.get()))
                    return false;
                ma_sound_uninit(sound.sound.get());
                return true;
            });
        }
    };

    //--------------------------------------------------------------
    //! @brief コンストラクタ
    //--------------------------------------------------------------
    AudioManager::AudioManager()
        : m_audioContext(nullptr)
        , m_masterVolume(1.0f) {
        m_busVolumes.fill(1.0f);
    }

    //--------------------------------------------------------------
    //! @brief デストラクタ
    //--------------------------------------------------------------
    AudioManager::~AudioManager() = default;

    //--------------------------------------------------------------
    //! @brief 初期化
    //--------------------------------------------------------------
    bool AudioManager::Initialize() {
        m_audioContext = std::make_unique<AudioContext>();
        if(!m_audioContext->Initialize()) {
            m_audioContext.reset();
            return false;
        }

        SetMasterVolume(m_masterVolume);
        for(AudioBus bus = 0; bus < kAudioBusCount; ++bus)
            SetBusVolume(bus, m_busVolumes[bus]);
        return true;
    }

    //--------------------------------------------------------------
    //! @brief 更新関数
    //--------------------------------------------------------------
    void AudioManager::Update(float /*deltaTime*/) {
        if(m_audioContext) {
            m_audioContext->Update();
        }
    }

    //--------------------------------------------------------------
    //! @brief 音声を再生する
    //--------------------------------------------------------------
    void AudioManager::Play(const Tsukino::Asset::AudioAsset& audioAsset, bool isLoop, float volume, AudioBus bus) {
        if(!m_audioContext)
            return;
        if(bus >= kAudioBusCount) {
            Tsukino::Core::Log::Warn("AudioManager::Play - Bus index out of range: " + std::to_string(bus));
            bus = 0;
        }

        // 1回の再生ごとに音を作る（同じ音を重ねて鳴らせるように）。
        // 中身は miniaudio がファイルごとに共有するので、Prepare 済みならすぐに作れる
        auto sound = std::make_unique<ma_sound>();
        if(ma_sound_init_from_file(&m_audioContext->engine, audioAsset.filePath.c_str(), SoundFlags(audioAsset.filePath),
                                   &m_audioContext->buses[bus], nullptr, sound.get())
           != MA_SUCCESS) {
            Tsukino::Core::Log::Error("AudioManager::Play - Failed to open: " + audioAsset.filePath);
            return;
        }

        const float clamped = std::clamp(volume, 0.0f, 1.0f);
        ma_sound_set_volume(sound.get(), clamped);
        ma_sound_set_looping(sound.get(), isLoop ? MA_TRUE : MA_FALSE);
        ma_sound_start(sound.get());

        m_audioContext->playing.push_back({audioAsset.filePath, std::move(sound), clamped, bus});
    }

    //--------------------------------------------------------------
    //! @brief 音声のデータの読み込みを始める
    //--------------------------------------------------------------
    void AudioManager::Prepare(const Tsukino::Asset::AudioAsset& audioAsset) {
        if(!m_audioContext || m_audioContext->prepared.contains(audioAsset.filePath))
            return;

        // 鳴らさない音を1つ作って持っておく。読み込みは miniaudio の裏のスレッドで進む
        auto sound = std::make_unique<ma_sound>();
        if(ma_sound_init_from_file(&m_audioContext->engine, audioAsset.filePath.c_str(), SoundFlags(audioAsset.filePath) | MA_SOUND_FLAG_ASYNC,
                                   nullptr, nullptr, sound.get())
           != MA_SUCCESS) {
            Tsukino::Core::Log::Error("AudioManager::Prepare - Failed to open: " + audioAsset.filePath);
            return;
        }
        m_audioContext->prepared.emplace(audioAsset.filePath, std::move(sound));
    }

    //--------------------------------------------------------------
    //! @brief 音声のデータを読み終えたか確認する
    //--------------------------------------------------------------
    bool AudioManager::IsPrepared(const Tsukino::Asset::AudioAsset& audioAsset) const {
        if(!m_audioContext)
            return false;

        const auto it = m_audioContext->prepared.find(audioAsset.filePath);
        if(it == m_audioContext->prepared.end())
            return false;

        const auto* dataSource = static_cast<const ma_resource_manager_data_source*>(ma_sound_get_data_source(it->second.get()));
        return dataSource != nullptr && ma_resource_manager_data_source_result(dataSource) == MA_SUCCESS;
    }

    //--------------------------------------------------------------
    //! @brief 特定の音声を停止する
    //--------------------------------------------------------------
    void AudioManager::Stop(const Tsukino::Asset::AudioAsset& audioAsset) {
        if(!m_audioContext)
            return;

        // 同じ音を重ねて鳴らしている場合は、そのすべてを止める
        std::erase_if(m_audioContext->playing, [&](AudioContext::PlayingSound& sound) {
            if(sound.filePath != audioAsset.filePath)
                return false;
            ma_sound_uninit(sound.sound.get());
            return true;
        });
    }

    //--------------------------------------------------------------
    //! @brief 全ての音声を停止する
    //--------------------------------------------------------------
    void AudioManager::StopAll() {
        if(!m_audioContext)
            return;

        for(AudioContext::PlayingSound& sound : m_audioContext->playing)
            ma_sound_uninit(sound.sound.get());
        m_audioContext->playing.clear();
    }

    //--------------------------------------------------------------
    //! @brief 音の出力を止める
    //--------------------------------------------------------------
    void AudioManager::Suspend() {
        if(m_audioContext)
            ma_engine_stop(&m_audioContext->engine);
    }

    //--------------------------------------------------------------
    //! @brief Suspend で止めた音の出力を再開する
    //--------------------------------------------------------------
    void AudioManager::Resume() {
        if(m_audioContext)
            ma_engine_start(&m_audioContext->engine);
    }

    //--------------------------------------------------------------
    //! @brief 特定の音声が再生中か確認する
    //--------------------------------------------------------------
    bool AudioManager::IsPlaying(const Tsukino::Asset::AudioAsset& audioAsset) const {
        if(!m_audioContext)
            return false;

        return std::any_of(m_audioContext->playing.begin(), m_audioContext->playing.end(), [&](const AudioContext::PlayingSound& sound) {
            return sound.filePath == audioAsset.filePath && ma_sound_is_playing(sound.sound.get());
        });
    }

    //--------------------------------------------------------------
    //! @brief 鳴っている音声の音量を変える
    //--------------------------------------------------------------
    void AudioManager::SetVolume(const Tsukino::Asset::AudioAsset& audioAsset, float volume) {
        if(!m_audioContext)
            return;

        // 同じ音を重ねて鳴らしている場合は、そのすべてに掛ける
        const float clamped = std::clamp(volume, 0.0f, 1.0f);
        for(AudioContext::PlayingSound& sound : m_audioContext->playing) {
            if(sound.filePath == audioAsset.filePath) {
                sound.volume = clamped;
                ma_sound_set_volume(sound.sound.get(), clamped);
            }
        }
    }

    //--------------------------------------------------------------
    //! @brief マスター音量を設定する
    //--------------------------------------------------------------
    void AudioManager::SetMasterVolume(float volume) {
        m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
        if(m_audioContext)
            ma_engine_set_volume(&m_audioContext->engine, m_masterVolume);
    }

    //--------------------------------------------------------------
    //! @brief マスター音量を取得する
    //--------------------------------------------------------------
    float AudioManager::GetMasterVolume() const {
        return m_masterVolume;
    }

    //--------------------------------------------------------------
    //! @brief バスの音量を設定する
    //--------------------------------------------------------------
    void AudioManager::SetBusVolume(AudioBus bus, float volume) {
        if(bus >= kAudioBusCount) {
            Tsukino::Core::Log::Warn("AudioManager::SetBusVolume - Bus index out of range: " + std::to_string(bus));
            return;
        }
        m_busVolumes[bus] = std::clamp(volume, 0.0f, 1.0f);

        // そのバスで鳴っている音（流れている BGM など）にもすぐ掛かる
        if(m_audioContext)
            ma_sound_group_set_volume(&m_audioContext->buses[bus], m_busVolumes[bus]);
    }

    //--------------------------------------------------------------
    //! @brief バスの音量を取得する
    //--------------------------------------------------------------
    float AudioManager::GetBusVolume(AudioBus bus) const {
        return (bus < kAudioBusCount) ? m_busVolumes[bus] : 1.0f;
    }

}    // namespace Tsukino::Audio
