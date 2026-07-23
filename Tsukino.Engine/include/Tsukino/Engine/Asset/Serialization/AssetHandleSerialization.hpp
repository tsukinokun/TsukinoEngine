#pragma once
#include <Tsukino/Engine/Asset/AssetHandle.hpp>

#include <cereal/cereal.hpp>

namespace Tsukino::Asset {

    template <class Archive>
    void save(Archive& archive, const AssetHandle& handle) {
        archive(handle.Value());
    }

    template <class Archive>
    void load(Archive& archive, AssetHandle& handle) {
        u64 value = 0;
        archive(value);
        handle = AssetHandle(value);
    }

}
