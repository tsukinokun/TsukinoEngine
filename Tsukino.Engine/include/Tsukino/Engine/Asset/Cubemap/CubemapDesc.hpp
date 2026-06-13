//--------------------------------------------------------------
//! @file   CubemapDesc.hpp
//! @brief  キューブマップの設定データ
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <string>
#include <cereal/cereal.hpp>
// 名前空間 : Tsukino::Asset
namespace Tsukino::Asset {
    //--------------------------------------------------------------
    //! @struct CubemapDesc
    //! @brief  .cubemapファイルの内容
    //--------------------------------------------------------------
    struct CubemapDesc {
        std::string px;    // +X 右
        std::string nx;    // -X 左
        std::string py;    // +Y 上
        std::string ny;    // -Y 下
        std::string pz;    // +Z 前
        std::string nz;    // -Z 後

        template <class Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("px", px),
                    cereal::make_nvp("nx", nx),
                    cereal::make_nvp("py", py),
                    cereal::make_nvp("ny", ny),
                    cereal::make_nvp("pz", pz),
                    cereal::make_nvp("nz", nz));
        }
    };
}    // namespace Tsukino::Asset
