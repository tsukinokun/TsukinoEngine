//----------------------------------------------------------------------------
//! @file   Passes.cpp
//! @brief  bgfx 版の各パス・ベイク済みフォントの仮の実装
//! @detail どれもまだ GPU の資源を持たず、設定を受け取るだけです。
//!         DX11 版から1つずつ移すときに、パスごとのファイルへ分けます。
//----------------------------------------------------------------------------
#include <Tsukino/Renderer/AmbientParticlePass.hpp>
#include <Tsukino/Renderer/BloomPass.hpp>
#include <Tsukino/Renderer/FogPass.hpp>
#include <Tsukino/Renderer/IBLBaker.hpp>
#include <Tsukino/Renderer/MotionBlurPass.hpp>
#include <Tsukino/Renderer/SkyPass.hpp>
#include <Tsukino/Renderer/SphereColorGradePass.hpp>
#include <Tsukino/Renderer/Text/BakedFont.hpp>
#include <Tsukino/Renderer/Bgfx/IBLBakerGpu.hpp>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    // 各パスの GPU の資源（bgfx 版はまだ空）と、作成・破棄
    //------------------------------------------------------------------------
    struct AmbientParticlePass::Gpu {};
    struct BloomPass::Gpu {};
    struct FogPass::Gpu {};
    struct MotionBlurPass::Gpu {};
    struct SkyPass::Gpu {};
    struct SphereColorGradePass::Gpu {};

    AmbientParticlePass::AmbientParticlePass() : m_gpu(std::make_unique<Gpu>()) {}
    AmbientParticlePass::~AmbientParticlePass() = default;
    BloomPass::BloomPass() : m_gpu(std::make_unique<Gpu>()) {}
    BloomPass::~BloomPass() = default;
    FogPass::FogPass() : m_gpu(std::make_unique<Gpu>()) {}
    FogPass::~FogPass() = default;
    IBLBaker::IBLBaker() : m_gpu(std::make_unique<Gpu>()) {}
    IBLBaker::~IBLBaker() = default;
    MotionBlurPass::MotionBlurPass() : m_gpu(std::make_unique<Gpu>()) {}
    MotionBlurPass::~MotionBlurPass() = default;
    SkyPass::SkyPass() : m_gpu(std::make_unique<Gpu>()) {}
    SkyPass::~SkyPass() = default;
    SphereColorGradePass::SphereColorGradePass() : m_gpu(std::make_unique<Gpu>()) {}
    SphereColorGradePass::~SphereColorGradePass() = default;

    //------------------------------------------------------------------------
    //! 空のシェーダーを設定します（bgfx 版はまだ空を描かない）。
    //------------------------------------------------------------------------
    void SkyPass::SetPipeline(const Tsukino::Asset::ShaderAsset* /*vs*/, const Tsukino::Asset::ShaderAsset* /*ps*/) {}

    //------------------------------------------------------------------------
    // ベイク済みフォント（bgfx 版はまだ読めないので、Create() は nullptr を返す）
    //------------------------------------------------------------------------
    struct BakedFont::Impl {};

    //------------------------------------------------------------------------
    //! フォントのバイナリを読み込みます（bgfx 版はまだ読めない）。
    //------------------------------------------------------------------------
    std::unique_ptr<BakedFont> BakedFont::Create(RenderResources& /*resources*/, const u8* /*data*/, std::size_t /*size*/) {
        return nullptr;
    }

    //------------------------------------------------------------------------
    //! 中身を受け取って作ります。
    //------------------------------------------------------------------------
    BakedFont::BakedFont(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

    //------------------------------------------------------------------------
    //! フォントを破棄します。
    //------------------------------------------------------------------------
    BakedFont::~BakedFont() = default;

    //------------------------------------------------------------------------
    //! 文字をフォントが持っているかを返します（作られないので呼ばれない）。
    //------------------------------------------------------------------------
    bool BakedFont::ContainsCharacter(wchar_t /*ch*/) const {
        return false;
    }

    //------------------------------------------------------------------------
    //! 文字列の描画サイズを測ります（作られないので呼ばれない）。
    //------------------------------------------------------------------------
    hlslpp::float2 BakedFont::MeasureString(const std::wstring& /*text*/) const {
        return hlslpp::float2(0.0f, 0.0f);
    }

    //------------------------------------------------------------------------
    //! 文字列をスプライトの並びにします（作られないので呼ばれない）。
    //------------------------------------------------------------------------
    void BakedFont::AppendStringSprites(std::vector<SpriteQuad>& /*outSprites*/, const std::wstring& /*text*/, hlslpp::float2 /*position*/,
                                        hlslpp::float4 /*color*/, float /*rotation*/, hlslpp::float2 /*origin*/, float /*scale*/) const {}

}    // namespace Tsukino::Renderer
