//-------------------------------------------------------------
//! @file   LightStressTestComponent.hpp
//! @brief  多光源ストレステスト用HUDタグコンポーネントの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once

// 名前空間 : Tsukino::Sandbox::DebugTools::ECS
namespace Tsukino::Sandbox::DebugTools::ECS {
    //-------------------------------------------------------------
    //! @struct LightStressTestHudComponent
    //! @brief  LightStressTestSystemが毎フレームtextを書き換えるHUDの目印
    //! @note   FontComponentと同じエンティティに付ける。
    //!         対象を特定するためだけのタグだが、EnTTは空の型を
    //!         「空型最適化」の対象にして emplace が void を返し View::each にも
    //!         渡さなくなるため、既存のDebugCameraTagと同じくダミーメンバを持たせる。
    //-------------------------------------------------------------
    struct LightStressTestHudComponent {
        bool dummy = true;
    };

}    // namespace Tsukino::Sandbox::DebugTools::ECS
