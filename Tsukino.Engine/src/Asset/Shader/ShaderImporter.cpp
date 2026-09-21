//--------------------------------------------------------------
//! @file   ShaderImporter.cpp
//! @brief  シェーダーインポーター
//! @author 山﨑愛
//--------------------------------------------------------------
#include <Tsukino/Engine/Asset/Shader/ShaderImporter.hpp>

#include <Tsukino/Core/Log.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>

#include <d3dcompiler.h>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
// 名前空間 Tsukino::Asset
namespace Tsukino::Asset {
    namespace {
        //--------------------------------------------------------------
        //! エンジン組み込みシェーダー（Scene.hlsli等の共有ヘッダ）の置き場所を返します。
        //--------------------------------------------------------------
        Tsukino::Core::Path BuiltInShaderDirectory() {
            return Tsukino::IO::FileSystem::GetEngineAssetRootPath() / Tsukino::Core::Path(std::string("Tsukino.BuiltIn/Assets/Shaders"));
        }

        //--------------------------------------------------------------
        //! @class ShaderIncludeHandler
        //! @brief #include の探索先にエンジン組み込みシェーダーの置き場所を足すハンドラ
        //! @note  D3D_COMPILE_STANDARD_FILE_INCLUDE はインクルード元からの相対しか見ない。
        //!        そのためゲーム側のシェーダーからはエンジンのScene.hlsliへ手が届かず、
        //!        b0の宣言をゲーム側で書き写すしかなかった。書き写した宣言はいつか本体と
        //!        ずれる（実際にShadowMapStatic.vs.hlslがinvViewProjを落として
        //!        スタティックメッシュの影が壊れていた）ので、探索先を1つ増やして
        //!        どのリポジトリのシェーダーからでもincludeできるようにする。
        //!
        //!        探索順は「インクルード元のディレクトリ → エンジンのShaders」。
        //!        ゲーム側が同名のファイルを置いたときはゲーム側が勝つ
        //--------------------------------------------------------------
        class ShaderIncludeHandler final : public ID3DInclude {
        public:
            //----------------------------------------------------------
            //! @param [in] sourceDirectory コンパイル対象のシェーダーが置かれたディレクトリ
            //----------------------------------------------------------
            explicit ShaderIncludeHandler(Tsukino::Core::Path sourceDirectory)
                : m_sourceDirectory(std::move(sourceDirectory))
                , m_builtInDirectory(BuiltInShaderDirectory()) {}

            //----------------------------------------------------------
            //! インクルード先を開きます。
            //! @note 実体はCloseが呼ばれるまでこのクラスが握り続ける
            //----------------------------------------------------------
            HRESULT __stdcall Open(D3D_INCLUDE_TYPE, LPCSTR fileName, LPCVOID parentData, LPCVOID* outData, UINT* outBytes) override {
                *outData  = nullptr;
                *outBytes = 0;

                if(!fileName)
                    return E_FAIL;

                //--------------------------------------------------
                // 入れ子のincludeは親バッファが読まれた場所を基準にする。
                // 親が分からない（＝コンパイル対象そのものからのinclude）ときは
                // コンパイル対象のディレクトリを使う
                //--------------------------------------------------
                const auto                parent    = m_directoryOf.find(parentData);
                const Tsukino::Core::Path searchDir = (parent != m_directoryOf.end()) ? parent->second : m_sourceDirectory;

                Tsukino::Core::Path resolved = searchDir / Tsukino::Core::Path(std::string(fileName));
                if(!Tsukino::IO::FileSystem::Exists(resolved)) {
                    resolved = m_builtInDirectory / Tsukino::Core::Path(std::string(fileName));
                    if(!Tsukino::IO::FileSystem::Exists(resolved))
                        return E_FAIL;
                }

                auto buffer = std::make_unique<std::vector<std::uint8_t>>(Tsukino::IO::FileSystem::ReadBinary(resolved));

                // data()がnullptrを返さないよう最低1バイト持たせる（空のヘッダでも通す）
                if(buffer->empty())
                    buffer->push_back('\n');

                void* data = buffer->data();
                *outData   = data;
                *outBytes  = static_cast<UINT>(buffer->size());

                m_directoryOf[data] = resolved.parent_path();
                m_buffers[data]     = std::move(buffer);
                return S_OK;
            }

            //----------------------------------------------------------
            //! Openで開いた実体を解放します。
            //----------------------------------------------------------
            HRESULT __stdcall Close(LPCVOID data) override {
                m_directoryOf.erase(data);
                m_buffers.erase(data);
                return S_OK;
            }

        private:
            Tsukino::Core::Path                                           m_sourceDirectory;    // コンパイル対象のディレクトリ
            Tsukino::Core::Path                                           m_builtInDirectory;   // エンジン組み込みシェーダーのディレクトリ
            std::map<LPCVOID, Tsukino::Core::Path>                        m_directoryOf;        // 開いたバッファ → そのファイルのディレクトリ
            std::map<LPCVOID, std::unique_ptr<std::vector<std::uint8_t>>> m_buffers;            // Closeまで生かしておく実体
        };
    }    // namespace

    //--------------------------------------------------------------
    //! @brief  シェーダーアセットをインポートする関数
    //--------------------------------------------------------------
    bool ShaderImporter::Import(const Tsukino::Core::Path& inputPath, const Tsukino::Core::Path& outputDirectory) {
        //--------------------------------------------------------------
        // 絶対パスの取得
        //--------------------------------------------------------------
        Tsukino::Core::Path baseDir           = Tsukino::IO::FileSystem::GetAssetRootPath();
        Tsukino::Core::Path absoluteInputPath = baseDir / inputPath;

        //--------------------------------------------------------------
        // 出力パスの決定
        // (inputPathがエンジン組み込みアセット由来の絶対パスの場合、そのまま
        //  outputDirectory / inputPath とすると絶対パスへ丸ごと置き換わってしまい、
        //  エンジンのソースツリー内に.csoを書き込んでしまう。ToEngineRelativePath()で
        //  相対パスに戻してから結合する)
        //--------------------------------------------------------------
        Tsukino::Core::Path outputPath = outputDirectory / Tsukino::IO::FileSystem::ToEngineRelativePath(inputPath);
        outputPath.replace_extension(".cso");

        //--------------------------------------------------------------
        // 親ディレクトリ作成
        //--------------------------------------------------------------
        // 出力先を作れないまま書き込みへ進むと、失敗が原因から遠い場所で
        // 「キャッシュが無い」として現れるため、ここで止める
        if(!Tsukino::IO::FileSystem::CreateDirectories(outputPath.parent_path())) {
            Tsukino::Core::Log::Error("ShaderImporter: Failed to create the output directory: "
                                      + outputPath.parent_path().string());
            return false;
        }

        //------------------------------------------------
        // シェーダー種別を拡張子から判定
        //------------------------------------------------
        std::string ext = inputPath.string();
        std::string target;
        std::string entrypoint;    // エントリポイント名

        // 拡張子に基づいてシェーダーのターゲットを決定
        if(ext.ends_with(".vs.hlsl")) {
            target     = "vs_5_0";
            entrypoint = "VSMain";    // 頂点シェーダーのエントリポイント名
        } else if(ext.ends_with(".ps.hlsl")) {
            target     = "ps_5_0";
            entrypoint = "PSMain";    // ピクセルシェーダーのエントリポイント名
        } else {
            Tsukino::Core::Log::Error("Unknown shader type: " + ext);
            return false;
        }

        //------------------------------------------------
        // 出力パス
        //------------------------------------------------
        std::string name = inputPath.stem();    // xxx.vs + .cso

        //------------------------------------------------
        // コンパイルフラグ
        //------------------------------------------------
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        //------------------------------------------------
        // シェーダーコンパイル
        //------------------------------------------------
        ID3DBlob* shaderBlob = nullptr;
        ID3DBlob* errorBlob  = nullptr;

        // #includeの解決。標準ハンドラはインクルード元からの相対しか見ないため、
        // エンジン組み込みシェーダーの置き場所も探す自前のハンドラを使う
        ShaderIncludeHandler includeHandler(absoluteInputPath.parent_path());

        HRESULT hr = D3DCompileFromFile(absoluteInputPath.ToWString().c_str(),
                                        nullptr,
                                        &includeHandler,
                                        entrypoint.c_str(),
                                        target.c_str(),
                                        flags,
                                        0,
                                        &shaderBlob,
                                        &errorBlob);

        if(FAILED(hr)) {
            if(errorBlob) {
                Tsukino::Core::Log::Error((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }
            return false;
        }

        //------------------------------------------------
        // .cso 保存
        //------------------------------------------------
        std::ofstream file(outputPath.string(), std::ios::binary);
        if(!file)
            return false;
        file.write((char*)shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

        shaderBlob->Release();

        Tsukino::Core::Log::Info("Shader compiled: " + outputPath.string());
        return true;
    }

    namespace {
        //--------------------------------------------------------------
        //! 1ファイル分の #include "..." を辿り、依存先を集めます。
        //! @param [in]     filePath 走査するファイルの絶対パス
        //! @param [in,out] visited  既に辿ったファイル（循環インクルード対策）
        //! @param [in,out] outDeps  見つかった依存先の追加先
        //! @note  D3D_COMPILE_STANDARD_FILE_INCLUDE はインクルード元からの相対で
        //!        解決するため、ここでも同じくインクルード元のディレクトリを基準にする。
        //!        山括弧の #include <...> は探索パス依存で解決先が一意に決まらないので
        //!        追わない（HLSL側は引用符の形しか使っていない）
        //--------------------------------------------------------------
        void CollectIncludesRecursive(const Tsukino::Core::Path& filePath, std::set<std::string>& visited,
                                      std::vector<Tsukino::Core::Path>& outDeps) {
            // 同じファイルを二度辿らない。循環インクルードがあっても止まらなくなる
            if(!visited.insert(Tsukino::Core::Path::ToLower(filePath.string())).second)
                return;

            std::ifstream file(filePath.string());
            if(!file)
                return;

            const Tsukino::Core::Path parentDir = filePath.parent_path();

            std::string line;
            while(std::getline(file, line)) {
                //--------------------------------------------------------------
                // #include "..." だけを拾う。厳密なプリプロセッサではないので、
                // コメントアウトされた行まで拾ってしまうことはあるが、
                // 「依存を多めに見る」方向の誤りなので余計に再コンパイルされるだけで済む
                //--------------------------------------------------------------
                const std::size_t includePos = line.find("#include");
                if(includePos == std::string::npos)
                    continue;

                const std::size_t firstQuote = line.find('"', includePos);
                if(firstQuote == std::string::npos)
                    continue;

                const std::size_t lastQuote = line.find('"', firstQuote + 1);
                if(lastQuote == std::string::npos)
                    continue;

                const std::string includeName = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                if(includeName.empty())
                    continue;

                // 探索順はShaderIncludeHandler::Openと必ず揃えること。
                // ここが食い違うと、Scene.hlsliを直してもゲーム側シェーダーの
                // キャッシュが無効化されず、古い.csoを使い続けてしまう
                Tsukino::Core::Path includePath = parentDir / Tsukino::Core::Path(includeName);
                if(!Tsukino::IO::FileSystem::Exists(includePath)) {
                    includePath = BuiltInShaderDirectory() / Tsukino::Core::Path(includeName);
                    if(!Tsukino::IO::FileSystem::Exists(includePath))
                        continue;
                }

                outDeps.push_back(includePath);

                // 入れ子のインクルードも追う
                CollectIncludesRecursive(includePath, visited, outDeps);
            }
        }
    }    // namespace

    //--------------------------------------------------------------
    //! シェーダーが #include している .hlsli を再帰的に列挙します。
    //--------------------------------------------------------------
    std::vector<Tsukino::Core::Path> ShaderImporter::CollectDependencies(const Tsukino::Core::Path& inputPath) const {
        std::vector<Tsukino::Core::Path> dependencies;

        const Tsukino::Core::Path baseDir      = Tsukino::IO::FileSystem::GetAssetRootPath();
        const Tsukino::Core::Path absolutePath = baseDir / inputPath;

        std::set<std::string> visited;
        CollectIncludesRecursive(absolutePath, visited, dependencies);

        return dependencies;
    }

}    // namespace Tsukino::Asset
