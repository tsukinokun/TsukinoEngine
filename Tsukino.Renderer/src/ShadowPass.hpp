//----------------------------------------------------------------------------
//! @file   ShadowPass.hpp
//! @brief  シャドウマップパスの宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。シャドウマップ（テクスチャ・DSV・SRV・
//!         比較サンプラー）と影用パイプラインを持ち、GBuffer パスの形状を光源から見た深度で
//!         描きます。カスケード（カメラの視錐台を奥行きで切った1切れごとのシャドウマップ）の
//!         ライト空間 ViewProjection 行列の計算もここで行います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/Renderer.hpp>

#include <array>
#include <memory>
#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class DrawCommandExecutor;    // 前方宣言

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
    //! シャドウマップパスのクラスです。
    //------------------------------------------------------------------------
    class ShadowPass {
    public:
        //! シャドウマップ1枚の一辺（ピクセル）。画面サイズに依存しないのでリサイズでは作り直さない
        static constexpr u32 kMapSize = 2048;

        //! カスケードの枚数。b0のcascadeViewProjの要素数と同じでなければならない
        static constexpr u32 kCascadeCount = kShadowCascadeCount;

        //! 影を出す最大の奥行き（カメラのビュー空間 z）。最も外側のカスケードの奥側の分割距離になる。
        //! カメラの farZ がこれより手前なら farZ で切る。
        //! フォグに沈んで見えなくなるより手前で切るが、画面に映っている物には影が要るので、
        //! ゲームが物を置く・湧かせる範囲を覆える値にしてある
        static constexpr float kShadowDistance = 1400.0f;

        //! 分割距離の対数寄せ具合（practical split scheme の λ）。1.0で完全な対数、0.0で等間隔。
        //! 大きいほど近景へ解像度が寄る
        static constexpr float kSplitLambda = 0.88f;

        //! スキンメッシュのバウンドを膨らませる倍率。
        //! MeshDataのAABBはバインドポーズのものなので、アニメで手足がはみ出す
        static constexpr float kSkinnedBoundsInflate = 1.6f;

        //! 平行投影の奥行き。カスケードを包む球の中心から、光の来る側へkDepthTowardLight、
        //! 向こう側へkDepthAwayFromLightまでを収める。
        //! シェーダーはこの合計（kDepthRange）で深度バイアスをワールド距離から深度値へ換算する。
        //!
        //! 全カスケードで同じ値を使うので、一番大きい球（最も外側のカスケード）の半径より
        //! 十分に長くする。向こう側は球が収まれば足りるが、光の来る側は球の外にある
        //! 投影者（高い木や崖）も収める必要がある。ここが足りないと、それらが near 面の
        //! 手前へ出てしまい、影マップに描かれない
        static constexpr float kDepthTowardLight   = 3000.0f;
        static constexpr float kDepthAwayFromLight = 3000.0f;
        static constexpr float kDepthRange         = kDepthTowardLight + kDepthAwayFromLight;

        //! シャドウマップと影用パイプラインを作成します。
        //! @param  [in] graphicsContext  デバイスとデバイスコンテキストの取得元
        //! @param  [in] resources        PipelineFactory の取得元
        //! @param  [in] frameConstants   b0 の転送先
        //! @param  [in] commandExecutor  描画コマンドの実行
        //! @param  [in] staticVS         スタティックメッシュ用のシャドウ頂点シェーダー
        //! @param  [in] skeletalVS       スケルタルメッシュ用のシャドウ頂点シェーダー
        //! @param  [in] ps               シャドウ用ピクセルシェーダー
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool Initialize(GraphicsContext&                   graphicsContext,
                        RenderResources&                   resources,
                        FrameConstants&                    frameConstants,
                        DrawCommandExecutor&               commandExecutor,
                        const Tsukino::Asset::ShaderAsset* staticVS,
                        const Tsukino::Asset::ShaderAsset* skeletalVS,
                        const Tsukino::Asset::ShaderAsset* ps);

        //! このフレームのカスケードを求めて、ワールドのシーン定数（b0）へ書き込みます。
        //! @param  [in] worldScene ワールドのシーン定数。view / invViewProj（メインカメラ）と lightDir を使う
        //! @note   カメラとライトが確定した後、Execute() とライティングより前に毎フレーム呼ぶこと。
        //!         ライトの向きが設定されていない（長さ0）ときは何もしない
        void UpdateCascades(const CBufferScene& worldScene);

        //! GBuffer パスの描画コマンドのうち、影を落とすものをシャドウマップへ描きます。
        //! @param  [in]     commands このフレームの描画コマンド
        //! @param  [in,out] stats    描画統計の加算先
        //! @return true: 描いた（レンダーターゲットとビューポートを書き換えたので呼び出し側で戻すこと）
        //!         false: 影用パイプラインが無いので何もしていない
        //! @note   先に UpdateCascades() を呼んでおくこと
        bool Execute(const std::vector<DrawCommand>& commands, Renderer::FrameStats& stats);

        //! シャドウマップ（t8）と比較サンプラー（s8）をピクセルシェーダーへバインドします。
        //! @param  [in] context デバイスコンテキスト
        void BindForSampling(ID3D11DeviceContext* context) const;

        //! カメラの視錐台をカスケードに切り分け、1枚ずつのライト空間の投影を求めます。
        //! @param  [in] cameraView        カメラの View 行列
        //! @param  [in] cameraInvViewProj カメラの ViewProjection の逆行列
        //! @param  [in] lightDir          光の向き（正規化済み）
        //! @return 近い順に kCascadeCount 枚のカスケード
        //! @note   次の関数を順に呼ぶだけの関数。手順ごとの中身はそれぞれの関数を見る
        //!         ComputeFrustumCorners → ComputeSplitDepths → ComputeSliceCorners → ComputeBoundingSphere → ComputeLightViewProj
        [[nodiscard]]
        static std::array<ShadowCascade, kCascadeCount> ComputeCascades(const Tsukino::Core::Math::matrix& cameraView,
                                                                        const Tsukino::Core::Math::matrix& cameraInvViewProj,
                                                                        const hlslpp::float3&              lightDir);

        //! カメラの視錐台の8頂点を、NDC の立方体の頂点を逆射影して求めます。
        //! @param  [in] cameraView        カメラの View 行列（頂点の奥行きを求めるのに使う）
        //! @param  [in] cameraInvViewProj カメラの ViewProjection の逆行列
        //! @return ワールド空間の8頂点と、near 面・far 面の奥行き
        //! @note   リバースZなので NDC の z=1 が near 面、z=0 が far 面になる
        [[nodiscard]]
        static FrustumCorners ComputeFrustumCorners(const Tsukino::Core::Math::matrix& cameraView, const Tsukino::Core::Math::matrix& cameraInvViewProj);

        //! 視錐台を奥行き方向に切る距離を求めます。
        //! @param  [in] nearDepth カメラの near 面の奥行き
        //! @param  [in] farDepth  カメラの far 面の奥行き
        //! @return 近い順に kCascadeCount + 1 個の奥行き。i 番目と i+1 番目の間がカスケード i の受け持ち。
        //!         先頭は nearDepth、末尾は kShadowDistance と farDepth の近いほう
        //! @note   practical split scheme（対数分割と等分割の重み付き平均。kSplitLambda が対数側の重み）で求める
        [[nodiscard]]
        static std::array<float, kCascadeCount + 1> ComputeSplitDepths(float nearDepth, float farDepth);

        //! 視錐台を2つの奥行きで切った切れ端の8頂点を求めます。
        //! @param  [in] frustum   カメラの視錐台
        //! @param  [in] splitNear 切れ端の手前の奥行き
        //! @param  [in] splitFar  切れ端の奥の奥行き
        //! @return 切れ端の8頂点（ワールド。先頭4つが手前、後ろ4つが奥）
        [[nodiscard]]
        static std::array<hlslpp::float3, 8> ComputeSliceCorners(const FrustumCorners& frustum, float splitNear, float splitFar);

        //! 点の集まりを包む球を求めます。
        //! @param  [in] corners 包む点（視錐台の切れ端の8頂点）
        //! @return 重心を中心とし、最も遠い点までを半径とした球
        //! @note   半径は切り上げて整える（カメラを回したときの浮動小数点の揺れで、毎フレーム大きさが変わらないように）
        [[nodiscard]]
        static BoundingSphere ComputeBoundingSphere(const std::array<hlslpp::float3, 8>& corners);

        //! カスケード1枚の、シャドウマップ1テクセルが覆うワールド幅を求めます。
        //! @param  [in] boundsRadius 平行投影の半径（ワールド距離）
        //! @return 1テクセルのワールド幅（= 2 * boundsRadius / kMapSize）
        [[nodiscard]]
        static constexpr float ComputeTexelWorldSize(float boundsRadius) noexcept {
            return 2.0f * boundsRadius / static_cast<float>(kMapSize);
        }

        //! 球を覆うディレクショナルライトの ViewProjection 行列を、カスケード1枚分求めます。
        //! @param  [in] lightDir 光の向き（正規化済み）
        //! @param  [in] bounds   投影で覆う球（視錐台の切れ端を包む球）。
        //!                       範囲はテクセル単位に丸めて置くので、厳密な中心からは最大1テクセルずれる
        //! @return ライト空間の ViewProjection 行列
        [[nodiscard]]
        static Tsukino::Core::Math::matrix ComputeLightViewProj(const hlslpp::float3& lightDir, const BoundingSphere& bounds);

        //! シャドウパスでカスケードの範囲外にあるオブジェクトを間引くかを切り替えます。
        //! @note 既定は有効。同じビルドで有効・無効の両方を計測できるようにするための口で、
        //!       見た目は変わらない前提（変わるなら間引きすぎている）
        void SetCullingEnabled(bool enabled) { m_cullingEnabled = enabled; }

    private:

        //! 描画コマンドがカスケードの投影範囲に掛かっているかを判定します。
        //! @param  [in] cmd              判定する描画コマンド
        //! @param  [in] cascade          判定するカスケード
        //! @return true: 描く必要がある（掛かっている、または判定できない）, false: 確実に範囲外
        //! @note   判定できないもの（バウンド不明・インスタンス描画）は必ず true を返す。
        //!         間引きは「確実に範囲外」と言えるときだけ行う
        [[nodiscard]]
        static bool OverlapsCascade(const DrawCommand& cmd, const ShadowCascade& cascade);

        //! シャドウマップ（テクスチャ・DSV・SRV・比較サンプラー）を作成します。
        //! @param  [in] device 描画デバイス
        //! @return true: 作成成功, false: 作成失敗
        [[nodiscard]]
        bool CreateShadowMap(ID3D11Device* device);

    private:
        GraphicsContext*     m_graphicsContext = nullptr;    // デバイスコンテキストの取得元（借りている）
        FrameConstants*      m_frameConstants  = nullptr;    // b0 の転送先（借りている）
        DrawCommandExecutor* m_commandExecutor = nullptr;    // 描画コマンドの実行（借りている）

        ComPtr<ID3D11Texture2D>          m_mapTex;     // シャドウマップテクスチャ（kCascadeCount枚の配列）
        ComPtr<ID3D11ShaderResourceView> m_mapSRV;     // 配列全体の SRV（PS でのサンプリング用。t8）
        ComPtr<ID3D11SamplerState>       m_sampler;    // PCF 用比較サンプラー

        // スライスごとの DSV。書き込みは1枚ずつなので、配列要素を個別に指すビューが要る
        std::array<ComPtr<ID3D11DepthStencilView>, kCascadeCount> m_mapDSV;

        // このフレームのカスケード（UpdateCascades() が埋め、Execute() が使う）
        std::array<ShadowCascade, kCascadeCount> m_cascades{};

        bool m_cullingEnabled = true;    // カスケードの範囲外を間引くか（計測用に切り替えられる）

        std::shared_ptr<PipelineState> m_staticPipeline;      // スタティックメッシュ用シャドウパイプライン
        std::shared_ptr<PipelineState> m_skeletalPipeline;    // スケルタルメッシュ用シャドウパイプライン
    };
}    // namespace Tsukino::Renderer
