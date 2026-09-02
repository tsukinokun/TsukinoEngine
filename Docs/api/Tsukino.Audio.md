# Tsukino.Audio の公開 API

**このファイルは自動生成です。直接編集しないでください。**

型の在処だけ知りたいときは `../api-index.md` を見る。

## 主要な型

ゲーム側から実際に触る型。メンバを全て展開している。

### Tsukino::Audio::AudioManager

`Tsukino.Audio/include/Tsukino/Audio/AudioManager.hpp`

**公開関数**

| シグネチャ | 説明 |
|---|---|
| `AudioManager()` | コンストラクタ |
| `~AudioManager()` | デストラクタ |
| `AudioManager(const AudioManager &)=delete` |  |
| `AudioManager & operator=(const AudioManager &)=delete` |  |
| `AudioManager(AudioManager &&)=delete` |  |
| `AudioManager & operator=(AudioManager &&)=delete` |  |
| `bool Initialize()` | 初期化 |
| `void Update(float deltaTime)` | 更新関数 |
| `void Play(const Tsukino::Asset::AudioAsset &audioAsset, bool isLoop=false, float volume=1.0f)` | 音声を再生する |
| `void Stop(const Tsukino::Asset::AudioAsset &audioAsset)` | 特定の音声を停止する |
| `void StopAll()` | 全ての音声を停止する |
| `bool IsPlaying(const Tsukino::Asset::AudioAsset &audioAsset) const` | 特定の音声が再生中か確認する |
| `void SetMasterVolume(float volume)` | マスター音量を設定する |
| `float GetMasterVolume() const` | マスター音量を取得する |

## 全公開型の索引

メンバ名のみ。詳細が要るときはヘッダを開く。

- **Tsukino::Audio::AudioContext** — `Tsukino.Audio/src/Audio/AudioManager.cpp`
  - engine, waveBanks, Initialize(), Update(), GetOrLoadWaveBank()
- **Tsukino::Audio::AudioManager** — `Tsukino.Audio/include/Tsukino/Audio/AudioManager.hpp`
  - AudioManager(), ~AudioManager(), AudioManager(), operator=(), AudioManager(), operator=(), Initialize(), Update(), Play(), Stop(), StopAll(), IsPlaying(), SetMasterVolume(), GetMasterVolume()
