//----------------------------------------------------------------------------
//! @file   GpuProfiler.hpp
//! @brief  パス単位のGPUタイマーの宣言
//! @detail Tsukino.Renderer の内部専用ヘッダです。D3D11 のタイムスタンプクエリで各パスを
//!         囲み、フレーム全体を TIMESTAMP_DISJOINT で囲みます。クエリは3フレーム分の
//!         リングバッファで持ち、2フレーム以上前の結果だけを待たずに読み出します。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Renderer/GpuTiming.hpp>

#include <d3d11.h>
#include <wrl/client.h>

#include <array>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    //------------------------------------------------------------------------
    //! パス単位のGPU時間を計測するクラスです。
    //! @note  Initialize に失敗した場合はすべての呼び出しが何もしない。
    //!        計測が描画を止めないよう、結果の読み出しは D3D11_ASYNC_GETDATA_DONOTFLUSH で
    //!        行い、まだ届いていなければそのフレームの結果は捨てる
    //------------------------------------------------------------------------
    class GpuProfiler {
    public:
        //------------------------------------------------------------------------
        //! クエリを作成します。
        //! @param  [in] device  クエリを作るデバイス
        //! @param  [in] context クエリを発行するデバイスコンテキスト
        //! @return true: 作成成功, false: 作成失敗（計測は無効になる）
        //------------------------------------------------------------------------
        [[nodiscard]]
        bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);

        //------------------------------------------------------------------------
        //! 届いている結果を読み出し、このフレームの計測を始めます。
        //! @note  Render() の先頭で1回だけ呼ぶ
        //------------------------------------------------------------------------
        void BeginFrame();

        //------------------------------------------------------------------------
        //! パスの開始時刻を記録します。
        //! @param  [in] pass 計測するパス
        //------------------------------------------------------------------------
        void BeginPass(GpuPass pass);

        //------------------------------------------------------------------------
        //! パスの終了時刻を記録します。
        //! @param  [in] pass 計測するパス（BeginPass と同じもの）
        //------------------------------------------------------------------------
        void EndPass(GpuPass pass);

        //------------------------------------------------------------------------
        //! このフレームの計測を終えます。
        //! @note  Present の前に呼ぶ
        //------------------------------------------------------------------------
        void EndFrame();

        //------------------------------------------------------------------------
        //! 最後に読み出せたフレームのGPU時間を返します。
        //! @return パスごとのGPU時間
        //------------------------------------------------------------------------
        [[nodiscard]]
        const GpuFrameTimings& GetLatest() const noexcept {
            return m_latest;
        }

    private:
        //! リングバッファの段数。2フレーム後に読むので、書き込み中の1段と合わせて3段
        static constexpr u32 kFrameLatency = 3;

        //------------------------------------------------------------------------
        //! 1フレーム分のクエリです。
        //------------------------------------------------------------------------
        struct FrameQueries {
            Microsoft::WRL::ComPtr<ID3D11Query> disjoint;      //!< フレーム全体の周波数と有効性
            Microsoft::WRL::ComPtr<ID3D11Query> frameBegin;    //!< フレームの開始時刻
            Microsoft::WRL::ComPtr<ID3D11Query> frameEnd;      //!< フレームの終了時刻
            std::array<Microsoft::WRL::ComPtr<ID3D11Query>, kGpuPassCount> passBegin;    //!< パスの開始時刻
            std::array<Microsoft::WRL::ComPtr<ID3D11Query>, kGpuPassCount> passEnd;      //!< パスの終了時刻
            std::array<bool, kGpuPassCount> passIssued{};    //!< このフレームでパスを計測したか
            bool pending = false;    //!< 発行済みで、まだ結果を読んでいないか
        };

        //------------------------------------------------------------------------
        //! 1フレーム分の結果を読み出します。
        //! @param  [in,out] frame 読み出すクエリ。読めたか捨てたら pending を下ろす
        //! @return true: 読み出せた, false: まだ届いていない
        //------------------------------------------------------------------------
        bool TryResolve(FrameQueries& frame);

        ID3D11DeviceContext* m_context = nullptr;    // クエリを発行するデバイスコンテキスト（借り物）

        std::array<FrameQueries, kFrameLatency> m_frames;    // クエリのリングバッファ
        u64             m_frameIndex = 0;        // 何フレーム目か（リングバッファの添字に使う）
        bool            m_enabled    = false;    // クエリの作成に成功したか
        bool            m_inFrame    = false;    // BeginFrame と EndFrame の間か
        GpuFrameTimings m_latest;                // 最後に読み出せた結果
    };
}    // namespace Tsukino::Renderer
