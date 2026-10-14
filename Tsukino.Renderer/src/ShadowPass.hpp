//----------------------------------------------------------------------------
//! @file   ShadowPass.hpp
//! @brief  シャドウマップパスの宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。シャドウマップ（テクスチャ・DSV・SRV・
//!         比較サンプラー）と影用パイプラインを持ち、GBuffer パスの形状を光源から見た深度で
//!         描きます。ディレクショナルライトの ViewProjection 行列の計算もここで行います。
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
    //! シャドウマップパスのクラスです。
    //------------------------------------------------------------------------
    class ShadowPass {
    public:
        //! シャドウマップ1枚の一辺（ピクセル）。画面サイズに依存しないのでリサイズでは作り直さない
        static constexpr u32 kMapSize = 2048;

        //! カスケードの枚数。b0のcascadeViewProjの要素数と同じでなければならない
        static constexpr u32 kCascadeCount = kShadowCascadeCount;

        //! 影を出す最大距離（ワールド。注視点からの距離）。最も外側のカスケードの半径になる。
        //! フォグに沈んで見えなくなるより手前で切るが、画面に映っている物には影が要るので、
        //! ゲームが物を置く・湧かせる範囲を覆える値にしてある。ここを伸ばすぶんはカスケードを増やして補う
        //! （枚数を据え置いて伸ばすと、近景1枚あたりの受け持ちが広がって足元の影が粗くなる）
        static constexpr float kShadowDistance = 1400.0f;

        //! 半径の対数寄せ具合（practical split scheme の λ）。1.0で完全な対数、0.0で等間隔。
        //! 大きいほど近景へ解像度が寄る。
        //! 本来は視錐台を奥行きで分ける距離の式だが、ここでは注視点を中心とした
        //! 同心の箱の半径として流用している（カメラの向きには依らない）
        static constexpr float kSplitLambda = 0.88f;

        //! 半径を求めるときの基準の最小値（式の near に当たる）。
        //! 小さくすると対数項が極端に小さくなり、第1カスケードが実用にならないほど狭くなる
        static constexpr float kSplitNear = 50.0f;

        //! スキンメッシュのバウンドを膨らませる倍率。
        //! MeshDataのAABBはバインドポーズのものなので、アニメで手足がはみ出す
        static constexpr float kSkinnedBoundsInflate = 1.6f;

        //! 平行投影の奥行き。注視点から光の来る側へkDepthTowardLight、向こう側へkDepthAwayFromLightまでを収める。
        //! シェーダーはこの合計（kDepthRange）で深度バイアスをワールド距離から深度値へ換算する。
        //!
        //! 全カスケードで同じ値を使うので、一番広いカスケード（kShadowDistance）が
        //! 斜めから見て収まるだけの奥行きが要る。ここが足りないと、注視点から光の方向へ
        //! 離れた位置にいる物が near 面の手前へ出てしまい、影マップに描かれない
        //! （「遠くの敵だけ影が出ない」という形で現れる）
        static constexpr float kDepthTowardLight   = 2000.0f;
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

        //! GBuffer パスの描画コマンドのうち、影を落とすものをシャドウマップへ描きます。
        //! @param  [in]     commands このフレームの描画コマンド
        //! @param  [in,out] stats    描画統計の加算先
        //! @return true: 描いた（レンダーターゲットとビューポートを書き換えたので呼び出し側で戻すこと）
        //!         false: 影用パイプラインが無いので何もしていない
        bool Execute(const std::vector<DrawCommand>& commands, Renderer::FrameStats& stats);

        //! シャドウマップ（t8）と比較サンプラー（s8）をピクセルシェーダーへバインドします。
        //! @param  [in] context デバイスコンテキスト
        void BindForSampling(ID3D11DeviceContext* context) const;

        //! カスケードごとの平行投影の半径を求めます。
        //! @return 近い順に kCascadeCount 個の半径（注視点から左右・上下へのワールド距離）
        //! @note   practical split scheme（対数分割と等分割の重み付き平均）の式で求める。
        //!         対数だけだと近景へ寄りすぎ、等間隔だけだと近景が粗くなる
        [[nodiscard]]
        static std::array<float, kCascadeCount> ComputeCascadeHalfExtents();

        //! カスケード1枚の、シャドウマップ1テクセルが覆うワールド幅を求めます。
        //! @param  [in] halfExtent 平行投影の半径（ワールド距離）
        //! @return 1テクセルのワールド幅（= 2 * halfExtent / kMapSize）
        [[nodiscard]]
        static constexpr float ComputeTexelWorldSize(float halfExtent) noexcept {
            return 2.0f * halfExtent / static_cast<float>(kMapSize);
        }

        //! ディレクショナルライトの ViewProjection 行列を、カスケード1枚分求めます。
        //! @param  [in] normalizedDir 光の向き（正規化済み）
        //! @param  [in] focusPoint    投影範囲の中心（通常はメインカメラの注視点）。
        //!                            範囲はテクセル単位に丸めて置くので、厳密な中心からは最大1テクセルずれる
        //! @param  [in] halfExtent    平行投影の半径（ワールド距離）
        //! @return ライト空間の ViewProjection 行列
        [[nodiscard]]
        static Tsukino::Core::Math::matrix ComputeLightViewProj(const hlslpp::float3& normalizedDir, const hlslpp::float3& focusPoint,
                                                                float halfExtent);

        //! シャドウパスでカスケードの範囲外にあるオブジェクトを間引くかを切り替えます。
        //! @note 既定は有効。同じビルドで有効・無効の両方を計測できるようにするための口で、
        //!       見た目は変わらない前提（変わるなら間引きすぎている）
        void SetCullingEnabled(bool enabled) { m_cullingEnabled = enabled; }

    private:

        //! 描画コマンドがカスケードの投影範囲に掛かっているかを判定します。
        //! @param  [in] cmd              判定する描画コマンド
        //! @param  [in] cascadeViewProj  そのカスケードのライト空間 ViewProjection
        //! @param  [in] halfExtent       そのカスケードの平行投影の半径
        //! @return true: 描く必要がある（掛かっている、または判定できない）, false: 確実に範囲外
        //! @note   判定できないもの（バウンド不明・インスタンス描画）は必ず true を返す。
        //!         間引きは「確実に範囲外」と言えるときだけ行う
        [[nodiscard]]
        static bool OverlapsCascade(const DrawCommand& cmd, const Tsukino::Core::Math::matrix& cascadeViewProj, float halfExtent);

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

        bool m_cullingEnabled = true;    // カスケードの範囲外を間引くか（計測用に切り替えられる）

        std::shared_ptr<PipelineState> m_staticPipeline;      // スタティックメッシュ用シャドウパイプライン
        std::shared_ptr<PipelineState> m_skeletalPipeline;    // スケルタルメッシュ用シャドウパイプライン
    };
}    // namespace Tsukino::Renderer
