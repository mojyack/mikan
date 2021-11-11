#include "phrase.hpp"

namespace mikan {
auto Phrase::get_protection_level() const -> ProtectionLevel {
    return protection;
}
auto Phrase::set_protection_level(ProtectionLevel level) -> void {
    protection = level;
}
auto Phrase::get_raw() const -> const MeCabWord& {
    return candidates.get_raw();
}
auto Phrase::get_translated() const -> const MeCabWord& {
    return candidates.get_translated();
}
auto Phrase::get_mutable_feature() -> std::string& {
    return candidates.get_mutable_feature();
}
auto Phrase::override_translated(const MeCabWord& translated) -> void {
    candidates.override_translated(translated);
}
auto Phrase::override_translated(MeCabWord&& translated) -> void {
    candidates.override_translated(translated);
}
auto Phrase::is_candidates_initialized() const -> bool {
    return candidates.is_initialized();
}
auto Phrase::has_candidates() const -> bool {
    return candidates.has_candidates();
}
auto Phrase::get_candidates() -> PhraseCandidates& {
    return candidates;
}
auto Phrase::reset_candidates(PhraseCandidates&& new_candidates) -> void {
    candidates = std::move(new_candidates);
}
Phrase::Phrase() : candidates(MeCabWord()){};
Phrase::Phrase(const std::string& raw) : candidates(MeCabWord(raw)) {}
Phrase::Phrase(MeCabWord&& raw, MeCabWord&& translated) : candidates(MeCabWord(raw), MeCabWord(translated)) {}

namespace {
auto get_node_surface(const MeCab::Node* const node) -> std::string {
    return std::string(node->surface, node->length);
}
MeCabWord node_to_word(const MeCab::Node* const node) {
    if(node->stat == MECAB_UNK_NODE) {
        return MeCabWord(get_node_surface(node));
    } else {
        return MeCabWord(node->feature, node->rcAttr, node->lcAttr, node->wcost, node->cost, true);
    }
}
} // namespace
Phrase::Phrase(const MeCab::Node* const node) : candidates(MeCabWord(get_node_surface(node)), node_to_word(node)) {}
} // namespace mikan
