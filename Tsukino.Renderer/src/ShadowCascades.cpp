//----------------------------------------------------------------------------
//! @file   ShadowCascades.cpp
//! @brief  カスケードシャドウの投影の計算の実装
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/ShadowCascades.hpp>

#include <Tsukino/Renderer/MeshBuffer.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! 影の調整値を使える範囲へ丸めます。
    //------------------------------------------------------------------------
    ShadowSettings ShadowCascades::SanitizeSettings(const ShadowSettings& settings) {
        ShadowSettings sanitized = settings;

        // 2のべき乗に揃える（半端な解像度はテクセル幅の丸めと相性が悪く、GPU にも素直でない）
        const u32 clampedSize = std::clamp(settings.mapSize, ShadowSettings::kMinMapSize, ShadowSettings::kMaxMapSize);
        sanitized.mapSize     = std::bit_floor(clampedSize);

        //--------------------------------------------------------------------
        // 長さの下限。distance が0だとカスケードの球の半径も0になり、
        // テクセル幅（半径 / 解像度）で割る所が0除算になるので、ごく小さな正の値で止める
        //--------------------------------------------------------------------
        constexpr float kMinDistance = 0.01f;
        sanitized.distance     = std::max(settings.distance, kMinDistance);
        sanitized.splitStart   = std::max(settings.splitStart, 0.0f);
        sanitized.splitLambda  = std::clamp(settings.splitLambda, 0.0f, 1.0f);
        sanitized.casterMargin = std::max(settings.casterMargin, 0.0f);

        return sanitized;
    }

    //------------------------------------------------------------------------
    //! 描画コマンドがカスケードの投影範囲に掛かっているかを判定します。
    //------------------------------------------------------------------------
    bool ShadowCascades::OverlapsCascade(const DrawCommand& cmd, const ShadowCascade& cascade) {
        if(!cmd.mesh)
            return false;

        //--------------------------------------------------------------------
        // 判定しないケース。
        //
        // バウンド半径0はMeshDataがboundsを持っていない経路（プリミティブ等）。
        // インスタンス描画はメッシュ単体のバウンドが全インスタンスを覆わない
        // （草がその例。今はcastsShadow=falseだが、ここでも明示的に守る）。
        // どちらも「分からないものは消さない」側へ倒す
        //--------------------------------------------------------------------
        if(cmd.mesh->boundsRadius <= 0.0f || cmd.instanceCount > 1)
            return true;

        //--------------------------------------------------------------------
        // ローカルのバウンド球をワールドへ移す。
        // 半径はワールド行列の最大軸スケールで伸ばす（非一様スケールでも
        // 覆い漏らさない側に倒す）
        //--------------------------------------------------------------------
        const hlslpp::float4 centerWorld = hlslpp::mul(hlslpp::float4(cmd.mesh->boundsCenter, 1.0f), cmd.transform);

        const auto axisLength = [&cmd](int row) {
            const hlslpp::float4 axis = cmd.transform[row];
            return std::sqrt(static_cast<float>(axis.x) * static_cast<float>(axis.x) + static_cast<float>(axis.y) * static_cast<float>(axis.y)
                             + static_cast<float>(axis.z) * static_cast<float>(axis.z));
        };
        const float scale = std::max({axisLength(0), axisLength(1), axisLength(2)});

        float radiusWorld = cmd.mesh->boundsRadius * scale;

        // スキンメッシュはバインドポーズのAABBなので、アニメではみ出す分を見込む
        if(cmd.boneMatrices && cmd.boneCount > 0)
            radiusWorld *= kSkinnedBoundsInflate;

        //--------------------------------------------------------------------
        // カスケードのクリップ空間（平行投影なのでw=1、xyは-1〜1）で判定する。
        // ワールドの半径は平行投影の半径（＝カスケードを包む球の半径）で割ると
        // クリップ空間の半径になる。
        // 奥行き（z）は見ない。全カスケードで共通の広い範囲を取ってあり、
        // ここで切ると「範囲外の高い所にある投影者」を落としてしまうため
        //--------------------------------------------------------------------
        const hlslpp::float4 clip = hlslpp::mul(centerWorld, cascade.lightViewProj);

        const float boundsRadius = cascade.bounds.radius;
        const float clipRadius   = (boundsRadius > 0.0f) ? (radiusWorld / boundsRadius) : 0.0f;
        const float x          = static_cast<float>(clip.x);
        const float y          = static_cast<float>(clip.y);

        return (x + clipRadius >= -1.0f) && (x - clipRadius <= 1.0f) && (y + clipRadius >= -1.0f) && (y - clipRadius <= 1.0f);
    }

    //------------------------------------------------------------------------
    //! カメラの視錐台をカスケードに切り分け、1枚ずつのライト空間の投影を求めます。
    //------------------------------------------------------------------------
    ShadowCascadeSet ShadowCascades::ComputeCascades(const Tsukino::Core::Math::matrix& cameraView,
                                                 const Tsukino::Core::Math::matrix& cameraInvViewProj,
                                                 const hlslpp::float3&              lightDir,
                                                 const ShadowSettings&              settings,
                                                 u32                                activeCount) {
        ShadowCascadeSet result{};
        std::array<ShadowCascade, kCascadeCount>& cascades = result.cascades;

        // 手順0: カメラの視錐台をワールド空間の8頂点として取り出す
        const FrustumCorners frustum = ComputeFrustumCorners(cameraView, cameraInvViewProj);

        // カメラ行列がまだ設定されていない（最初のフレームなど）と奥行きが求まらない。
        // NaN でもここで弾けるよう、否定の形で比べる
        if(!(frustum.farDepth > frustum.nearDepth))
            return result;

        // 手順1: 視錐台を奥行き方向に切る距離を決める
        const std::array<float, kCascadeCount + 1> splitDepths = ComputeSplitDepths(frustum.nearDepth, frustum.farDepth, settings, activeCount);

        float maxRadius = 0.0f;
        for(u32 i = 0; i < activeCount; ++i) {
            ShadowCascade& cascade = cascades[i];
            cascade.splitNear      = splitDepths[i];
            cascade.splitFar       = splitDepths[i + 1];

            // 手順2: 切れ端の8頂点を求め、それを包む球を作る
            const std::array<hlslpp::float3, 8> sliceCorners = ComputeSliceCorners(frustum, cascade.splitNear, cascade.splitFar);
            cascade.bounds                                   = ComputeBoundingSphere(sliceCorners);

            // 手順4で中心をテクセル幅へ丸めると、投影範囲が最大1テクセルずれて球の端がはみ出す。
            // 1テクセル分の余白を足しておく（半径は切り上げ済みの整数から決まるので、揺れない）
            cascade.bounds.radius += ComputeTexelWorldSize(cascade.bounds.radius, settings.mapSize);
            cascade.texelWorldSize = ComputeTexelWorldSize(cascade.bounds.radius, settings.mapSize);

            maxRadius = std::max(maxRadius, cascade.bounds.radius);
        }

        //--------------------------------------------------------------------
        // 使わないカスケードは最後の有効なカスケードの複製にする。
        // u_scene のレイアウトとシェーダーは常に kCascadeCount 枚ぶんを読むが、
        // 分割距離が揃うので有効な枚数より奥の番号は選ばれない
        //--------------------------------------------------------------------
        for(u32 i = activeCount; i < kCascadeCount; ++i)
            cascades[i] = cascades[activeCount - 1];

        //--------------------------------------------------------------------
        // 手順3: 平行投影の奥行きを決める。
        // 全カスケードで同じ値を使うので、一番大きい球が収まる長さにする。
        // 向こう側は球が収まれば足りるが、光の来る側は球の外にある投影者（高い木や崖）も
        // 収める必要があるので casterMargin を足す。ここが足りないと、それらが near 面の
        // 手前へ出てしまい、影マップに描かれない
        //--------------------------------------------------------------------
        result.depthAwayFromLight = maxRadius;
        result.depthTowardLight   = maxRadius + settings.casterMargin;

        // 手順4: 球を覆う平行投影をライト空間で作る
        for(ShadowCascade& cascade : cascades)
            cascade.lightViewProj = ComputeLightViewProj(lightDir, cascade.bounds, settings.mapSize, result.depthTowardLight, result.depthAwayFromLight);

        return result;
    }

    //------------------------------------------------------------------------
    //! カメラの視錐台の8頂点を、NDC の立方体の頂点を逆射影して求めます。
    //------------------------------------------------------------------------
    FrustumCorners ShadowCascades::ComputeFrustumCorners(const Tsukino::Core::Math::matrix& cameraView,
                                                     const Tsukino::Core::Math::matrix& cameraInvViewProj) {
        //--------------------------------------------------------------------
        // 画面の四隅（NDC の xy）。near 面と far 面で同じ並びにしておくと、
        // nearPlane[i] と farPlane[i] がカメラから伸びる同じ稜線の両端になる
        //--------------------------------------------------------------------
        constexpr std::array<std::array<float, 2>, 4> kScreenCorners = {{
            {{-1.0f, -1.0f}},    // 左下
            {{+1.0f, -1.0f}},    // 右下
            {{+1.0f, +1.0f}},    // 右上
            {{-1.0f, +1.0f}},    // 左上
        }};

        // リバースZのため、NDC の z は near 面が1、far 面が0
        constexpr float kNearNdcZ = 1.0f;
        constexpr float kFarNdcZ  = 0.0f;

        // NDC の点をワールドへ戻す。透視投影ではwが1でないので、最後にwで割る
        const auto unproject = [&cameraInvViewProj](const std::array<float, 2>& screen, float ndcZ) {
            const hlslpp::float4 world = hlslpp::mul(hlslpp::float4(screen[0], screen[1], ndcZ, 1.0f), cameraInvViewProj);
            return hlslpp::float3(world.xyz) / hlslpp::float1(world.w);
        };

        FrustumCorners frustum{};
        for(u32 i = 0; i < 4; ++i) {
            frustum.nearPlane[i] = unproject(kScreenCorners[i], kNearNdcZ);
            frustum.farPlane[i]  = unproject(kScreenCorners[i], kFarNdcZ);
        }

        //--------------------------------------------------------------------
        // 奥行きは View 行列でビュー空間へ移した z。
        // near 面・far 面はビュー空間の z が一定の平面なので、どの頂点で測っても同じ
        //--------------------------------------------------------------------
        const auto viewDepth = [&cameraView](const hlslpp::float3& world) {
            return static_cast<float>(hlslpp::mul(hlslpp::float4(world, 1.0f), cameraView).z);
        };
        frustum.nearDepth = viewDepth(frustum.nearPlane[0]);
        frustum.farDepth  = viewDepth(frustum.farPlane[0]);

        return frustum;
    }

    //------------------------------------------------------------------------
    //! 視錐台を奥行き方向に切る距離を求めます。
    //------------------------------------------------------------------------
    std::array<float, ShadowCascades::kCascadeCount + 1> ShadowCascades::ComputeSplitDepths(float nearDepth, float farDepth, const ShadowSettings& settings, u32 activeCount) {
        std::array<float, kCascadeCount + 1> splitDepths{};

        // 影を出すのはカメラの far 面と settings.distance の近いほうまで
        const float shadowFar = std::min(settings.distance, farDepth);

        //--------------------------------------------------------------------
        // 式の起点。カメラの near から始めると、TPSカメラでは何も写っていない
        // 手前の空間にカスケードを使い切ってしまう（ShadowSettings::splitStart参照）。
        // 起点より手前はカスケード0がまとめて受け持つ（下の splitDepths.front()）
        //--------------------------------------------------------------------
        // clampにしないのは、near が settings.distance より奥のカメラで lo > hi になるため
        const float splitStart = std::min(std::max(settings.splitStart, nearDepth), shadowFar);

        // 対数分割は起点が0だと計算できない（正射影のカメラで起こりうる）ので、下限を設ける
        const float logNear = std::max(splitStart, 1.0e-3f);

        for(u32 i = 0; i <= activeCount; ++i) {
            const float ratio = static_cast<float>(i) / static_cast<float>(activeCount);

            //----------------------------------------------------------------
            // 対数分割は、カメラから見た1テクセルの大きさがどの奥行きでも同じに
            // なる理想的な分け方だが、そのままだと第1カスケードが極端に薄くなる。
            // 等分割はその逆で近景が粗い。両者の重み付き平均を取るのが
            // practical split scheme（splitLambdaが対数側の重み）
            //----------------------------------------------------------------
            const float logSplit     = logNear * std::pow(shadowFar / logNear, ratio);
            const float uniformSplit = splitStart + (shadowFar - splitStart) * ratio;

            splitDepths[i] = settings.splitLambda * logSplit + (1.0f - settings.splitLambda) * uniformSplit;
        }

        // 両端は式の丸め誤差を残さず、ちょうど near と shadowFar にする。
        // 先頭を splitStart ではなく near にするので、カスケード0は起点より手前も受け持つ
        // 使わないカスケードの分も shadowFar で埋める（シェーダーは常に kCascadeCount 個の境目を読む）
        splitDepths.front() = nearDepth;
        for(u32 i = activeCount; i <= kCascadeCount; ++i)
            splitDepths[i] = shadowFar;

        return splitDepths;
    }

    //------------------------------------------------------------------------
    //! 視錐台を2つの奥行きで切った切れ端の8頂点を求めます。
    //------------------------------------------------------------------------
    std::array<hlslpp::float3, 8> ShadowCascades::ComputeSliceCorners(const FrustumCorners& frustum, float splitNear, float splitFar) {
        //--------------------------------------------------------------------
        // 視錐台の稜線（near 面の頂点から far 面の頂点へ向かう辺）の上では、
        // ビュー空間の z は位置に比例して変わる。だから「奥行き splitNear の点」は
        // 稜線を (splitNear - nearDepth) / (farDepth - nearDepth) の割合で
        // 線形補間すれば、ちょうど求まる
        //--------------------------------------------------------------------
        const float depthRange = frustum.farDepth - frustum.nearDepth;
        const float nearRatio  = (splitNear - frustum.nearDepth) / depthRange;
        const float farRatio   = (splitFar - frustum.nearDepth) / depthRange;

        std::array<hlslpp::float3, 8> sliceCorners{};
        for(u32 i = 0; i < 4; ++i) {
            const hlslpp::float3 edge = frustum.farPlane[i] - frustum.nearPlane[i];

            sliceCorners[i]     = frustum.nearPlane[i] + edge * hlslpp::float1(nearRatio);    // 手前の4頂点
            sliceCorners[i + 4] = frustum.nearPlane[i] + edge * hlslpp::float1(farRatio);     // 奥の4頂点
        }

        return sliceCorners;
    }

    //------------------------------------------------------------------------
    //! 点の集まりを包む球を求めます。
    //------------------------------------------------------------------------
    BoundingSphere ShadowCascades::ComputeBoundingSphere(const std::array<hlslpp::float3, 8>& corners) {
        //--------------------------------------------------------------------
        // 箱（AABB）ではなく球で包む理由。
        // 切れ端をライト空間の箱でぴったり包むと、カメラを回すたびに箱の大きさが
        // 変わり、1テクセルの大きさも変わって影の縁がちらつく。
        // 球の大きさはカメラの向きに依らないので、カメラを回しても投影範囲は
        // 平行移動するだけになり、テクセル幅に丸める手順3と組み合わせると影が止まって見える。
        // 球のほうが隙間が多く解像度は少し損をするが、揺れないことを優先する
        //--------------------------------------------------------------------
        hlslpp::float3 center = hlslpp::float3(0.0f, 0.0f, 0.0f);
        for(const hlslpp::float3& corner : corners)
            center += corner;
        // 複合代入（center /= float1）にしない。float1 が3成分へ広がらずxだけを割り、
        // yzを0で割って中心がinfになる（半径もinfになり、影が1つも描かれなかった）。
        // 二項の演算子なら広がるので、こちらで書く
        center = center / hlslpp::float1(static_cast<float>(corners.size()));

        float radius = 0.0f;
        for(const hlslpp::float3& corner : corners)
            radius = std::max(radius, static_cast<float>(hlslpp::length(corner - center)));

        // 数学的には向きに依らない半径も、浮動小数点では毎フレーム末尾の桁が揺れる。
        // 揺れるとテクセル幅も揺れるので、切り上げて値を固定する
        radius = std::ceil(radius);

        return BoundingSphere{center, radius};
    }

    //------------------------------------------------------------------------
    //! 球を覆うディレクショナルライトの ViewProjection 行列を、カスケード1枚分求めます。
    //------------------------------------------------------------------------
    Tsukino::Core::Math::matrix ShadowCascades::ComputeLightViewProj(const hlslpp::float3& lightDir, const BoundingSphere& bounds, u32 mapSize,
                                                                 float depthTowardLight, float depthAwayFromLight) {
        //--------------------------------------------------------------------
        // ディレクショナルライトは平行投影を使う。
        // 投影範囲は、視錐台の切れ端を包む球を、光の向きから見てちょうど覆う正方形
        //--------------------------------------------------------------------
        hlslpp::float3 up = hlslpp::float3(0.0f, 1.0f, 0.0f);

        // ライト方向が真上/真下に近いときupベクトルが平行になるので回避
        float dotUp = std::abs(hlslpp::dot(lightDir, up));
        if(dotUp > 0.99f) {
            up = hlslpp::float3(0.0f, 0.0f, 1.0f);
        }

        //--------------------------------------------------------------------
        // View行列はワールド原点から光の向きへ見るものに固定し、球の中心ではなく
        // 投影範囲（off-center）の側を動かす。
        //
        // 球の中心に合わせてView行列ごと動かすと、中心が1テクセル未満動くたびに
        // シャドウマップ上の格子もずれ、影の縁がカメラの移動に合わせてちらつく
        // （プレイヤー追従のTPSでは常に動いている）。原点基準のView空間で
        // 中心の位置をテクセル幅の整数倍へ丸めてから範囲を置けば、格子は
        // ワールドに対して動かず、影の縁は止まって見える
        //--------------------------------------------------------------------
        const hlslpp::float3        origin    = hlslpp::float3(0.0f, 0.0f, 0.0f);
        Tsukino::Core::Math::matrix lightView = Tsukino::Core::Math::matrix::lookAtLH(origin, lightDir, up);

        const hlslpp::float4 centerInLight = hlslpp::mul(hlslpp::float4(bounds.center, 1.0f), lightView);

        const float radius         = bounds.radius;
        const float texelWorldSize = ComputeTexelWorldSize(radius, mapSize);
        const float centerX        = std::floor(static_cast<float>(centerInLight.x) / texelWorldSize) * texelWorldSize;
        const float centerY        = std::floor(static_cast<float>(centerInLight.y) / texelWorldSize) * texelWorldSize;
        const float centerDepth    = static_cast<float>(centerInLight.z);

        // リバースZのため far → near の順に渡す
        Tsukino::Core::Math::matrix lightProj = Tsukino::Core::Math::matrix::orthographicOffCenterLH(centerX - radius,                     // left
                                                                                                     centerX + radius,                     // right
                                                                                                     centerY - radius,                     // bottom
                                                                                                     centerY + radius,                     // top
                                                                                                     centerDepth + depthAwayFromLight,    // far
                                                                                                     centerDepth - depthTowardLight       // near
        );

        return hlslpp::mul(lightView, lightProj);
    }
}    // namespace Tsukino::Renderer
