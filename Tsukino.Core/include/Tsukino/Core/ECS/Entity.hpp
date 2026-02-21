//---------------------------------------------------------------
//! @file   Entity.hpp
//! @brief  エンティティIDの定義
//! @author 山﨑愛
//---------------------------------------------------------------
#pragma once
#include <entt/entt.hpp>
// 名前空間 : Tsukino::ECS
namespace tsukino::ECS {
    // enttライブラリの型を型エイリアスとして定義してラッパする
    using Entity = entt::entity;
}    // namespace tsukino::ECS
