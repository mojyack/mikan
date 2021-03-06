#include <cstddef>
#include <mutex>
#include <stdexcept>
#include <string>

#include <fcitx-utils/cutf8.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>
#include <fcitx/surroundingtext.h>
#include <mecab.h>

#include "candidate.hpp"
#include "configuration.hpp"
#include "mecab-model.hpp"
#include "mikan5.hpp"
#include "misc.hpp"
#include "phrase.hpp"
#include "romaji-table.hpp"
#include "state.hpp"
#include "type.hpp"

namespace {
template <typename T>
T* get_candidate_list_for(const fcitx::InputPanel& input_panel) {
    return dynamic_cast<T*>(input_panel.candidateList().get());
}
bool is_candidate_list_for(fcitx::InputContext& context, void* ptr) {
    auto sp = context.inputPanel().candidateList();
    return sp && !(sp.get() != ptr);
}
bool begin_with(const std::u32string& str, const std::u32string& key) {
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

void MikanState::commit_phrase(const Phrase* phrase) {
    const MeCabWord& translated = phrase->get_translated();
    context.commitString(translated.get_feature());
}
void MikanState::commit_all_phrases() {
    merge_branch_sentences();
    Phrase* current;
    calc_phrase_in_cursor(&current);
    for(const auto& p : *phrases) {
        commit_phrase(&p);
        if(current == &p && !to_kana.empty()) {
            context.commitString(to_kana);
            to_kana.clear();
        }
    }
}
void MikanState::make_branch_sentence() {
    if(!translation_changed) {
        shrink_sentences(true);
        translation_changed = true;
    }
}
bool MikanState::merge_branch_sentences() {
    if(phrases == nullptr) {
        return false;
    }
    if(sentences.get_index() != 0) {
        // if sentences.index != 0, it means user fixed translation manually.
        // so we have to check the differences between sentences[0] and *phrases.
        engine.request_fix_dictionary(*phrases);
    }
    if(translation_changed) {
        shrink_sentences();
        translation_changed = false;
        return true;
    } else {
        return false;
    }
}
void MikanState::shrink_sentences(bool reserve_one) {
    if(phrases == nullptr) {
        return;
    }
    if(reserve_one) {
        sentences.reset({*phrases, *phrases});
        sentences.set_index(1);
    } else {
        sentences.reset({*phrases});
    }
    phrases = sentences.get_current_ptr();
}
bool MikanState::delete_surrounding_text() {
    if(phrases != nullptr || !to_kana.empty() || !context.capabilityFlags().test(fcitx::CapabilityFlag::SurroundingText)) {
        return false;
    }
    const auto& text = context.surroundingText();
    if(!text.isValid()) {
        return false;
    }
    context.deleteSurroundingText(0, text.anchor() - text.cursor());
    return true;
}
void MikanState::calc_phrase_in_cursor(Phrase** phrase, size_t* cursor_in_phrase) const {
    if(phrases == nullptr) {
        *phrase = nullptr;
        if(cursor_in_phrase != nullptr) {
            *cursor_in_phrase = 0;
        }
        return;
    }
    size_t total_bytes = 0;
    for(const auto& p : *phrases) {
        size_t phrase_end = total_bytes + p.get_raw().get_feature().size();
        if(cursor > total_bytes && cursor <= phrase_end) {
            *phrase = const_cast<Phrase*>(&p);
            if(cursor_in_phrase != nullptr) {
                *cursor_in_phrase = cursor - total_bytes;
            }
            return;
        }
        total_bytes = phrase_end;
    }
    throw std::runtime_error("cursor is not in any phrase.");
    return;
}
void MikanState::move_cursor_back() {
    if(phrases == nullptr) {
        return;
    }
    // move cursor to back.
    cursor = 0;
    for(const auto& p : *phrases) {
        cursor += p.get_raw().get_feature().size();
    }
}
void MikanState::append_kana(const std::string& kana) {
    // append kana to selected phrase.
    if(phrases == nullptr) {
        sentences.reset({Phrases{Phrase{kana}}});
        phrases = sentences.get_current_ptr();
        cursor  = kana.size();
    } else {
        Phrase* current;
        size_t  cursor_pos;
        calc_phrase_in_cursor(&current, &cursor_pos);
        if(current->get_protection_level() != ProtectionLevel::NONE) {
            phrases->emplace_back();
            current    = &phrases->back();
            cursor_pos = 0;
        }
        current->get_mutable().insert(cursor_pos, kana);
        current->set_protection_level(ProtectionLevel::NONE);
        cursor += kana.size();
        sentence_changed = true;
    }
    *phrases = translate_phrases(*phrases, true)[0];
    auto_commit();
}
Sentences MikanState::translate_phrases(const Phrases& source, bool best_only) {
    if(phrases == nullptr) {
        return Sentences();
    }
    return engine.translate_phrases(source, best_only);
}
fcitx::Text MikanState::build_preedit_text() const {
    fcitx::Text preedit;
    if(phrases == nullptr) {
        std::string text = to_kana;
        preedit.append(to_kana, fcitx::TextFormatFlag::Underline);
        preedit.setCursor(to_kana.size());
        return preedit;
    }

    Phrase* current;
    size_t  cursor_in_phrase;
    size_t  preedit_cursor = 0;
    calc_phrase_in_cursor(&current, &cursor_in_phrase);
    const bool is_current_last = current == &phrases->back();
    for(size_t i = 0, limit = phrases->size(); i < limit; ++i) {
        const auto& phrase = (*phrases)[i];
        auto        text   = std::string();
        auto        format = fcitx::TextFormatFlag::NoFlag;
        if(&phrase == current) {
            if(!to_kana.empty()) {
                format = fcitx::TextFormatFlag::Underline;
                text   = phrase.get_raw().get_feature();
                text.insert(cursor_in_phrase, to_kana);
                preedit_cursor += cursor_in_phrase + to_kana.size();
            } else {
                format = fcitx::TextFormatFlag::Bold;
                text   = phrase.get_translated().get_feature();
                preedit_cursor += text.size();
            }
            preedit.setCursor(preedit_cursor);
        } else {
            format = fcitx::TextFormatFlag::Underline;
            text   = phrase.get_translated().get_feature();
            preedit_cursor += text.size();
        }
        preedit.append(text, format);
        // add space between phrases.
        if(share.insert_space == InsertSpaceOptions::On || (share.insert_space == InsertSpaceOptions::Smart && !is_current_last)) {
            const static std::string space      = " ";
            std::string              space_copy = space;
            preedit.append(space_copy);
            preedit_cursor += space.size();
        }
    }
    return preedit;
}
void MikanState::apply_candidates() {
    if(!context.inputPanel().candidateList()) {
        return;
    }
    // hide candidate list window.
    context.inputPanel().setCandidateList(nullptr);
}
void MikanState::auto_commit() {
    if(phrases == nullptr || phrases->size() < share.auto_commit_threshold || phrases->size() < 2) {
        return;
    }
    size_t commit_num = phrases->size() - share.auto_commit_threshold + 1;
    size_t on_holds   = 0;
    bool   commited   = false;
    for(size_t i = 0; i <= commit_num; ++i) {
        // we have to ensure that the following phrases' translations will be the same without this phrase.
        if((*phrases)[on_holds].get_protection_level() != ProtectionLevel::PRESERVE_TRANSLATION) {
            std::vector<Phrase> copy(phrases->begin() + on_holds + 1, phrases->end());
            copy = translate_phrases(copy, true)[0];
            if(copy[0].get_translated().get_feature() != (*phrases)[on_holds + 1].get_translated().get_feature()) {
                // translation result will be changed.
                // but maybe we can commit this phrase with next one.
                on_holds += 1;
                continue;
            }
        }

        // commit phrases.
        for(size_t i = 0; i <= on_holds; ++i) {
            commit_phrase(&(*phrases)[0]);
            size_t commited_bytes = (*phrases)[0].get_raw().get_feature().size();
            if(cursor >= commited_bytes) {
                cursor -= commited_bytes;
            } else {
                cursor = 0;
            }
            phrases->erase(phrases->begin());
        }
        on_holds = 0;
        commited = true;
    }
    if(commited) {
        apply_candidates();
        sentence_changed = true;
    }
}
void MikanState::reset() {
    sentences.clear();
    phrases          = nullptr;
    sentence_changed = true;
}
bool MikanState::handle_romaji(const fcitx::KeyEvent& event) {
    if(event.isRelease()) {
        return false;
    }
    const std::string romaji_8 = fcitx::Key::keySymToUTF8(event.key().sym());
    if(fcitx_utf8_strlen(romaji_8.data()) != 1) {
        return false;
    }
    if(merge_branch_sentences()) {
        move_cursor_back();
    }
    auto filter = [](std::vector<size_t>& filtered, size_t i, size_t insert_pos, char32_t romaji) -> bool {
        enum class JudgeResult {
            FAIL,
            CONTINUE,
            MATCH,
        };
        auto judge = [romaji, insert_pos](const RomajiKana& romaji_kana) -> JudgeResult {
            if(romaji_kana.romaji.size() <= insert_pos) {
                return JudgeResult::FAIL;
            }
            if(romaji_kana.romaji[insert_pos] == romaji) {
                if(romaji_kana.romaji.size() == insert_pos + 1) {
                    return JudgeResult::MATCH;
                }
                return JudgeResult::CONTINUE;
            };
            return JudgeResult::FAIL;
        };
        switch(judge(romaji_table[i])) {
        case JudgeResult::FAIL:
            return false;
        case JudgeResult::CONTINUE:
            filtered.emplace_back(i);
            return false;
        case JudgeResult::MATCH:
            return true;
        }
    };
#define FILTER(f, i, p, r)          \
    if(filter(f, i, p, r)) {        \
        matched = &romaji_table[i]; \
        break;                      \
    }
#define RESET(f, p, r)                               \
    for(size_t i = 0; i < romaji_table_limit; ++i) { \
        FILTER(f, i, p, r)                           \
    }
    std::vector<size_t> filtered;
    const RomajiKana*   matched    = nullptr;
    const char32_t      romaji     = fcitx_utf8_get_char_validated(romaji_8.data(), romaji_8.size(), nullptr);
    const size_t        insert_pos = fcitx_utf8_strlen(to_kana.data());
    if(romaji_table_indexs.empty()) {
        RESET(filtered, insert_pos, romaji)
    } else {
        for(auto itr = romaji_table_indexs.cbegin(); itr != romaji_table_indexs.cend(); ++itr) {
            FILTER(filtered, *itr, insert_pos, romaji)
        }
    }
    if(matched == nullptr) {
        romaji_table_indexs = std::move(filtered);
        if(romaji_table_indexs.empty()) {
            // no kana matched with current to_kana. try once more without it.
            RESET(romaji_table_indexs, 0, romaji)
            if(!romaji_table_indexs.empty() || matched != nullptr) {
                to_kana.clear();
            }
        }
        if(romaji_table_indexs.size() == 1) {
            // only one result.
            matched = &romaji_table[romaji_table_indexs[0]];
        }
    }
    if(!romaji_table_indexs.empty() || matched != nullptr) {
        apply_candidates();
        delete_surrounding_text();
        if(matched != nullptr) {
            append_kana(matched->kana);
            romaji_table_indexs.clear();
            if(matched->refill != nullptr) {
                RESET(romaji_table_indexs, 0, fcitx_utf8_get_char_validated(matched->refill, std::strlen(matched->refill), nullptr));
                to_kana = matched->refill;
            } else {
                to_kana.clear();
            }

        } else {
            to_kana += romaji_8;
        }
        return true;
    } else {
        return false;
    }
#undef FILTER
#undef RESET
}
bool MikanState::handle_commit_phrases(const fcitx::KeyEvent& event) {
    if(!share.key_config.match(Actions::COMMIT, event) || phrases == nullptr) {
        return false;
    }
    commit_all_phrases();
    reset();
    apply_candidates();
    return true;
}
bool MikanState::handle_delete(const fcitx::KeyEvent& event) {
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
        auto   raw   = u8tou32(current_phrase->get_raw().get_feature());
        size_t bytes = 0;
        auto   kana  = raw.begin();
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
            bool success = true;
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
            current_phrase->get_mutable() = u32tou8(raw);

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
bool MikanState::handle_reinterpret_phrases(const fcitx::KeyEvent& event) {
    const static std::vector actions = {Actions::REINTERPRET_NEXT, Actions::REINTERPRET_PREV};
    if(phrases == nullptr || !share.key_config.match(actions, event)) {
        return false;
    }

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
bool MikanState::handle_candidates(const fcitx::KeyEvent& event) {
    const static std::vector actions = {Actions::CANDIDATE_NEXT, Actions::CANDIDATE_PREV, Actions::CANDIDATE_PAGE_NEXT, Actions::CANDIDATE_PAGE_PREV};
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
bool MikanState::handle_move_cursor_phrase(const fcitx::KeyEvent& event) {
    const static std::vector actions = {Actions::PHRASE_NEXT, Actions::PHRASE_PREV};
    if(!share.key_config.match(actions, event)) {
        return false;
    }
    bool    forward = share.key_config.match(Actions::PHRASE_NEXT, event);
    Phrase* current_phrase;
    Phrase* new_phrase;
    size_t  cursor_in_phrase;
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
    merge_branch_sentences();
    return true;
}
bool MikanState::handle_split_phrase(const fcitx::KeyEvent& event) {
    const static std::vector actions = {Actions::SPLIT_PHRASE_LEFT, Actions::SPLIT_PHRASE_RIGHT};
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
    size_t phrase_index = current_phrase - &(*phrases)[0];

    phrases->insert(phrases->begin() + phrase_index + 1, Phrase());
    Phrase *a = &(*phrases)[phrase_index], *b = &(*phrases)[phrase_index + 1];

    // split 'a' into two.
    auto   raw_32    = u8tou32(a->get_raw().get_feature());
    size_t split_pos = share.key_config.match(Actions::SPLIT_PHRASE_LEFT, event) ? 1 : raw_32.size() - 1;
    b->get_mutable() = u32tou8(raw_32.substr(split_pos));
    a->get_mutable() = u32tou8(raw_32.substr(0, split_pos));

    // protect them.
    a->set_protection_level(ProtectionLevel::PRESERVE_SEPARATION);
    b->set_protection_level(ProtectionLevel::PRESERVE_SEPARATION);

    apply_candidates();
    *phrases         = translate_phrases(*phrases, true)[0];
    sentence_changed = true;
    return true;
}
bool MikanState::handle_merge_phrase(const fcitx::KeyEvent& event) {
    const static std::vector actions = {Actions::MERGE_PHRASE_LEFT, Actions::MERGE_PHRASE_RIGHT};
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
    bool    left         = share.key_config.match(Actions::MERGE_PHRASE_LEFT, event);
    Phrase* merge_phrase = current_phrase + (left ? -1 : 1);
    if(merge_phrase < &(*phrases)[0] || merge_phrase > &phrases->back()) {
        return true;
    }

    // merge 'current_phrase' and 'merge_phrase' into 'current_phrase'.
    std::string a = merge_phrase->get_raw().get_feature(), b = current_phrase->get_raw().get_feature();
    current_phrase->get_mutable() = left ? a + b : b + a;
    current_phrase->set_protection_level(ProtectionLevel::PRESERVE_SEPARATION);

    // remove 'merge_phrase'.
    size_t phrase_index = merge_phrase - &(*phrases)[0];
    phrases->erase(phrases->begin() + phrase_index);

    apply_candidates();
    *phrases = translate_phrases(*phrases, true)[0];
    return true;
}
bool MikanState::handle_move_separator(const fcitx::KeyEvent& event) {
    const static std::vector actions = {Actions::MOVE_SEPARATOR_LEFT, Actions::MOVE_SEPARATOR_RIGHT};
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
    size_t moved_chara_size;
    bool   left = share.key_config.match(Actions::MOVE_SEPARATOR_LEFT, event);
    if(left) {
        auto current_u32 = u8tou32(current_phrase->get_raw().get_feature());
        moved_chara_size = fcitx_ucs4_char_len(current_u32.back());
        move_phrase->get_mutable().insert(0, u32tou8(current_u32.back()));
        current_u32.pop_back();
        current_phrase->get_mutable() = u32tou8(current_u32);
    } else {
        auto move_u32    = u8tou32(move_phrase->get_raw().get_feature());
        moved_chara_size = fcitx_ucs4_char_len(move_u32[0]);
        current_phrase->get_mutable() += u32tou8(move_u32[0]);
        move_u32.erase(0, 1);
        move_phrase->get_mutable() = u32tou8(move_u32);
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
bool MikanState::handle_convert_katakana(const fcitx::KeyEvent& event) {
    if(!share.key_config.match(Actions::CONVERT_KATAKANA, event)) {
        return false;
    }

    make_branch_sentence();

    Phrase* current_phrase;
    calc_phrase_in_cursor(&current_phrase);
    if(current_phrase == nullptr) {
        return false;
    }

    std::string                  katakana;
    const std::string&           raw = current_phrase->get_raw().get_feature();
    auto                         u32 = u8tou32(raw);
    std::vector<const HiraKata*> options;
    size_t                       char_num = 0;
    for(auto c = u32.begin(); c != u32.end(); ++c) {
        std::u32string               term(c - char_num, c + 1);
        std::vector<const HiraKata*> filtered;
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
                std::string u8str  = options[0]->katakana;
                auto        u32str = u8tou32(u8str);
                katakana += u32tou8(u32str.substr(0, char_num));
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
        std::lock_guard<std::mutex> lock(share.primary_vocabulary.mutex);
        auto& dic     = share.primary_vocabulary.data;
        auto& lattice = *dic->lattice;
        lattice.set_request_type(MECAB_ONE_BEST);
        lattice.set_sentence(raw.data());
        lattice.set_feature_constraint(0, raw.size(), katakana.data());
        dic->tagger->parse(&lattice);
        bool found = false;
        for(const auto* node = lattice.bos_node(); node; node = node->next) {
            if(node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) {
                continue;
            }
            if(node->stat == MECAB_NOR_NODE) {
                found = true;
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
void MikanState::handle_key_event(fcitx::KeyEvent& event) {
    auto& panel  = context.inputPanel();
    bool  accept = false;
    for(auto h : handlers) {
        if(h(this, event)) {
            accept = true;
            break;
        }
    }
    if(accept) {
        event.filterAndAccept();
        panel.setClientPreedit(build_preedit_text());
        context.updatePreedit();
        context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    } else if(phrases != nullptr) {
        event.filterAndAccept();
    }
    return;
}
void MikanState::handle_activate() {
    context.setCapabilityFlags(context.capabilityFlags() |= fcitx::CapabilityFlag::ClientUnfocusCommit);
}
void MikanState::handle_deactivate() {
    apply_candidates();
    context.inputPanel().reset();
    context.updatePreedit();
    context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    if(phrases != nullptr) {
        commit_all_phrases();
        to_kana.clear();
        reset();
    }
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
MikanState::MikanState(fcitx::InputContext& context, MikanEngine& engine, Share& share) : context(context), engine(engine), share(share) {
}
