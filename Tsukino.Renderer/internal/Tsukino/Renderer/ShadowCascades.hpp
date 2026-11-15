//----------------------------------------------------------------------------
//! @file   ShadowCascades.hpp
//! @brief  カスケードシャドウの投影の計算（Renderer の内部用）
//! @detail カメラの視錐台を奥行きで切り、1切れごとのライト空間の ViewProjection 行列を求めます。
//!         描画 API に依らない計算だけを集めてあり、ShadowPass が使います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/ConstantBuffer.hpp>
#include <Tsukino/Renderer/DrawCommand.hpp>
#include <Tsukino/Renderer/ShadowSettings.hpp>

#include <array>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! ワールド空間の視錐台の8頂点と、near 面・far 面の奥行きです。
    //! @note  nearPlane[i] と farPlane[i] は同じ稜線（カメラから伸びる辺）の両端
    //------------------------------------------------------------------------
    struct FrustumCorners {
        std::array<hlslpp::float3, 4> nearPlane;     // near 面の4頂点（ワールド）
        std::array<hlslpp::float3, 4> farPlane;      // far 面の4頂点（ワールド）
        float                         nearDepth = 0.0f;    // near 面の奥行き（カメラのビュー空間 z）
        float                         farDepth  = 0.0f;    // far 面の奥行き（同上）
    };

    //------------------------------------------------------------------------
    //! 点の集まりを包む球です。
    //------------------------------------------------------------------------
    struct BoundingSphere {
        hlslpp::float3 center = hlslpp::float3(0.0f, 0.0f, 0.0f);    // 中心（ワールド）
        float          radius = 0.0f;                                // 半径（ワールド距離）
    };

    //------------------------------------------------------------------------
    //! カスケード1枚ぶんの計算結果です。
    //------------------------------------------------------------------------
    struct ShadowCascade {
        float          splitNear      = 0.0f;    // 受け持つ奥行きの手前（カメラのビュー空間 z）
        float          splitFar       = 0.0f;    // 受け持つ奥行きの奥（同上）
        BoundingSphere bounds;                   // 視錐台の切れ端を包む球。半径がそのまま平行投影の半径になる
        float          texelWorldSize = 0.0f;    // シャドウマップ1テクセルが覆うワールド幅

        Tsukino::Core::Math::matrix lightViewProj = Tsukino::Core::Math::matrix::identity();    // ライト空間の ViewProjection
    };

    //------------------------------------------------------------------------
    //! 1フレームぶんのカスケード一式です。
    //------------------------------------------------------------------------
    struct ShadowCascadeSet {
        std::array<ShadowCascade, kShadowCascadeCount> cascades{};    // 近い順のカスケード

        //! 平行投影の奥行き。カスケードを包む球の中心から、光の来る側へ depthTowardLight、
        //! 向こう側へ depthAwayFromLight までを収める。全カスケードで同じ値を使い、
        //! シェーダーはこの合計で深度バイアスをワールド距離から深度値へ換算する
        float depthTowardLight   = 0.0f;
        float depthAwayFromLight = 0.0f;
    };

}    // namespace Tsukino::Renderer

// 名前空間 : Tsukino::Renderer::ShadowCascades
namespace Tsukino::Renderer::ShadowCascades {

    //! カスケードの枚数。b0のcascadeViewProjの要素数と同じでなければならない
    inline constexpr u32 kCascadeCount = kShadowCascadeCount;

    //! スキンメッシュのバウンドを膨らませる倍率。
    //! MeshDataのAABBはバインドポーズのものなので、アニメで手足がはみ出す
    inline constexpr float kSkinnedBoundsInflate = 1.6f;

    //! カメラの視錐台をカスケードに切り分け、1枚ずつのライト空間の投影を求めます。
    //! @param  [in] cameraView        カメラの View 行列
    //! @param  [in] cameraInvViewProj カメラの ViewProjection の逆行列
    //! @param  [in] lightDir          光の向き（正規化済み）
    //! @param  [in] settings          影の調整値（丸め済み）
    //! @param  [in] activeCount       実際に使うカスケードの枚数（1〜kCascadeCount）
    //! @return 近い順に kCascadeCount 枚のカスケードと、全カスケード共通の平行投影の奥行き。
    //!         activeCount 枚目より後ろは最後の有効なカスケードの複製になる
    //! @note   次の関数を順に呼ぶだけの関数。手順ごとの中身はそれぞれの関数を見る
    //!         ComputeFrustumCorners → ComputeSplitDepths → ComputeSliceCorners → ComputeBoundingSphere → ComputeLightViewProj
    [[nodiscard]]
    ShadowCascadeSet ComputeCascades(const Tsukino::Core::Math::matrix& cameraView,
                                     const Tsukino::Core::Math::matrix& cameraInvViewProj,
                                     const hlslpp::float3&              lightDir,
                                     const ShadowSettings&              settings,
                                     u32                                activeCount);

    //! カメラの視錐台の8頂点を、NDC の立方体の頂点を逆射影して求めます。
    //! @param  [in] cameraView        カメラの View 行列（頂点の奥行きを求めるのに使う）
    //! @param  [in] cameraInvViewProj カメラの ViewProjection の逆行列
    //! @return ワールド空間の8頂点と、near 面・far 面の奥行き
    //! @note   リバースZなので NDC の z=1 が near 面、z=0 が far 面になる
    [[nodiscard]]
    FrustumCorners ComputeFrustumCorners(const Tsukino::Core::Math::matrix& cameraView, const Tsukino::Core::Math::matrix& cameraInvViewProj);

    //! 視錐台を奥行き方向に切る距離を求めます。
    //! @param  [in] nearDepth カメラの near 面の奥行き
    //! @param  [in] farDepth  カメラの far 面の奥行き
    //! @param  [in] settings  影の調整値（distance / splitStart / splitLambda を使う）
    //! @param  [in] activeCount 実際に使うカスケードの枚数（1〜kCascadeCount）
    //! @return 近い順に kCascadeCount + 1 個の奥行き。i 番目と i+1 番目の間がカスケード i の受け持ち。
    //!         先頭は nearDepth、activeCount 番目から後ろは settings.distance と farDepth の近いほう
    //! @note   practical split scheme（対数分割と等分割の重み付き平均。splitLambda が対数側の重み）で求める。
    //!         式の起点は nearDepth ではなく splitStart（カメラの near より奥ならそちら）
    [[nodiscard]]
    std::array<float, kCascadeCount + 1> ComputeSplitDepths(float nearDepth, float farDepth, const ShadowSettings& settings, u32 activeCount);

    //! 視錐台を2つの奥行きで切った切れ端の8頂点を求めます。
    //! @param  [in] frustum   カメラの視錐台
    //! @param  [in] splitNear 切れ端の手前の奥行き
    //! @param  [in] splitFar  切れ端の奥の奥行き
    //! @return 切れ端の8頂点（ワールド。先頭4つが手前、後ろ4つが奥）
    [[nodiscard]]
    std::array<hlslpp::float3, 8> ComputeSliceCorners(const FrustumCorners& frustum, float splitNear, float splitFar);

    //! 点の集まりを包む球を求めます。
    //! @param  [in] corners 包む点（視錐台の切れ端の8頂点）
    //! @return 重心を中心とし、最も遠い点までを半径とした球
    //! @note   半径は切り上げて整える（カメラを回したときの浮動小数点の揺れで、毎フレーム大きさが変わらないように）
    [[nodiscard]]
    BoundingSphere ComputeBoundingSphere(const std::array<hlslpp::float3, 8>& corners);

    //! カスケード1枚の、シャドウマップ1テクセルが覆うワールド幅を求めます。
    //! @param  [in] boundsRadius 平行投影の半径（ワールド距離）
    //! @param  [in] mapSize      シャドウマップの一辺（ピクセル）
    //! @return 1テクセルのワールド幅（= 2 * boundsRadius / mapSize）
    [[nodiscard]]
    constexpr float ComputeTexelWorldSize(float boundsRadius, u32 mapSize) noexcept {
        return 2.0f * boundsRadius / static_cast<float>(mapSize);
    }

    //! 球を覆うディレクショナルライトの ViewProjection 行列を、カスケード1枚分求めます。
    //! @param  [in] lightDir           光の向き（正規化済み）
    //! @param  [in] bounds             投影で覆う球（視錐台の切れ端を包む球）。
    //!                                 範囲はテクセル単位に丸めて置くので、厳密な中心からは最大1テクセルずれる
    //! @param  [in] mapSize            シャドウマップの一辺（ピクセル）
    //! @param  [in] depthTowardLight   球の中心から光の来る側へ収める奥行き
    //! @param  [in] depthAwayFromLight 球の中心から向こう側へ収める奥行き
    //! @return ライト空間の ViewProjection 行列
    [[nodiscard]]
    Tsukino::Core::Math::matrix ComputeLightViewProj(const hlslpp::float3& lightDir, const BoundingSphere& bounds, u32 mapSize,
                                                     float depthTowardLight, float depthAwayFromLight);

    //! 影の調整値を使える範囲へ丸めます。
    //! @param  [in] settings 丸める前の調整値
    //! @return 丸めた調整値（mapSize は2のべき乗、長さは0以上、splitLambda は0〜1）
    [[nodiscard]]
    ShadowSettings SanitizeSettings(const ShadowSettings& settings);

    //! 描画コマンドがカスケードの投影範囲に掛かっているかを判定します。
    //! @param  [in] cmd              判定する描画コマンド
    //! @param  [in] cascade          判定するカスケード
    //! @return true: 描く必要がある（掛かっている、または判定できない）, false: 確実に範囲外
    //! @note   判定できないもの（バウンド不明・インスタンス描画）は必ず true を返す。
    //!         間引きは「確実に範囲外」と言えるときだけ行う
    [[nodiscard]]
    bool OverlapsCascade(const DrawCommand& cmd, const ShadowCascade& cascade);

}    // namespace Tsukino::Renderer::ShadowCascades
