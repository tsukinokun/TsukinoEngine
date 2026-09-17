//--------------------------------------------------------------
//! @file   AudioManager.cpp
//! @brief  オーディオ管理システムの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Audio/AudioManager.hpp>
#include <Tsukino/Engine/Asset/Audio/AudioAsset.hpp>
#include <Tsukino/Core/Log.hpp>

#include <Audio.h>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include <locale>
#include <codecvt>

namespace {
    // std::string から std::wstring へ変換するユーティリティ
    std::wstring ToWString(const std::string& str) {
        if (str.empty()) return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }
}

// 名前空間 : Tsukino::Audio
namespace Tsukino::Audio {

    //--------------------------------------------------------------
    //! @class  AudioContext
    //! @brief  DirectXTK AudioEngineを隠蔽するコンテキスト
    //--------------------------------------------------------------
    class AudioContext {
    public:
        //--------------------------------------------------------------
        //! @struct PlayingSound
        //! @brief  再生中（または一時停止中）の音1つぶん。
        //!         どのアセットの音かを覚えておき、Stop / IsPlaying で引き当てる
        //--------------------------------------------------------------
        struct PlayingSound {
            std::string                                   waveBankPath;    //!< 属しているWaveBank（AudioAsset::waveBankPath）
            u32                                           waveIndex = 0;   //!< WaveBank内のインデックス（AudioAsset::waveIndex）
            std::unique_ptr<DirectX::SoundEffectInstance> instance;        //!< 再生を止める・状態を問うためのハンドル
        };

        std::unique_ptr<DirectX::AudioEngine> engine;
        std::unordered_map<std::string, std::unique_ptr<DirectX::WaveBank>> waveBanks;
        //! 再生中のインスタンス。WaveBankの音を参照しているので、engine・waveBanksより後に宣言して先に破棄させる
        std::vector<PlayingSound> playing;

        //--------------------------------------------------------------
        //! @brief 初期化
        //--------------------------------------------------------------
        bool Initialize() {
            DirectX::AUDIO_ENGINE_FLAGS flags = DirectX::AudioEngine_Default;
#ifdef _DEBUG
            flags |= DirectX::AudioEngine_Debug;
#endif
            try {
                engine = std::make_unique<DirectX::AudioEngine>(flags);
                return true;
            } catch (const std::exception& e) {
                Tsukino::Core::Log::Error(std::string("Failed to initialize AudioEngine: ") + e.what());
                return false;
            }
        }

        //--------------------------------------------------------------
        //! @brief 更新関数
        //--------------------------------------------------------------
        void Update() {
            if (engine && !engine->Update()) {
                if (engine->IsCriticalError()) {
                    Tsukino::Core::Log::Error("AudioEngine critical error detected.");
                }
            }

            // 鳴り終わった音を一覧から外す。撃ちっぱなしの効果音（ヒット音など）が
            // 溜まり続けてボイスを食い潰さないようにする
            std::erase_if(playing, [](const PlayingSound& sound) {
                return !sound.instance || sound.instance->GetState() == DirectX::STOPPED;
            });
        }

        //--------------------------------------------------------------
        //! @brief WaveBankが存在しなければ読み込んで返す
        //--------------------------------------------------------------
        DirectX::WaveBank* GetOrLoadWaveBank(const std::string& waveBankPath) {
            auto it = waveBanks.find(waveBankPath);
            if (it != waveBanks.end()) {
                return it->second.get();
            }

            try {
                std::wstring wPath = ToWString(waveBankPath);
                auto wb = std::make_unique<DirectX::WaveBank>(engine.get(), wPath.c_str());
                DirectX::WaveBank* rawPtr = wb.get();
                waveBanks[waveBankPath] = std::move(wb);
                return rawPtr;
            } catch (const std::exception& e) {
                Tsukino::Core::Log::Error(std::string("Failed to load WaveBank: ") + e.what());
                return nullptr;
            }
        }
    };

    //--------------------------------------------------------------
    //! @brief コンストラクタ
    //--------------------------------------------------------------
    AudioManager::AudioManager() 
        : m_audioContext(nullptr)
        , m_masterVolume(1.0f) {
    }

    //--------------------------------------------------------------
    //! @brief デストラクタ
    //--------------------------------------------------------------
    AudioManager::~AudioManager() {
        if (m_audioContext && m_audioContext->engine) {
            m_audioContext->engine->Suspend();
        }
    }

    //--------------------------------------------------------------
    //! @brief 初期化
    //--------------------------------------------------------------
    bool AudioManager::Initialize() {
        m_audioContext = std::make_unique<AudioContext>();
        
        if (!m_audioContext->Initialize()) {
            return false;
        }

        SetMasterVolume(1.0f);
        return true;
    }

    //--------------------------------------------------------------
    //! @brief 更新関数
    //--------------------------------------------------------------
    void AudioManager::Update(float /*deltaTime*/) {
        if (m_audioContext) {
            m_audioContext->Update();
        }
    }

    //--------------------------------------------------------------
    //! @brief 音声を再生する
    //--------------------------------------------------------------
    void AudioManager::Play(const Tsukino::Asset::AudioAsset& audioAsset, bool isLoop, float volume) {
        if (!m_audioContext || !m_audioContext->engine) return;

        DirectX::WaveBank* waveBank = m_audioContext->GetOrLoadWaveBank(audioAsset.waveBankPath);
        if (!waveBank) return;

        // 撃ちっぱなしの WaveBank::Play() では後から止められないため、
        // 1回の再生ごとにインスタンスを作って手元に持つ
        std::unique_ptr<DirectX::SoundEffectInstance> instance;
        try {
            instance = waveBank->CreateInstance(audioAsset.waveIndex);
        } catch (const std::exception& e) {
            Tsukino::Core::Log::Error(std::string("AudioManager::Play - Failed to create sound instance: ") + e.what());
            return;
        }

        if (!instance) {
            // ストリーミング用のWaveBankなど、インスタンスを作れない音
            Tsukino::Core::Log::Error("AudioManager::Play - Sound instance is not available for: " + audioAsset.waveBankPath);
            return;
        }

        // マスター音量は SetMasterVolume が AudioEngine 側へ設定済みなので、ここでは掛けない
        instance->SetVolume(std::clamp(volume, 0.0f, 1.0f));
        instance->Play(isLoop);

        m_audioContext->playing.push_back({audioAsset.waveBankPath, audioAsset.waveIndex, std::move(instance)});
    }

    //--------------------------------------------------------------
    //! @brief 特定の音声を停止する
    //--------------------------------------------------------------
    void AudioManager::Stop(const Tsukino::Asset::AudioAsset& audioAsset) {
        if (!m_audioContext) return;

        // 同じ音を重ねて鳴らしている場合は、そのすべてを止める
        std::erase_if(m_audioContext->playing, [&](AudioContext::PlayingSound& sound) {
            if (sound.waveIndex != audioAsset.waveIndex || sound.waveBankPath != audioAsset.waveBankPath) {
                return false;
            }
            if (sound.instance) {
                sound.instance->Stop(true);    // 余韻を待たずに即座に止める
            }
            return true;
        });
    }

    //--------------------------------------------------------------
    //! @brief 全ての音声を停止する
    //--------------------------------------------------------------
    void AudioManager::StopAll() {
        if (!m_audioContext) return;

        for (AudioContext::PlayingSound& sound : m_audioContext->playing) {
            if (sound.instance) {
                sound.instance->Stop(true);
            }
        }
        m_audioContext->playing.clear();
    }

    //--------------------------------------------------------------
    //! @brief 特定の音声が再生中か確認する
    //--------------------------------------------------------------
    bool AudioManager::IsPlaying(const Tsukino::Asset::AudioAsset& audioAsset) const {
        if (!m_audioContext) return false;

        return std::any_of(m_audioContext->playing.begin(), m_audioContext->playing.end(), [&](const AudioContext::PlayingSound& sound) {
            return sound.waveIndex == audioAsset.waveIndex && sound.waveBankPath == audioAsset.waveBankPath && sound.instance
                   && sound.instance->GetState() == DirectX::PLAYING;
        });
    }

    //--------------------------------------------------------------
    //! @brief マスター音量を設定する
    //--------------------------------------------------------------
    void AudioManager::SetMasterVolume(float volume) {
        m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
        if (m_audioContext && m_audioContext->engine) {
            m_audioContext->engine->SetMasterVolume(m_masterVolume);
        }
    }

    //--------------------------------------------------------------
    //! @brief マスター音量を取得する
    //--------------------------------------------------------------
    float AudioManager::GetMasterVolume() const {
        return m_masterVolume;
    }

} // namespace Tsukino::Audio
