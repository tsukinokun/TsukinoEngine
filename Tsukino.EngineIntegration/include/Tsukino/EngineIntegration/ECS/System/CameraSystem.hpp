//-------------------------------------------------------------
//! @file   CameraSystem.hpp
//! @brief  CameraSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //-------------------------------------------------------------
    //! @class  CameraSystem
    //! @brief  CameraComponentを持つエンティティのビュー行列と
    //-------------------------------------------------------------
    class CameraSystem : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        //! @brief 更新処理
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        float m_lastScreenWidth  = 0.0f;    // 前回行列を作ったときの画面の幅（大きさが変わったら全カメラを作り直す）
        float m_lastScreenHeight = 0.0f;    // 同じく高さ
    };
}    // namespace Tsukino::BuiltIn::ECS
