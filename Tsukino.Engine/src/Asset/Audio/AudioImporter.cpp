//--------------------------------------------------------------
//! @file   AudioImporter.cpp
//! @brief  オーディオのインポータークラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Audio/AudioImporter.hpp>

#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/Log.hpp>

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

#include <cstdint>
#include <fstream>
#include <vector>

// mp3 のデコードに使う Media Foundation（OS 同梱）。StaticLib なので利用側の links に頼らずここで引く
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    namespace {
        //----------------------------------------------------------------------------
        //! 圧縮音声（mp3 など）を 16bit PCM の .wav へデコードします。
        //! @param  [in] inputPath  入力ファイルの絶対パス
        //! @param  [in] outputPath 書き出す .wav の絶対パス
        //! @return 書き出せたら true。
        //! @note   XWBTool.exe は .wav しか受け付けないため、その前段として使います。
        //!         デコードは Windows 標準の Media Foundation（Source Reader）に任せます。
        //----------------------------------------------------------------------------
        [[nodiscard]]
        bool DecodeToWav(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputPath) {
            using Microsoft::WRL::ComPtr;

            // COM の初期化（呼び出し側で別モードで初期化済みなら、そのまま使う）
            const HRESULT comResult = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const bool    comOwned  = SUCCEEDED(comResult);
            if(FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
                Tsukino::Core::Log::Error("AudioImporter: Failed to initialize COM for audio decoding.");
                return false;
            }

            if(FAILED(::MFStartup(MF_VERSION, MFSTARTUP_LITE))) {
                Tsukino::Core::Log::Error("AudioImporter: Failed to start Media Foundation.");
                if(comOwned)
                    ::CoUninitialize();
                return false;
            }

            bool                 succeeded = false;
            std::vector<uint8_t> pcm;          // デコード済みの PCM データ
            std::vector<uint8_t> formatBytes;  // WAVEFORMATEX（拡張部分を含む）

            // ComPtr が MFShutdown より先に解放されるようスコープを切る
            do {
                // ソースリーダーの作成
                ComPtr<IMFSourceReader> reader;
                if(FAILED(::MFCreateSourceReaderFromURL(inputPath.ToWString().c_str(), nullptr, &reader))) {
                    Tsukino::Core::Log::Error("AudioImporter: Failed to open audio for decoding: " + inputPath.string());
                    break;
                }

                // 最初の音声ストリームだけを読む
                const DWORD audioStream = static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM);
                reader->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
                reader->SetStreamSelection(audioStream, TRUE);

                // 出力形式を 16bit PCM に指定（サンプルレート・チャンネル数は元のまま）
                ComPtr<IMFMediaType> requestType;
                if(FAILED(::MFCreateMediaType(&requestType))
                   || FAILED(requestType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio))
                   || FAILED(requestType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM))
                   || FAILED(requestType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16))
                   || FAILED(reader->SetCurrentMediaType(audioStream, nullptr, requestType.Get()))) {
                    Tsukino::Core::Log::Error("AudioImporter: Failed to request PCM output: " + inputPath.string());
                    break;
                }

                // 実際に決まった出力形式を WAVEFORMATEX として取得
                ComPtr<IMFMediaType> actualType;
                if(FAILED(reader->GetCurrentMediaType(audioStream, &actualType))) {
                    Tsukino::Core::Log::Error("AudioImporter: Failed to get decoded audio format: " + inputPath.string());
                    break;
                }

                WAVEFORMATEX* waveFormat     = nullptr;
                UINT32        waveFormatSize = 0;
                if(FAILED(::MFCreateWaveFormatExFromMFMediaType(actualType.Get(), &waveFormat, &waveFormatSize))) {
                    Tsukino::Core::Log::Error("AudioImporter: Failed to convert decoded audio format: " + inputPath.string());
                    break;
                }
                formatBytes.assign(reinterpret_cast<uint8_t*>(waveFormat), reinterpret_cast<uint8_t*>(waveFormat) + waveFormatSize);
                ::CoTaskMemFree(waveFormat);

                // 終端まで読み、PCM を連結
                bool readFailed = false;
                for(;;) {
                    DWORD           flags = 0;
                    ComPtr<IMFSample> sample;
                    if(FAILED(reader->ReadSample(audioStream, 0, nullptr, &flags, nullptr, &sample))) {
                        readFailed = true;
                        break;
                    }
                    if((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
                        break;
                    if(!sample)
                        continue;

                    ComPtr<IMFMediaBuffer> buffer;
                    if(FAILED(sample->ConvertToContiguousBuffer(&buffer))) {
                        readFailed = true;
                        break;
                    }

                    BYTE* data   = nullptr;
                    DWORD length = 0;
                    if(FAILED(buffer->Lock(&data, nullptr, &length))) {
                        readFailed = true;
                        break;
                    }
                    pcm.insert(pcm.end(), data, data + length);
                    buffer->Unlock();
                }

                if(readFailed || pcm.empty()) {
                    Tsukino::Core::Log::Error("AudioImporter: Failed to decode audio samples: " + inputPath.string());
                    break;
                }

                succeeded = true;
            } while(false);

            ::MFShutdown();
            if(comOwned)
                ::CoUninitialize();

            if(!succeeded)
                return false;

            //----------------------------------------------------------------------------
            // RIFF/WAVE（fmt + data チャンク）として書き出し
            //----------------------------------------------------------------------------
            std::ofstream file(outputPath.ToWString(), std::ios::binary | std::ios::trunc);
            if(!file) {
                Tsukino::Core::Log::Error("AudioImporter: Failed to create the intermediate wav: " + outputPath.string());
                return false;
            }

            const auto writeU32 = [&file](uint32_t value) { file.write(reinterpret_cast<const char*>(&value), sizeof(value)); };

            const uint32_t formatSize = static_cast<uint32_t>(formatBytes.size());
            const uint32_t dataSize   = static_cast<uint32_t>(pcm.size());

            file.write("RIFF", 4);
            writeU32(4 + (8 + formatSize) + (8 + dataSize));    // "WAVE" + fmt チャンク + data チャンク
            file.write("WAVE", 4);

            file.write("fmt ", 4);
            writeU32(formatSize);
            file.write(reinterpret_cast<const char*>(formatBytes.data()), formatSize);

            file.write("data", 4);
            writeU32(dataSize);
            file.write(reinterpret_cast<const char*>(pcm.data()), dataSize);

            if(!file) {
                Tsukino::Core::Log::Error("AudioImporter: Failed to write the intermediate wav: " + outputPath.string());
                return false;
            }
            return true;
        }
    }    // namespace

    //--------------------------------------------------------------
    //! @brief  外部プロセスを実行して終了コードを返す
    //--------------------------------------------------------------
    bool AudioImporter::RunProcess(const Tsukino::Core::Path& executablePath, const std::wstring& arguments, const Tsukino::Core::Path& workingDir) {   
		//--------------------------------------------------------------
        // 実行パスと引数を結合してコマンドラインを作成
        //--------------------------------------------------------------
        std::wstring         commandLine = L"\"" + executablePath.ToWString() + L"\" " + arguments;
        std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
        mutableCommandLine.push_back(L'\0');

        STARTUPINFOW        startupInfo{};
        PROCESS_INFORMATION processInfo{};
        startupInfo.cb = sizeof(STARTUPINFOW);

        const BOOL created = ::CreateProcessW(
            nullptr, mutableCommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, workingDir.ToWString().c_str(), &startupInfo, &processInfo);

        if(!created) {
            Tsukino::Core::Log::Error("Failed to launch XWBTool.exe. error=" + std::to_string(::GetLastError()));
            return false;
        }

        ::WaitForSingleObject(processInfo.hProcess, INFINITE);

        DWORD exitCode = 1;
        ::GetExitCodeProcess(processInfo.hProcess, &exitCode);

        ::CloseHandle(processInfo.hThread);
        ::CloseHandle(processInfo.hProcess);

        if(exitCode != 0) {
            Tsukino::Core::Log::Error("XWBTool.exe failed. exitCode=" + std::to_string(exitCode));
            return false;
        }

        return true;
    }

    //--------------------------------------------------------------
    //! @brief  フォントのインポート関数
    //--------------------------------------------------------------
    bool AudioImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // パス内のフラグメント（#以降）を除外
        //--------------------------------------------------------------
        std::string rawPath       = inputPath.string();
        size_t      fragmentPos   = rawPath.find('#');
        std::string basePathValue = (fragmentPos == std::string::npos) ? rawPath : rawPath.substr(0, fragmentPos);

        Tsukino::Core::Path baseInputPath(basePathValue);

        //--------------------------------------------------------------
        // 拡張子チェック（フラグメント除外後）
        //--------------------------------------------------------------
        std::string ext = baseInputPath.extension();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        // .mp3 は XWBTool が読めないため、一度 .wav へデコードしてから渡す
        const bool needsDecode = (ext == ".mp3");
        if(ext != ".wav" && !needsDecode)
            return false;

        //--------------------------------------------------------------
        // 絶対入力パスの取得
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / baseInputPath;

        //--------------------------------------------------------------
        // 出力パスの決定（.xwb）
        // (baseInputPathがエンジン組み込みアセット由来の絶対パスの場合、そのまま
        //  outputDirectory / baseInputPath とすると絶対パスへ丸ごと置き換わってしまい、
        //  エンジンのソースツリー内に.xwbを書き込んでしまう。
        //  ToEngineRelativePath()で相対パスに戻してから結合する)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(baseInputPath);
        outputPath.replace_extension(".xwb");

        //--------------------------------------------------------------
        // 親ディレクトリ作成
        //--------------------------------------------------------------
        // 出力先を作れないまま書き込みへ進むと、失敗が原因から遠い場所で
        // 「キャッシュが無い」として現れるため、ここで止める
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("AudioImporter: Failed to create the output directory: "
                                      + outputPath.parent_path().string());
            return false;
        }

        //--------------------------------------------------------------
        // XWBTool.exe のパス
        // (エンジン自身が所有するツールのため、取り込み側リポジトリの
        //  GetAssetRootPath()ではなくGetEngineAssetRootPath()から解決する)
        //--------------------------------------------------------------
        Tsukino::Core::Path toolPath = Tsukino::IO::FileSystem::GetEngineAssetRootPath() / "Tools/XWBTool.exe";

        //--------------------------------------------------------------
        // .mp3 は出力先の隣へ中間 .wav をデコードし、それを XWBTool の入力にする
        //--------------------------------------------------------------
        Tsukino::Core::Path toolInputPath = absoluteInputPath;
        Tsukino::Core::Path intermediateWavPath;
        if(needsDecode) {
            intermediateWavPath = outputPath;
            intermediateWavPath.replace_extension(".decode.wav");
            if(!DecodeToWav(absoluteInputPath, intermediateWavPath)) {
                ::DeleteFileW(intermediateWavPath.ToWString().c_str());
                Tsukino::Core::Log::Error("Failed to convert audio: " + absoluteInputPath.string() + " -> " + outputPath.string());
                return false;
            }
            toolInputPath = intermediateWavPath;
        }

        //--------------------------------------------------------------
        // 引数を作成して変換実行
        //--------------------------------------------------------------
        std::wstring arguments  = L"-nologo -y -nc -o ";
        arguments              += L"\"" + outputPath.ToWString() + L"\" ";
        arguments              += L"\"" + toolInputPath.ToWString() + L"\"";

        const bool converted = RunProcess(toolPath, arguments, baseDir);

        // 中間 .wav は変換の成否に関わらず消す（キャッシュに残すと .xwb の数倍の容量を食う）
        if(needsDecode)
            ::DeleteFileW(intermediateWavPath.ToWString().c_str());

        if(!converted) {
            Tsukino::Core::Log::Error("Failed to convert audio: " + absoluteInputPath.string() + " -> " + outputPath.string());
            return false;
        }

        Tsukino::Core::Log::Info("Audio imported: " + outputPath.string());
        return true;
    }

}    // namespace Tsukino::Asset
