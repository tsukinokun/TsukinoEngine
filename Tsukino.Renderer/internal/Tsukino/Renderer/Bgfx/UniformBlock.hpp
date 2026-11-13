//----------------------------------------------------------------------------
//! @file   UniformBlock.hpp
//! @brief  定数バッファの構造体を bgfx のユニフォームで送る仕組み（Renderer の内部用）
//! @detail DX11 版の定数バッファ1つを、bgfx の vec4 配列のユニフォーム1つで送ります。
//!         C++ の構造体（ConstantBuffer.hpp）は変えず、そのままのバイト列を送ります。
//!         シェーダー（.sc）の側は、同じ名前の `uniform vec4 u_xxx[N]` を宣言し、
//!         メンバーは #define で配列の要素へ当てて読みます（Shaders/bgfx/Scene.sh など）。
//----------------------------------------------------------------------------
#pragma once
#include <Tsukino/Core/typedef.hpp>

#include <bgfx/bgfx.h>

// 名前空間 : Tsukino::Renderer
namespace Tsukino::Renderer {

    //------------------------------------------------------------------------
    //! 定数バッファの構造体 T を、vec4 配列のユニフォーム1つとして送るクラスです。
    //! @note  T の大きさは 16 バイトの倍数であること（HLSL の定数バッファと同じ決まり）。
    //!        hlslpp の型は trivially copyable ではないが、中身は float の並びなので、
    //!        DX11 版の UpdateSubresource と同じくバイト列のまま送ってよい。
    //!        行列は C++ と同じ行優先の並びで届くので、シェーダーでは mtxFromRows で組み立てる。
    //!        bgfx のユニフォームは描画1回ごとの値なので、Submit の前に毎回 Set する
    //------------------------------------------------------------------------
    template <class T>
    class UniformBlock {
        static_assert(sizeof(T) % 16 == 0, "UniformBlock: the struct size must be a multiple of 16 bytes (one vec4).");

    public:
        //! 構造体1つぶんの vec4 の数
        static constexpr u16 kVec4Count = static_cast<u16>(sizeof(T) / 16);

        UniformBlock() = default;
        UniformBlock(const UniformBlock&)            = delete;
        UniformBlock& operator=(const UniformBlock&) = delete;

        //! ユニフォームを破棄します。
        ~UniformBlock() {
            Destroy();
        }

        //! ユニフォームを作成します。
        //! @param  [in] name シェーダー側の配列の名前（例 "u_scene"）
        //! @return true: 作成成功
        bool Create(const char* name) {
            Destroy();
            m_handle = bgfx::createUniform(name, bgfx::UniformType::Vec4, kVec4Count);
            return bgfx::isValid(m_handle);
        }

        //! ユニフォームを破棄します（作っていなければ何もしません）。
        void Destroy() {
            if(bgfx::isValid(m_handle)) {
                bgfx::destroy(m_handle);
                m_handle = BGFX_INVALID_HANDLE;
            }
        }

        //! 次に Submit する描画へ、構造体の中身を送ります。
        //! @param  [in] data 送る構造体
        void Set(const T& data) const {
            if(bgfx::isValid(m_handle))
                bgfx::setUniform(m_handle, &data, kVec4Count);
        }

        //! ユニフォームが作られているかを返します。
        //! @return 作られていれば true
        [[nodiscard]]
        bool IsValid() const noexcept {
            return bgfx::isValid(m_handle);
        }

    private:
        bgfx::UniformHandle m_handle = BGFX_INVALID_HANDLE;    // vec4 配列のユニフォーム
    };

}    // namespace Tsukino::Renderer
