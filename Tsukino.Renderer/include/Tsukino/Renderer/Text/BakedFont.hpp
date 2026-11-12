//----------------------------------------------------------------------------
//! @file   BakedFont.hpp
//! @brief  事前ベイクしたフォント（.spritefont）の宣言
//! @detail 文字の画像を1枚に焼き込んだフォントを読み込み、文字列をスプライトの並びにします。
//!         読み込みと文字の画像の持ち方は描画 API ごとの中身（BakedFont::Impl）が担当します。
//!         日本語のように文字の多い言語は DynamicFontAtlas を使います。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>
#include <Tsukino/Renderer/DrawCommand.hpp>

#include <hlsl++.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {
    class RenderResources;    // 前方宣言

    //------------------------------------------------------------------------
    //! 事前ベイクしたフォントです。
    //------------------------------------------------------------------------
    class BakedFont {
    public:
        struct Impl;    //!< 描画 API ごとの中身（Renderer の内部で定義）

        //! フォントのバイナリを読み込みます。
        //! @param  [in] resources 文字の画像を置く先（フォントより長生きさせること）
        //! @param  [in] data      .spritefont のバイナリ
        //! @param  [in] size      バイナリのバイト数
        //! @return 読み込んだフォント。読めなかったときは nullptr
        [[nodiscard]]
        static std::unique_ptr<BakedFont> Create(RenderResources& resources, const u8* data, std::size_t size);

        //! フォントを破棄します。
        ~BakedFont();

        BakedFont(const BakedFont&)            = delete;
        BakedFont& operator=(const BakedFont&) = delete;

        //! 文字をフォントが持っているかを返します。
        //! @param  [in] ch 調べる文字
        //! @return 持っていれば true
        [[nodiscard]]
        bool ContainsCharacter(wchar_t ch) const;

        //! 文字列の描画サイズを測ります。
        //! @param  [in] text 測る文字列（フォントが持つ文字だけにしておくこと）
        //! @return 幅と高さ（ピクセル。拡大前）
        [[nodiscard]]
        hlslpp::float2 MeasureString(const std::wstring& text) const;

        //! 文字列を、グリフ1つにつき1枚のスプライトにして並びへ足します。
        //! @param  [in,out] outSprites スプライトの追加先（DrawCommandQueue::PushSprites へ渡す）
        //! @param  [in]     text       描く文字列（フォントが持つ文字だけにしておくこと）
        //! @param  [in]     position   描画位置（ピクセル）
        //! @param  [in]     color      文字色
        //! @param  [in]     rotation   回転（ラジアン。画面上で時計回りが正）
        //! @param  [in]     origin     原点（スケール適用前）
        //! @param  [in]     scale      拡大率
        //! @note   DirectXTK の SpriteFont::DrawString（反転なし）と同じ並べ方をする。
        //!         グリフの位置は、各スプライトの軸（origin）を文字列の原点からずらして表す
        void AppendStringSprites(std::vector<SpriteQuad>& outSprites, const std::wstring& text, hlslpp::float2 position,
                                 hlslpp::float4 color, float rotation, hlslpp::float2 origin, float scale) const;

    private:
        //! 中身を受け取って作ります（Create() から呼ぶ）。
        //! @param  [in] impl 読み込み済みの中身
        explicit BakedFont(std::unique_ptr<Impl> impl);

        std::unique_ptr<Impl> m_impl;    // 描画 API ごとの中身
    };
}    // namespace Tsukino::Renderer
