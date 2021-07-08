#include <cstring>

#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>

#include "misc.hpp"
#include "romaji-table.hpp"
#include "state.hpp"

namespace {
auto is_candidate_list_for(fcitx::InputContext& context, void* ptr) -> bool {
    const auto sp = context.inputPanel().candidateList();
    return sp && !(sp.get() != ptr);
}
auto begin_with(const std::u32string& str, const std::u32string& key) -> bool {
    if(str.size() < key.size()) {
        return false;
    }
    for(size_t i = 0; i < key.size(); ++i) {
        if(str[i] != key[i]) {
            return false;
        }
    }
    return true;
}
} // namespace
namespace mikan {
auto MikanState::handle_romaji(const fcitx::KeyEvent& event) -> bool {
    if(event.isRelease()) {
        return false;
    }
    const auto romaji_8 = fcitx::Key::keySymToUTF8(event.key().sym());
    if(fcitx_utf8_strlen(romaji_8.data()) != 1) {
        return false;
    }
    if(merge_branch_sentences()) {
        move_cursor_back();
    }

    to_kana += romaji_8;
    auto exact = romaji_index.filter(to_kana);
    if(exact == &romaji_table[romaji_table_limit]) {
        to_kana = romaji_8;
        exact = romaji_index.filter(to_kana);
        if(exact == &romaji_table[romaji_table_limit]) {
            to_kana.clear();
            return false;
        }
    }

    apply_candidates();
    delete_surrounding_text();
    if(exact != nullptr) {
        append_kana(exact->kana);
        if(exact->refill != nullptr) {
            to_kana = exact->refill;
        } else {
            to_kana.clear();
        }
    }
    return true;
}
auto MikanState::handle_delete(const fcitx::KeyEvent& event) -> bool {
    if(!share.key_config.match(Actions::BACKSPACE, event)) {
        return false;
    }
    if(!to_kana.empty()) {
        pop_back_u8(to_kana);
        return true;
    } else {
        Phrase* current_phrase;
        size_t  cursor_in_phrase;
        calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
        if(current_phrase == nullptr) {
            return false;
        }
        auto raw   = u8tou32(current_phrase->get_raw().get_feature());
        auto bytes = size_t(0);
        auto kana  = raw.begin();
        for(; kana != raw.end(); ++kana) {
            bytes += fcitx_ucs4_char_len(*kana);
            if(bytes == cursor_in_phrase) {
                break;
            }
        }
        if(kana != raw.end()) {
            // disassembly the kana to romaji and pop back.
            char kana8[FCITX_UTF8_MAX_LENGTH] = {};
            fcitx_ucs4_to_utf8(*kana, kana8);
            auto success = true;
            try {
                to_kana = kana_to_romaji(kana8);
            } catch(const std::runtime_error&) {
                success = false;
            }
            if(success) {
                pop_back_u8(to_kana);
            }

            // move cursor.
            cursor -= fcitx_ucs4_char_len(*kana);

            // delete character from the phrase.
            raw.erase(kana);
            current_phrase->get_mutable_feature() = u32tou8(raw);

            // translate.
            current_phrase->set_protection_level(ProtectionLevel::NONE);
            *phrases = translate_phrases(*phrases, true)[0];

            // phrases may be empty.
            if(phrases->empty()) {
                reset();
            }

            apply_candidates();
            return true;
        }
    }
    return false;
}
auto MikanState::handle_reinterpret_phrases(const fcitx::KeyEvent& event) -> bool {
    const static auto actions = std::vector{Actions::REINTERPRET_NEXT, Actions::REINTERPRET_PREV};
    if(phrases == nullptr || !share.key_config.match(actions, event)) {
        return false;
    }

    translation_changed = true;

    if(sentence_changed) {
        sentences.reset(translate_phrases(*phrases, false));
        sentence_changed = false;
    }
    if(!is_candidate_list_for(context, &sentences)) {
        context.inputPanel().setCandidateList(std::make_unique<CandidateList>(&sentences, share.candidate_page_size));
    }
    auto candidate_list = context.inputPanel().candidateList().get();
    if(share.key_config.match(Actions::REINTERPRET_NEXT, event)) {
        candidate_list->toCursorMovable()->nextCandidate();
    } else {
        candidate_list->toCursorMovable()->prevCandidate();
    }
    phrases = sentences.get_current_ptr();
    for(auto& p : *phrases) {
        p.set_protection_level(ProtectionLevel::PRESERVE_TRANSLATION);
    }
    return true;
}
auto MikanState::handle_candidates(const fcitx::KeyEvent& event) -> bool {
    const static auto actions = std::vector{Actions::CANDIDATE_NEXT, Actions::CANDIDATE_PREV, Actions::CANDIDATE_PAGE_NEXT, Actions::CANDIDATE_PAGE_PREV};
    if(!share.key_config.match(actions, event)) {
        return false;
    }
    Phrase* current_phrase;
    calc_phrase_in_cursor(&current_phrase, nullptr);
    if(current_phrase == nullptr) {
        return false;
    }

    if(!context.inputPanel().candidateList() && !share.key_config.match(Actions::CANDIDATE_NEXT, event)) {
        return false;
    }

    make_branch_sentence();

    // get candidate list.
    if(!current_phrase->is_candidates_initialized()) {
        calc_phrase_in_cursor(&current_phrase, nullptr);
        std::lock_guard<std::mutex> lock(share.primary_vocabulary.mutex);
        std::vector<MeCabModel*>    dic = {share.primary_vocabulary.data};
        std::copy(share.additional_vocabularies.begin(), share.additional_vocabularies.end(), std::back_inserter(dic));
        current_phrase->reset_candidates(PhraseCandidates(dic, current_phrase->get_raw().get_feature(), false));
        current_phrase->set_protection_level(ProtectionLevel::PRESERVE_TRANSLATION);
    }
    if(!current_phrase->has_candidates()) {
        return true;
    }

    if(!is_candidate_list_for(context, current_phrase)) {
        context.inputPanel().setCandidateList(std::make_unique<CandidateList>(&current_phrase->get_candidates(), share.candidate_page_size));
    }
    auto candidate_list = context.inputPanel().candidateList().get();

    // move candidate list index.
    if(share.key_config.match(Actions::CANDIDATE_NEXT, event)) {
        candidate_list->toCursorMovable()->nextCandidate();
    } else if(share.key_config.match(Actions::CANDIDATE_PREV, event)) {
        candidate_list->toCursorMovable()->prevCandidate();
    } else if(share.key_config.match(Actions::CANDIDATE_PAGE_NEXT, event)) {
        candidate_list->toPageable()->prev();
    } else if(share.key_config.match(Actions::CANDIDATE_PAGE_PREV, event)) {
        candidate_list->toPageable()->next();
    }

    return true;
}
auto MikanState::handle_commit_phrases(const fcitx::KeyEvent& event) -> bool {
    if(!share.key_config.match(Actions::COMMIT, event) || phrases == nullptr) {
        return false;
    }
    commit_all_phrases();
    reset();
    apply_candidates();
    return true;
}
auto MikanState::handle_move_cursor_phrase(const fcitx::KeyEvent& event) -> bool {
    const static auto actions = std::vector{Actions::PHRASE_NEXT, Actions::PHRASE_PREV};
    if(!share.key_config.match(actions, event)) {
        return false;
    }
    const auto forward = share.key_config.match(Actions::PHRASE_NEXT, event);
    Phrase*    current_phrase;
    Phrase*    new_phrase;
    size_t     cursor_in_phrase;
    calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
    if(current_phrase == nullptr) {
        return false;
    }
    new_phrase = forward ? current_phrase - 1 : current_phrase + 1;
    if(new_phrase < &(*phrases)[0] || new_phrase > &phrases->back()) {
        return false;
    }

    // move cursor.
    to_kana.clear();
    cursor = 0;
    for(auto p = &(*phrases)[0]; p <= new_phrase; ++p) {
        cursor += p->get_raw().get_feature().size();
    }
    apply_candidates();
    return true;
}
auto MikanState::handle_split_phrase(const fcitx::KeyEvent& event) -> bool {
    const static auto actions = std::vector{Actions::SPLIT_PHRASE_LEFT, Actions::SPLIT_PHRASE_RIGHT};
    if(!share.key_config.match(actions, event)) {
        return false;
    }

    make_branch_sentence();

    Phrase* current_phrase;
    size_t  cursor_in_phrase;
    calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
    if(current_phrase == nullptr || fcitx_utf8_strlen(current_phrase->get_raw().get_feature().data()) <= 1) {
        return false;
    }

    // insert a new phrase right after the selected phrase.
    const size_t phrase_index = current_phrase - &(*phrases)[0];

    phrases->insert(phrases->begin() + phrase_index + 1, Phrase());
    Phrase *a = &(*phrases)[phrase_index], *b = &(*phrases)[phrase_index + 1];

    // split 'a' into two.
    const auto   raw_32      = u8tou32(a->get_raw().get_feature());
    const size_t split_pos   = share.key_config.match(Actions::SPLIT_PHRASE_LEFT, event) ? 1 : raw_32.size() - 1;
    b->get_mutable_feature() = u32tou8(raw_32.substr(split_pos));
    a->get_mutable_feature() = u32tou8(raw_32.substr(0, split_pos));

    // protect them.
    a->set_protection_level(ProtectionLevel::PRESERVE_SEPARATION);
    b->set_protection_level(ProtectionLevel::PRESERVE_SEPARATION);

    apply_candidates();
    *phrases         = translate_phrases(*phrases, true)[0];
    sentence_changed = true;
    return true;
}
auto MikanState::handle_merge_phrase(const fcitx::KeyEvent& event) -> bool {
    const static auto actions = std::vector{Actions::MERGE_PHRASE_LEFT, Actions::MERGE_PHRASE_RIGHT};
    if(!share.key_config.match(actions, event)) {
        return false;
    }

    make_branch_sentence();

    Phrase* current_phrase;
    size_t  cursor_in_phrase;
    calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
    if(current_phrase == nullptr) {
        return true;
    }
    const auto left         = share.key_config.match(Actions::MERGE_PHRASE_LEFT, event);
    Phrase*    merge_phrase = current_phrase + (left ? -1 : 1);
    if(merge_phrase < &(*phrases)[0] || merge_phrase > &phrases->back()) {
        return true;
    }

    // merge 'current_phrase' and 'merge_phrase' into 'current_phrase'.
    std::string a = merge_phrase->get_raw().get_feature(), b = current_phrase->get_raw().get_feature();
    current_phrase->get_mutable_feature() = left ? a + b : b + a;
    current_phrase->set_protection_level(ProtectionLevel::PRESERVE_SEPARATION);

    // remove 'merge_phrase'.
    const size_t phrase_index = merge_phrase - &(*phrases)[0];
    phrases->erase(phrases->begin() + phrase_index);

    apply_candidates();
    *phrases = translate_phrases(*phrases, true)[0];
    return true;
}
auto MikanState::handle_move_separator(const fcitx::KeyEvent& event) -> bool {
    const static auto actions = std::vector{Actions::MOVE_SEPARATOR_LEFT, Actions::MOVE_SEPARATOR_RIGHT};
    if(!share.key_config.match(actions, event)) {
        return false;
    }

    make_branch_sentence();

    Phrase* current_phrase;
    size_t  cursor_in_phrase;
    calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
    if(current_phrase == nullptr) {
        return true;
    }
    Phrase* move_phrase = current_phrase + 1;
    if(move_phrase > &phrases->back()) {
        return true;
    }

    // align cursor to phrase back.
    cursor += current_phrase->get_raw().get_feature().size() - cursor_in_phrase;

    // move cursor and separator.
    size_t     moved_chara_size;
    const auto left = share.key_config.match(Actions::MOVE_SEPARATOR_LEFT, event);
    if(left) {
        auto current_u32 = u8tou32(current_phrase->get_raw().get_feature());
        moved_chara_size = fcitx_ucs4_char_len(current_u32.back());
        move_phrase->get_mutable_feature().insert(0, u32tou8(current_u32.back()));
        current_u32.pop_back();
        current_phrase->get_mutable_feature() = u32tou8(current_u32);
    } else {
        auto move_u32    = u8tou32(move_phrase->get_raw().get_feature());
        moved_chara_size = fcitx_ucs4_char_len(move_u32[0]);
        current_phrase->get_mutable_feature() += u32tou8(move_u32[0]);
        move_u32.erase(0, 1);
        move_phrase->get_mutable_feature() = u32tou8(move_u32);
    }
    if(left) {
        cursor -= moved_chara_size;
    } else {
        cursor += moved_chara_size;
    }

    current_phrase->set_protection_level(ProtectionLevel::PRESERVE_SEPARATION);
    move_phrase->set_protection_level(ProtectionLevel::PRESERVE_SEPARATION);
    apply_candidates();
    *phrases         = translate_phrases(*phrases, true)[0];
    sentence_changed = true;
    return true;
}
auto MikanState::handle_convert_katakana(const fcitx::KeyEvent& event) -> bool {
    if(!share.key_config.match(Actions::CONVERT_KATAKANA, event)) {
        return false;
    }

    make_branch_sentence();

    Phrase* current_phrase;
    calc_phrase_in_cursor(&current_phrase);
    if(current_phrase == nullptr) {
        return false;
    }

    auto        katakana = std::string();
    const auto& raw      = current_phrase->get_raw().get_feature();
    auto        u32      = u8tou32(raw);
    auto        options  = std::vector<const HiraKata*>();
    size_t      char_num = 0;
    for(auto c = u32.begin(); c != u32.end(); ++c) {
        auto term     = std::u32string(c - char_num, c + 1);
        auto filtered = std::vector<const HiraKata*>();
        for(size_t i = 0; i < hiragana_katakana_table_limit; ++i) {
            const auto& hirakata = hiragana_katakana_table[i];
            if(begin_with(hirakata.hiragana, term)) {
                emplace_unique(filtered, &hirakata, [](const HiraKata* h, const HiraKata* o) {
                    return h->hiragana == o->hiragana;
                });
            }
        }
        if(filtered.size() == 1) {
            katakana += filtered[0]->katakana;
            char_num = 0;
            options.clear();
        } else if(filtered.empty() || c + 1 == u32.end()) {
            if(!filtered.empty()) {
                options = std::move(filtered);
            }
            if(options.empty()) {
                katakana += u32tou8(*c);
            } else {
                if(char_num != 0) {
                    const auto& u8str  = options[0]->katakana;
                    const auto  u32str = u8tou32(u8str);
                    katakana += u32tou8(u32str.substr(0, char_num));
                } else {
                    katakana += options[0]->katakana;
                }
                options.clear();
                char_num = 0;
            }
        } else {
            options = std::move(filtered);
            char_num += 1;
        }
    }
    {
        // try to get word information(if exists in dictionary).
        auto  lock    = std::lock_guard<std::mutex>(share.primary_vocabulary.mutex);
        auto& dic     = share.primary_vocabulary.data;
        auto& lattice = *dic->lattice;
        lattice.set_request_type(MECAB_ONE_BEST);
        lattice.set_sentence(raw.data());
        lattice.set_feature_constraint(0, raw.size(), katakana.data());
        dic->tagger->parse(&lattice);
        auto found = false;
        for(const auto* node = lattice.bos_node(); node; node = node->next) {
            if(node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) {
                continue;
            }
            if(node->stat == MECAB_NOR_NODE) {
                found           = true;
                *current_phrase = Phrase(MeCabWord(raw), MeCabWord(katakana, node->rcAttr, node->lcAttr, node->wcost, node->cost, true));
                break;
            }
        }
        lattice.clear();
        if(!found) {
            current_phrase->override_translated(MeCabWord(katakana));
        }
    }

    apply_candidates();
    current_phrase->set_protection_level(ProtectionLevel::PRESERVE_TRANSLATION);
    return true;
}
const std::vector<std::function<bool(MikanState*, const fcitx::KeyEvent&)>> MikanState::handlers = {
    &MikanState::handle_candidates,
    &MikanState::handle_delete,
    &MikanState::handle_reinterpret_phrases,
    &MikanState::handle_commit_phrases,
    &MikanState::handle_move_cursor_phrase,
    &MikanState::handle_split_phrase,
    &MikanState::handle_merge_phrase,
    &MikanState::handle_move_separator,
    &MikanState::handle_convert_katakana,
    &MikanState::handle_romaji,
};
} // namespace mikan
