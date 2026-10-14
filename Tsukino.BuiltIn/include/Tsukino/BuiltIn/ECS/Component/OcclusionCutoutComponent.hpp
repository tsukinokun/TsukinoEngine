//--------------------------------------------------------------
//! @file   OcclusionCutoutComponent.hpp
//! @brief  「手前の遮り物を網点で抜く」対象であることを表すコンポーネント
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {
    //--------------------------------------------------------------
    //! @struct OcclusionCutoutComponent
    //! @brief  このモデルを、遮り抜き（FrameConstants::SetOcclusionCutout）の対象にする
    //! @note   付いていてenabledなら、ModelSystemが描画コマンドのDrawCommand::occlusionCutoutを立てる。
    //!         抜く円の位置・大きさ・奥行きは毎フレームアプリ側がFrameConstantsへ渡す
    //!         （エンジンは「誰の周りを抜くか」を知らない。注目したいキャラの周りを渡すのはアプリの仕事）。
    //!         持ち主によって抜く・抜かないを切り替えたいとき（拾える武器を持っている間は抜かない等）は
    //!         enabledを書き換える。
    //!
    //!         ditherFadeは円とは関係なく、物全体を網点で薄くする割合（DrawCommand::ditherFadeへそのまま渡す）。
    //!         演出で物を丸ごと透かしたいとき（倒れた後に敵を全部透かす等）にアプリ側が毎フレーム書く。
    //!         実行時の値なのでPrefabには保存しない
    //--------------------------------------------------------------
    struct OcclusionCutoutComponent {
        bool  enabled    = true;    // falseの間は円の遮り抜きをしない
        float ditherFade = 1.0f;    // 物全体の見えている割合（1で普通に描く、0で全部抜ける）。enabledに関係なく効く。保存しない
    };
}    // namespace Tsukino::BuiltIn::ECS
