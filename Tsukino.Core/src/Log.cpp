//--------------------------------------------------------------
//! @file   Log.cpp
//! @brief  ログ出力クラスの実装
//! @author 山﨑愛
//--------------------------------------------------------------
#include "Tsukino/Core/Log.h"
// 名前空間 : Tsukino::Core
namespace Tsukino::Core {

    //--------------------------------------------------------------
    //! @brief 情報ログ出力
    //--------------------------------------------------------------
    void Log::Info(const std::string& msg) {
        std::string out = "[Info] " + msg + "\n";
        OutputDebugStringA(out.c_str());
    }

    //--------------------------------------------------------------
    //! @brief 警告ログ出力
    //--------------------------------------------------------------
    void Log::Warn(const std::string& msg) {
        std::string out = "[Warn] " + msg + "\n";
        OutputDebugStringA(out.c_str());
    }

    //--------------------------------------------------------------
    //! @brief エラーログ出力
    //--------------------------------------------------------------
    void Log::Error(const std::string& msg) {
        std::string out = "[Error] " + msg + "\n";
        OutputDebugStringA(out.c_str());
    }

}    // namespace Tsukino::Core
