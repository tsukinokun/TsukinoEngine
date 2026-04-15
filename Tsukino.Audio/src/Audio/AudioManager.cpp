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
        std::unique_ptr<DirectX::AudioEngine> engine;
        std::unordered_map<std::string, std::unique_ptr<DirectX::WaveBank>> waveBanks;

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
    void AudioManager::Play(const Tsukino::Asset::AudioAsset& audioAsset, bool /*isLoop*/, float volume) {
        if (!m_audioContext || !m_audioContext->engine) return;

        DirectX::WaveBank* waveBank = m_audioContext->GetOrLoadWaveBank(audioAsset.waveBankPath);
        if (waveBank) {
            // DirectXTK の WaveBank::Play() は内部の AudioEngine を用いて再生を開始します
            waveBank->Play(audioAsset.waveIndex, volume * m_masterVolume, 0.0f, 0.0f);
        }
    }

    //--------------------------------------------------------------
    //! @brief 特定の音声を停止する
    //--------------------------------------------------------------
    void AudioManager::Stop(const Tsukino::Asset::AudioAsset& /*audioAsset*/) {
        // DirectXTK の単発再生 (SoundEffect::Play) は個別に停止できません。
        Tsukino::Core::Log::Warn("AudioManager::Stop - Specific stopping requires SoundEffectInstance which is unsupported by basic WaveBank::Play.");
    }

    //--------------------------------------------------------------
    //! @brief 全ての音声を停止する
    //--------------------------------------------------------------
    void AudioManager::StopAll() {
        if (m_audioContext && m_audioContext->engine) {
            // 一時停止して再開扱いにするか、保持しているInstanceを破棄するアプローチを取ります
            m_audioContext->engine->Suspend();
            m_audioContext->engine->Resume();
        }
    }

    //--------------------------------------------------------------
    //! @brief 特定の音声が再生中か確認する
    //--------------------------------------------------------------
    bool AudioManager::IsPlaying(const Tsukino::Asset::AudioAsset& /*audioAsset*/) const {
        return false; // Instance管理拡張後に実装
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
