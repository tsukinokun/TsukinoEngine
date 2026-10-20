//-------------------------------------------------------------
//! @file   HotReload.cpp
//! @brief  HotReload の実装
//-------------------------------------------------------------
#include <Tsukino/Core/IO/HotReload.hpp>

#include <Tsukino/Core/DebugTools/DebugFeatures.hpp>
#include <Tsukino/Core/IO/FileSystem.hpp>
#include <Tsukino/Core/IO/FileWatcher.hpp>
#include <Tsukino/Core/Log.hpp>

#include <cereal/archives/json.hpp>    // 同梱のrapidjsonをcerealと同じ設定で使う

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// 名前空間 : Tsukino::IO::HotReload
namespace Tsukino::IO::HotReload {
#ifdef TSUKINO_ENABLE_HOT_RELOAD
    namespace {
        //! 1回の読み直しでログへ出す差分の行数の上限
        constexpr size_t kMaxDiffLines = 20;

        //! cereal同梱のrapidjson
        namespace Json = CEREAL_RAPIDJSON_NAMESPACE;

        //-------------------------------------------------------------
        //! 監視の状態です（プロセスに1つ）。
        //-------------------------------------------------------------
        struct State {
            FileWatcher       watcher;
            std::atomic<bool> paused{ false };
        };

        State& GetState() {
            static State s_state;
            return s_state;
        }

        //-------------------------------------------------------------
        //! JSONの値を1行の文字列にする関数（差分のログ用）
        //-------------------------------------------------------------
        std::string ToText(const Json::Value& value) {
            if(value.IsNumber()) {
                if(value.IsInt64())
                    return std::to_string(value.GetInt64());
                char buffer[64];
                std::snprintf(buffer, sizeof(buffer), "%g", value.GetDouble());
                return buffer;
            }
            if(value.IsString())
                return std::string("\"") + value.GetString() + "\"";
            if(value.IsBool())
                return value.GetBool() ? "true" : "false";
            if(value.IsNull())
                return "null";
            if(value.IsArray())
                return "[" + std::to_string(value.Size()) + " items]";
            return "{...}";
        }

        //-------------------------------------------------------------
        //! 2つのJSONの値を葉までたどり、違うところを lines へ足す関数
        //! @param  path   [in]     ここまでのキーの並び（例: "break.downSeconds"）
        //! @param  before [in]     前回読めた値（無いときは nullptr）
        //! @param  after  [in]     今回の値（無いときは nullptr）
        //! @param  lines  [in,out] 差分の行
        //-------------------------------------------------------------
        void CollectDiff(const std::string& path, const Json::Value* before, const Json::Value* after, std::vector<std::string>& lines) {
            if(before == nullptr && after == nullptr)
                return;
            if(before == nullptr) {
                lines.push_back(path + ": " + ToText(*after) + " (added)");
                return;
            }
            if(after == nullptr) {
                lines.push_back(path + ": " + ToText(*before) + " (removed)");
                return;
            }

            if(before->IsObject() && after->IsObject()) {
                const std::string prefix = path.empty() ? "" : path + ".";
                for(auto it = before->MemberBegin(); it != before->MemberEnd(); ++it) {
                    const auto found = after->FindMember(it->name);
                    CollectDiff(prefix + it->name.GetString(), &it->value, found != after->MemberEnd() ? &found->value : nullptr, lines);
                }
                for(auto it = after->MemberBegin(); it != after->MemberEnd(); ++it) {
                    if(!before->HasMember(it->name))
                        CollectDiff(prefix + it->name.GetString(), nullptr, &it->value, lines);
                }
                return;
            }

            if(before->IsArray() && after->IsArray()) {
                const Json::SizeType count = std::max(before->Size(), after->Size());
                for(Json::SizeType i = 0; i < count; ++i) {
                    CollectDiff(path + "[" + std::to_string(i) + "]", i < before->Size() ? &(*before)[i] : nullptr,
                                i < after->Size() ? &(*after)[i] : nullptr, lines);
                }
                return;
            }

            if(*before != *after)
                lines.push_back(path + ": " + ToText(*before) + " -> " + ToText(*after));
        }

        //-------------------------------------------------------------
        //! 前回読めた本文と今回の本文の差分をログへ出す関数
        //! @note   ルートキーが1つだけのとき（{"HealthBar": {...}} など）は、その下から書く
        //-------------------------------------------------------------
        void LogDiff(const std::string& fileName, const std::string& beforeText, const std::string& afterText) {
            Json::Document before;
            Json::Document after;
            before.Parse(beforeText.c_str());
            after.Parse(afterText.c_str());
            if(before.HasParseError() || after.HasParseError())
                return;

            const Json::Value* beforeRoot = &before;
            const Json::Value* afterRoot  = &after;
            if(before.IsObject() && after.IsObject() && before.MemberCount() == 1 && after.MemberCount() == 1
               && before.MemberBegin()->name == after.MemberBegin()->name) {
                beforeRoot = &before.MemberBegin()->value;
                afterRoot  = &after.MemberBegin()->value;
            }

            std::vector<std::string> lines;
            CollectDiff("", beforeRoot, afterRoot, lines);

            if(lines.empty()) {
                Tsukino::Core::Log::Info("Hot reload changed: " + fileName + "  (no value changed)");
                return;
            }
            for(size_t i = 0; i < lines.size() && i < kMaxDiffLines; ++i)
                Tsukino::Core::Log::Info("Hot reload changed: " + fileName + "  " + lines[i]);
            if(lines.size() > kMaxDiffLines)
                Tsukino::Core::Log::Info("Hot reload changed: " + fileName + "  ... and " + std::to_string(lines.size() - kMaxDiffLines) + " more");
        }
    }    // namespace

    //-------------------------------------------------------------
    //! 監視するファイルを登録します。
    //-------------------------------------------------------------
    void Watch(const Tsukino::Core::Path& path, std::function<bool()> reload) {
        // 差分を出すため、前回読めた本文を覚えておく（reload と同じく Poll のスレッドからしか触らない）
        auto lastText = std::make_shared<std::string>(FileSystem::ReadText(path));

        GetState().watcher.Watch(path, [path, lastText, reload = std::move(reload)] {
            if(!reload()) {
                Tsukino::Core::Log::Error("Hot reload failed, keeping previous values: " + path.string());
                return;
            }

            std::string text = FileSystem::ReadText(path);
            Tsukino::Core::Log::Info("Hot reloaded: " + path.string());
            LogDiff(path.string(), *lastText, text);
            *lastText = std::move(text);
        });
    }

    //-------------------------------------------------------------
    //! 更新されたファイルを読み直します。
    //-------------------------------------------------------------
    void Poll() {
        State& state = GetState();
        if(!state.paused.load())
            state.watcher.Poll();
    }

    //-------------------------------------------------------------
    //! 読み直しを止める・再開します。
    //-------------------------------------------------------------
    void SetPaused(bool paused) {
        GetState().paused.store(paused);
    }
#else
    void Watch(const Tsukino::Core::Path& /*path*/, std::function<bool()> /*reload*/) {}
    void Poll() {}
    void SetPaused(bool /*paused*/) {}
#endif
}    // namespace Tsukino::IO::HotReload
