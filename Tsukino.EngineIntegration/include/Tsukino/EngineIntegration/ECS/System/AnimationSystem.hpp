//-------------------------------------------------------------
//! @file   AnimationSystem.hpp
//! @brief  AnimationSystemクラスの宣言
//! @author 山﨑愛
//-------------------------------------------------------------
#pragma once
#include <Tsukino/Core/ECS/System/ISystem.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

// 前方宣言
namespace Tsukino::GraphicsCommon {
    struct AnimationData;
    struct ModelData;
}    // namespace Tsukino::GraphicsCommon

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //-------------------------------------------------------------
    //! @class  AnimationSystem
    //! @brief  アニメーションを処理するシステム
    //-------------------------------------------------------------
    class AnimationSystem final : public Tsukino::ECS::ISystem {
    public:
        //-------------------------------------------------------------
        // システムの更新
        //! @param  registry    [in] ECS レジストリ
        //! @param  deltaTime   [in] 前フレームからの経過時間
        //-------------------------------------------------------------
        void Update(Tsukino::ECS::Registry& registry, float deltaTime) override;

    private:
        //-------------------------------------------------------------
        //! @struct ChannelTableKey
        //! @brief  ノード→チャンネル対応表のキー（クリップとスケルトンの組）
        //! @note   同じクリップでも、どのモデルのスケルトンへ適用するかで対応表は変わる
        //!         （リターゲット）。そのため両方をキーにする。
        //!         アセットはAssetManagerが解放しないためポインタは安定している
        //-------------------------------------------------------------
        struct ChannelTableKey {
            const void* animation = nullptr;
            const void* skeleton  = nullptr;

            bool operator==(const ChannelTableKey& other) const noexcept {
                return animation == other.animation && skeleton == other.skeleton;
            }
        };

        //-------------------------------------------------------------
        //! @struct ChannelTableKeyHash
        //! @brief  ChannelTableKey用のハッシュ関数
        //-------------------------------------------------------------
        struct ChannelTableKeyHash {
            [[nodiscard]]
            std::size_t operator()(const ChannelTableKey& key) const noexcept {
                const std::size_t a = std::hash<const void*>{}(key.animation);
                const std::size_t b = std::hash<const void*>{}(key.skeleton);
                return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2));
            }
        };

        //-------------------------------------------------------------
        //! @brief  ノード→チャンネルの対応表を取得する関数（無ければ構築してキャッシュ）
        //! @param  animation [in] アニメーションクリップ
        //! @param  model     [in] スケルトンの取得元モデル
        //! @return ノードindexを添字に、対応するチャンネルindex（無ければ-1）を並べた配列
        //! @note   以前は毎フレーム・毎エンティティ・全ノードについて全チャンネルを
        //!         std::stringで線形比較していた（Mixamoリグで1体あたり約4200回、
        //!         クロスフェード中は倍）。敵を数百体出すと毎フレーム数百万回の
        //!         文字列比較になり、ここが全体の支配的なコストになっていた
        //-------------------------------------------------------------
        const std::vector<std::int32_t>& ResolveChannelTable(const Tsukino::GraphicsCommon::AnimationData& animation,
                                                             const Tsukino::GraphicsCommon::ModelData&     model);

        //! @brief ノード→チャンネル対応表のキャッシュ
        std::unordered_map<ChannelTableKey, std::vector<std::int32_t>, ChannelTableKeyHash> m_channelTables;
    };

}    // namespace Tsukino::BuiltIn::ECS
