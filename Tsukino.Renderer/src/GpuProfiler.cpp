//----------------------------------------------------------------------------
//! @file   GpuProfiler.cpp
//! @brief  パス単位のGPUタイマーの実装
//----------------------------------------------------------------------------
#include "GpuProfiler.hpp"

#include <Tsukino/Core/Log.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    namespace {
        //------------------------------------------------------------------------
        //! クエリを1つ作成します。
        //! @param  [in]  device 作成に使うデバイス
        //! @param  [in]  type   クエリの種類
        //! @param  [out] query  作成したクエリ
        //! @return true: 作成成功, false: 作成失敗
        //------------------------------------------------------------------------
        bool CreateQuery(ID3D11Device* device, D3D11_QUERY type, Microsoft::WRL::ComPtr<ID3D11Query>& query) {
            D3D11_QUERY_DESC desc{};
            desc.Query     = type;
            desc.MiscFlags = 0;
            return SUCCEEDED(device->CreateQuery(&desc, query.ReleaseAndGetAddressOf()));
        }

        //------------------------------------------------------------------------
        //! タイムスタンプを待たずに読み出します。
        //! @param  [in]  context 読み出しに使うデバイスコンテキスト
        //! @param  [in]  query   タイムスタンプのクエリ
        //! @param  [out] value   読み出した時刻（GPUのティック）
        //! @return true: 読み出せた, false: まだ届いていない
        //------------------------------------------------------------------------
        bool ReadTimestamp(ID3D11DeviceContext* context, ID3D11Query* query, u64& value) {
            // DONOTFLUSH：結果を急かすためのコマンドバッファの吐き出しをさせない
            return context->GetData(query, &value, sizeof(value), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK;
        }
    }    // namespace

    //------------------------------------------------------------------------
    //! クエリを作成します。
    //------------------------------------------------------------------------
    bool GpuProfiler::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
        m_enabled = false;
        if(!device || !context)
            return false;

        m_context = context;

        //--------------------------------------------------------------------
        // 3フレーム分のクエリを作る（1フレームあたり disjoint 1個＋タイムスタンプ 2 + 2×パス数）
        //--------------------------------------------------------------------
        for(FrameQueries& frame : m_frames) {
            if(!CreateQuery(device, D3D11_QUERY_TIMESTAMP_DISJOINT, frame.disjoint) ||
               !CreateQuery(device, D3D11_QUERY_TIMESTAMP, frame.frameBegin) || !CreateQuery(device, D3D11_QUERY_TIMESTAMP, frame.frameEnd)) {
                Tsukino::Core::Log::Error("GpuProfiler: Failed to create timestamp queries. GPU pass timings are disabled.");
                return false;
            }

            for(std::size_t i = 0; i < kGpuPassCount; ++i) {
                if(!CreateQuery(device, D3D11_QUERY_TIMESTAMP, frame.passBegin[i]) || !CreateQuery(device, D3D11_QUERY_TIMESTAMP, frame.passEnd[i])) {
                    Tsukino::Core::Log::Error("GpuProfiler: Failed to create timestamp queries. GPU pass timings are disabled.");
                    return false;
                }
            }
        }

        // 表示名は先に埋めておく（結果が届く前でも名前は引けるように）
        for(std::size_t i = 0; i < kGpuPassCount; ++i) {
            m_latest.passes[i].name = GetGpuPassName(static_cast<GpuPass>(i));
        }

        m_enabled = true;
        return true;
    }

    //------------------------------------------------------------------------
    //! 届いている結果を読み出し、このフレームの計測を始めます。
    //------------------------------------------------------------------------
    void GpuProfiler::BeginFrame() {
        if(!m_enabled)
            return;

        //--------------------------------------------------------------------
        // 2フレーム以上前の結果を古い順に読む。
        // これから書き込む段（3フレーム前）がまだ読めていなければ最後にもう一度試し、
        // それでも届いていなければ捨てて上書きする（結果を待って止まらないため）。
        // 次に古い段（2フレーム前）が読めなければ、次のフレームでもう一度試す
        //--------------------------------------------------------------------
        FrameQueries& current = m_frames[m_frameIndex % kFrameLatency];
        FrameQueries& older   = m_frames[(m_frameIndex + 1) % kFrameLatency];

        if(current.pending && !TryResolve(current))
            current.pending = false;

        if(older.pending)
            TryResolve(older);

        //--------------------------------------------------------------------
        // このフレームの計測を始める
        //--------------------------------------------------------------------
        current.passIssued.fill(false);
        m_context->Begin(current.disjoint.Get());
        m_context->End(current.frameBegin.Get());
        m_inFrame = true;
    }

    //------------------------------------------------------------------------
    //! パスの開始時刻を記録します。
    //------------------------------------------------------------------------
    void GpuProfiler::BeginPass(GpuPass pass) {
        if(!m_inFrame || pass >= GpuPass::Count)
            return;

        FrameQueries& current = m_frames[m_frameIndex % kFrameLatency];
        const std::size_t index = static_cast<std::size_t>(pass);

        // タイムスタンプのクエリは Begin を持たず、End で時刻を打つ
        m_context->End(current.passBegin[index].Get());
    }

    //------------------------------------------------------------------------
    //! パスの終了時刻を記録します。
    //------------------------------------------------------------------------
    void GpuProfiler::EndPass(GpuPass pass) {
        if(!m_inFrame || pass >= GpuPass::Count)
            return;

        FrameQueries& current = m_frames[m_frameIndex % kFrameLatency];
        const std::size_t index = static_cast<std::size_t>(pass);

        m_context->End(current.passEnd[index].Get());
        current.passIssued[index] = true;
    }

    //------------------------------------------------------------------------
    //! このフレームの計測を終えます。
    //------------------------------------------------------------------------
    void GpuProfiler::EndFrame() {
        if(!m_inFrame)
            return;

        FrameQueries& current = m_frames[m_frameIndex % kFrameLatency];
        m_context->End(current.frameEnd.Get());
        m_context->End(current.disjoint.Get());
        current.pending = true;

        m_inFrame = false;
        ++m_frameIndex;
    }

    //------------------------------------------------------------------------
    //! 1フレーム分の結果を読み出します。
    //------------------------------------------------------------------------
    bool GpuProfiler::TryResolve(FrameQueries& frame) {
        //--------------------------------------------------------------------
        // disjoint が届いていなければ、そのフレームのタイムスタンプもまだ揃っていない
        //--------------------------------------------------------------------
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
        if(m_context->GetData(frame.disjoint.Get(), &disjoint, sizeof(disjoint), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
            return false;

        //--------------------------------------------------------------------
        // 途中でGPUの周波数が変わったフレームはタイムスタンプが信用できないので捨てる
        //--------------------------------------------------------------------
        if(disjoint.Disjoint || disjoint.Frequency == 0) {
            frame.pending = false;
            return true;
        }

        const double ticksToMs = 1000.0 / static_cast<double>(disjoint.Frequency);

        u64 frameBegin = 0;
        u64 frameEnd   = 0;
        if(!ReadTimestamp(m_context, frame.frameBegin.Get(), frameBegin) || !ReadTimestamp(m_context, frame.frameEnd.Get(), frameEnd))
            return false;

        //--------------------------------------------------------------------
        // パスごとの時間。いったん手元の配列へ集め、全部読めたときだけ反映する
        // （一部だけ新しいフレームの値になると、合計が合わない表示になるため）
        //--------------------------------------------------------------------
        std::array<float, kGpuPassCount> passMs{};
        for(std::size_t i = 0; i < kGpuPassCount; ++i) {
            if(!frame.passIssued[i])
                continue;    // このフレームで実行されなかったパスは 0 ms

            u64 begin = 0;
            u64 end   = 0;
            if(!ReadTimestamp(m_context, frame.passBegin[i].Get(), begin) || !ReadTimestamp(m_context, frame.passEnd[i].Get(), end))
                return false;

            passMs[i] = (end > begin) ? static_cast<float>(static_cast<double>(end - begin) * ticksToMs) : 0.0f;
        }

        for(std::size_t i = 0; i < kGpuPassCount; ++i) {
            m_latest.passes[i].ms = passMs[i];
        }
        m_latest.frameMs = (frameEnd > frameBegin) ? static_cast<float>(static_cast<double>(frameEnd - frameBegin) * ticksToMs) : 0.0f;
        m_latest.valid   = true;

        frame.pending = false;
        return true;
    }
}    // namespace Tsukino::Renderer
