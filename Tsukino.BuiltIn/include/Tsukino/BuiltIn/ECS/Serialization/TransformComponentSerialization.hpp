//--------------------------------------------------------------
//! @file   TransformComponentSerialization.hpp
//! @brief  TransformComponentのcerealシリアライズ定義
//! @author 山﨑愛
//--------------------------------------------------------------
#pragma once
#include <Tsukino/BuiltIn/ECS/Component/TransformComponent.hpp>

#include <Tsukino/Core/Math/Serialization/HlslppSerialization.hpp>

#include <cereal/cereal.hpp>

// 名前空間 : Tsukino::BuiltIn::ECS
namespace Tsukino::BuiltIn::ECS {

    //--------------------------------------------------------------
    //! @brief  cereal用：保存（セーブ）処理の定義
    //--------------------------------------------------------------
    template <class Archive>
    void save(Archive& archive, const TransformComponent& transform) {
        //--------------------------------------------------------------
        // インスペクタで編集する「位置・回転・スケール」と、親子関係を保存する
        // 行列キャッシュ（local/world）は毎フレーム再計算されるため除外
        //
        // parent は EntityRef なので save_minimal 経由で localName（"#PenguinCenter"
        // のような名前）だけが文字列として書き出される。C++側から
        // TransformUtility::SetParent で親を付けた場合は localName が空のため
        // 空文字が書き出される（＝ラウンドトリップでは親が復元されない）点に注意
        //--------------------------------------------------------------
        archive(cereal::make_nvp("position", transform.position), cereal::make_nvp("rotation", transform.rotation), cereal::make_nvp("scale", transform.scale),
                cereal::make_nvp("parent", transform.parent));
    }

    //--------------------------------------------------------------
    //! @brief  cereal用：読み込み（ロード）処理の定義
    //--------------------------------------------------------------
    template <class Archive>
    void load(Archive& archive, TransformComponent& transform) {
        //--------------------------------------------------------------
        // 保存時と同じ順番で復元
        //--------------------------------------------------------------
        archive(transform.position, transform.rotation, transform.scale);

        //--------------------------------------------------------------
        // parent は後から追加したフィールドのため、既存のTransform.jsonには
        // キーが存在しない。cerealのJSONInputArchiveはNVPが見つからないと
        // 例外を投げるので、その場合は「親なし（AddComponent時の初期値）」の
        // ままにして読み飛ばす。
        //
        // 例外を投げるのはIterator::searchで、itsIndexを進める前に throw し、
        // 呼び出し元のsearch()も itsNextName を先にクリアしてから探索するため、
        // ここでcatchしてもアーカイブの読み取り位置は壊れない
        // （cereal/archives/json.hpp の search() 実装のコメント参照）
        //--------------------------------------------------------------
        try {
            archive(cereal::make_nvp("parent", transform.parent));
        } catch(const cereal::Exception&) {
            // parentキーなし = 親なし。初期値のまま何もしない
        }
        //--------------------------------------------------------------
        // 初期パラメータが変わったので、行列（local/world）を
        // 次フレームで強制再計算させるために dirty フラグを true にする
        //--------------------------------------------------------------
        transform.dirty = true;
    }

}    // namespace Tsukino::BuiltIn::ECS
