#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/cutf8.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/inputpanel.h>
#include <fcitx/surroundingtext.h>
#include <mecab.h>

#include "candidate.hpp"
#include "configuration.hpp"
#include "mecab-model.hpp"
#include "mikan5.hpp"
#include "misc.hpp"
#include "romaji-table.hpp"
#include "state.hpp"
#include "type.hpp"

bool MikanState::is_candidate_list_active(Phrase* phrase) {
    auto sp = context.inputPanel().candidateList();
    if(!sp) {
        return false;
    }
    if(phrase == nullptr) {
        return true;
    }
    return !dynamic_cast<CandidateList*>(sp.get())->is_phrase(phrase);
}
void MikanState::commit_phrase(const Phrase* phrase) {
    context.commitString(phrase->translation);
    if(phrase->protection == Phrase::ProtectionLevel::PRESERVE_TRANSLATION) {
        engine.request_update_dictionary(phrase->raw.data(), phrase->translation.data());
    }
}
bool MikanState::delete_surrounding_text() {
    if(context.capabilityFlags().test(fcitx::CapabilityFlag::SurroundingText)) {
        const auto& text = context.surroundingText();
        if(!text.isValid()) {
            return false;
        }
        context.deleteSurroundingText(0, text.anchor() - text.cursor());
        return true;
    } else {
        return false;
    }
}
void MikanState::calc_phrase_in_cursor(Phrase** phrase, size_t* cursor_in_phrase) const {
    if(!cursor) {
        *phrase = nullptr;
        if(cursor_in_phrase != nullptr) {
            *cursor_in_phrase = 0;
        }
        return;
    }
    size_t total_bytes = 0;
    for(const auto& p : phrases) {
        size_t phrase_end = total_bytes + p.raw.size();
        if(cursor.value() > total_bytes && cursor.value() <= phrase_end) {
            *phrase = const_cast<Phrase*>(&p);
            if(cursor_in_phrase != nullptr) {
                *cursor_in_phrase = cursor.value() - total_bytes;
            }
            return;
        }
        total_bytes = phrase_end;
    }
    throw std::runtime_error("cursor is not in any phrase.");
    return;
}
void MikanState::move_cursor_back() {
    if(!cursor) {
        return;
    }
    // move cursor to back.
    cursor = 0;
    for(const auto& p : phrases) {
        cursor.value() += p.raw.size();
    }
}
void MikanState::append_kana(const std::string& kana) {
    // append kana to selected phrase.
    if(!cursor) {
        phrases.emplace_back(Phrase{kana});
        cursor = kana.size();
    } else {
        Phrase* current;
        size_t  cursor_pos;
        calc_phrase_in_cursor(&current, &cursor_pos);
        if(current->protection != Phrase::ProtectionLevel::NONE) {
            phrases.emplace_back();
            current    = &phrases.back();
            cursor_pos = 0;
        }
        current->raw.insert(cursor_pos, kana);
        current->protection = Phrase::ProtectionLevel::NONE;
        cursor.value() += kana.size();
    }
    translate_phrases();
    auto_commit();
}
void MikanState::translate_phrases(std::vector<Phrase>* target) {
    if(target == nullptr) {
        target = &phrases;
    }
    // translate.
    struct FeatureConstriant {
        size_t  begin;
        size_t  end;
        Phrase* phrase;
    };
    std::vector<Phrase>            nodes;
    std::vector<FeatureConstriant> feature_constriants;
    std::string                    buffer;
    for(auto p = target->begin(); p != target->end(); ++p) {
        if(p->protection != Phrase::ProtectionLevel::NONE) {
            size_t begin = buffer.size();
            size_t end   = begin + p->raw.size();
            feature_constriants.emplace_back(FeatureConstriant{begin, end, &*p});
        };
        buffer += p->raw;
    }
    {
        std::lock_guard<std::mutex> lock(share.primary_vocabulary.mutex);
        share.primary_vocabulary.data->lattice->set_sentence(buffer.data());
        for(const auto& f : feature_constriants) {
            share.primary_vocabulary.data->lattice->set_feature_constraint(f.begin, f.end, f.phrase->translation.data());
        }
        share.primary_vocabulary.data->tagger->parse(share.primary_vocabulary.data->lattice);
        for(const auto* node = share.primary_vocabulary.data->lattice->bos_node(); node; node = node->next) {
            if(node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) {
                continue;
            }
            nodes.emplace_back(Phrase{std::string(node->surface, node->length), node->feature});
            if(node->stat == MECAB_UNK_NODE) {
                nodes.back().translation = nodes.back().raw;
            }
        }
        share.primary_vocabulary.data->lattice->clear();
    }

    // retrive protected phrases.
    size_t total_bytes          = 0;
    auto   protected_phrase_pos = feature_constriants.begin();
    for(auto& n : nodes) {
        if(protected_phrase_pos == feature_constriants.end()) {
            break;
        }
        if(total_bytes == protected_phrase_pos->begin) {
            // this is a phrase from a protected one.
            n.protection = protected_phrase_pos->phrase->protection;
            if(n.protection == Phrase::ProtectionLevel::PRESERVE_TRANSLATION) {
                n.translation = std::move(protected_phrase_pos->phrase->translation);
                n.candidates  = std::move(protected_phrase_pos->phrase->candidates);
            }
            protected_phrase_pos += 1;
        } else if(total_bytes > protected_phrase_pos->begin) {
            throw std::runtime_error("protected phrase lost.");
        }
        total_bytes += n.raw.size();
    }
    *target = nodes;
}
fcitx::Text MikanState::build_preedit_text() const {
    fcitx::Text preedit;
    if(phrases.empty()) {
        std::string text = to_kana;
        preedit.append(to_kana, fcitx::TextFormatFlag::Underline);
        preedit.setCursor(to_kana.size());
        return preedit;
    }

    Phrase* current;
    size_t  cursor_in_phrase;
    size_t  preedit_cursor = 0;
    calc_phrase_in_cursor(&current, &cursor_in_phrase);
    for(size_t i = 0, limit = phrases.size(); i < limit; ++i) {
        const auto& phrase = phrases[i];
        auto        text   = std::string();
        auto        format = fcitx::TextFormatFlag::NoFlag;
        if(&phrase == current) {
            if(!to_kana.empty()) {
                format = fcitx::TextFormatFlag::Underline;
                text   = phrase.raw;
                text.insert(cursor_in_phrase, to_kana);
                preedit_cursor += cursor_in_phrase + to_kana.size();
            } else {
                format = fcitx::TextFormatFlag::Bold;
                text   = phrase.translation;
                preedit_cursor += text.size();
            }
            preedit.setCursor(preedit_cursor);
        } else {
            format = fcitx::TextFormatFlag::Underline;
            text   = phrase.translation;
            preedit_cursor += text.size();
        }
        preedit.append(text, format);
        // add space between phrases.
        const static std::string space      = " ";
        std::string              space_copy = space;
        preedit.append(space_copy);
        preedit_cursor += space.size();
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
    if(phrases.size() < share.auto_commit_threshold || phrases.size() < 2) {
        return;
    }
    size_t commit_num = phrases.size() - share.auto_commit_threshold + 1;
    size_t on_holds   = 0;
    bool   commited   = false;
    for(size_t i = 0; i <= commit_num; ++i) {
        // we have to ensure that the following phrases' translations will be the same without this phrase.
        if(phrases[on_holds].protection != Phrase::ProtectionLevel::PRESERVE_TRANSLATION) {
            std::vector<Phrase> copy(phrases.begin() + on_holds + 1, phrases.end());
            translate_phrases(&copy);
            if(copy[0].translation != phrases[on_holds + 1].translation) {
                // translation result will be changed.
                // but maybe we can commit this phrase with next one.
                on_holds += 1;
                continue;
            }
        }

        // commit phrases.
        for(size_t i = 0; i <= on_holds; ++i) {
            commit_phrase(&phrases[0]);
            size_t commited_bytes = phrases[0].raw.size();
            if(cursor.value() >= commited_bytes) {
                cursor.value() -= commited_bytes;
            } else {
                cursor.value() = 0;
            }
            phrases.erase(phrases.begin());
        }
        on_holds = 0;
        commited = true;
    }
    if(commited) {
        apply_candidates();
    }
}
bool MikanState::handle_romaji(const fcitx::KeyEvent& event) {
    if(event.isRelease()) {
        return false;
    }
    std::string romaji_8 = fcitx::Key::keySymToUTF8(event.key().sym());
    if(fcitx_utf8_strlen(romaji_8.data()) != 1) {
        return false;
    }
    if(move_back_on_input) {
        move_cursor_back();
        move_back_on_input = false;
    }
    char32_t    romaji     = fcitx_utf8_get_char_validated(romaji_8.data(), romaji_8.size(), nullptr);
    size_t      insert_pos = fcitx_utf8_strlen(to_kana.data());
    const char* match      = nullptr;
    const char* refill     = nullptr;
    bool        append     = false;
    for(uint64_t i = 0; i < romaji_table_limit; ++i) {
        if(romaji_table[i].romaji.size() <= insert_pos) {
            continue;
        }
        if(romaji_table[i].romaji[insert_pos] == romaji) {
            append = true;
            if(romaji_table[i].romaji[insert_pos + 1] == U'\0' && u8tou32(to_kana + romaji_8) == romaji_table[i].romaji) {
                match  = romaji_table[i].kana;
                refill = romaji_table[i].refill;
                break;
            }
        };
    }
    if(!append) {
        // no kana matched with current to_kana. try once more without it.
        for(uint64_t i = 0; i < romaji_table_limit; ++i) {
            if(romaji_table[i].romaji[0] == romaji) {
                append = true;
                if(romaji_table[i].romaji[1] == U'\0') {
                    match  = romaji_table[i].kana;
                    refill = romaji_table[i].refill;
                    break;
                }
            }
        }
        if(append) {
            to_kana.clear();
        }
    }
    if(append) {
        apply_candidates();
        delete_surrounding_text();
        if(match != nullptr) {
            append_kana(match);
            if(refill != nullptr) {
                to_kana = refill;
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
}
bool MikanState::handle_commit_phrases(const fcitx::KeyEvent& event) {
    if(!share.key_config.match(Actions::COMMIT, event) || !cursor) {
        return false;
    }
    for(const auto& p : phrases) {
        commit_phrase(&p);
    }
    apply_candidates();
    phrases.clear();
    cursor.reset();
    if(!to_kana.empty()) {
        context.commitString(to_kana);
        to_kana.clear();
    }
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
        auto   raw   = u8tou32(current_phrase->raw);
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
            to_kana = kana_to_romaji(kana8);
            pop_back_u8(to_kana);

            // move cursor.
            cursor.value() -= fcitx_ucs4_char_len(*kana);

            // delete character from the phrase.
            raw.erase(kana);
            current_phrase->raw = u32tou8(raw);

            // translate.
            current_phrase->protection = Phrase::ProtectionLevel::NONE;
            translate_phrases();

            // phrases may be empty.
            if(phrases.empty()) {
                cursor.reset();
            }

            apply_candidates();
            return true;
        }
    }
    return false;
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

    // make the next input to be inserted at the last phrase.
    move_back_on_input = true;

    // get candidate list.
    auto& phrase     = *current_phrase;
    auto& candidates = phrase.candidates;
    if(!candidates.is_initialized()) {
        std::lock_guard<std::mutex> lock(share.primary_vocabulary.mutex);
        std::vector<MeCabModel*> dic = {share.primary_vocabulary.data};
        std::copy(share.additional_vocabularies.begin(), share.additional_vocabularies.end(), std::back_inserter(dic));
        candidates = Candidates(dic, phrase.raw, phrase.translation);
    }
    if(!candidates.has_candidate()) {
        return true;
    }

    if(!is_candidate_list_active(&phrase)) {
        context.inputPanel().setCandidateList(std::make_unique<CandidateList>(&phrase, share.candidate_page_size));
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

    phrase.translation = candidates.get_current();
    phrase.protection  = Phrase::ProtectionLevel::PRESERVE_TRANSLATION;
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
    if(new_phrase < &phrases[0] || new_phrase > &phrases.back()) {
        return false;
    }
    // move cursor.
    //new_phrase->raw.append(to_kana);
    //new_phrase->translation.append(to_kana);
    to_kana.clear();
    cursor = 0;
    for(auto p = &phrases[0]; p <= new_phrase; ++p) {
        cursor.value() += p->raw.size();
    }
    apply_candidates();
    move_back_on_input = false;
    return true;
}
bool MikanState::handle_split_phrase(const fcitx::KeyEvent& event) {
    const static std::vector actions = {Actions::SPLIT_PHRASE_LEFT, Actions::SPLIT_PHRASE_RIGHT};
    if(!share.key_config.match(actions, event)) {
        return false;
    }

    move_back_on_input = true;

    Phrase* current_phrase;
    size_t  cursor_in_phrase;
    calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
    if(current_phrase == nullptr || fcitx_utf8_strlen(current_phrase->raw.data()) <= 1) {
        return false;
    }
    // insert a new phrase right after the selected phrase.
    size_t phrase_index = current_phrase - &phrases[0];
    phrases.insert(phrases.begin() + phrase_index + 1, Phrase());
    Phrase *a = &phrases[phrase_index], *b = &phrases[phrase_index + 1];

    // split 'a' into two.
    auto   raw_32    = u8tou32(a->raw);
    size_t split_pos = share.key_config.match(Actions::SPLIT_PHRASE_LEFT, event) ? 1 : raw_32.size() - 1;
    b->raw           = u32tou8(raw_32.substr(split_pos));
    a->raw           = u32tou8(raw_32.substr(0, split_pos));

    // protect them.
    a->protection = b->protection = Phrase::ProtectionLevel::PRESERVE_SEPARATION;

    apply_candidates();
    translate_phrases();
    return true;
}
bool MikanState::handle_merge_phrase(const fcitx::KeyEvent& event) {
    const static std::vector actions = {Actions::MERGE_PHRASE_LEFT, Actions::MERGE_PHRASE_RIGHT};
    if(!share.key_config.match(actions, event)) {
        return false;
    }

    move_back_on_input = true;

    Phrase* current_phrase;
    size_t  cursor_in_phrase;
    calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
    if(current_phrase == nullptr) {
        return true;
    }
    bool    left         = share.key_config.match(Actions::MERGE_PHRASE_LEFT, event);
    Phrase* merge_phrase = current_phrase + (left ? -1 : 1);
    if(merge_phrase < &phrases[0] || merge_phrase > &phrases.back()) {
        return true;
    }

    // merge 'current_phrase' and 'merge_phrase' into 'current_phrase'.
    current_phrase->raw        = left ? merge_phrase->raw + current_phrase->raw : current_phrase->raw + merge_phrase->raw;
    current_phrase->protection = Phrase::ProtectionLevel::PRESERVE_SEPARATION;

    // remove 'merge_phrase'.
    size_t phrase_index = merge_phrase - &phrases[0];
    phrases.erase(phrases.begin() + phrase_index);

    apply_candidates();
    translate_phrases();
    return true;
}
bool MikanState::handle_move_separator(const fcitx::KeyEvent& event) {
    const static std::vector actions = {Actions::MOVE_SEPARATOR_LEFT, Actions::MOVE_SEPARATOR_RIGHT};
    if(!share.key_config.match(actions, event)) {
        return false;
    }

    move_back_on_input = true;

    Phrase* current_phrase;
    size_t  cursor_in_phrase;
    calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
    if(current_phrase == nullptr) {
        return true;
    }
    Phrase* move_phrase = current_phrase + 1;
    if(move_phrase > &phrases.back()) {
        return true;
    }

    // align cursor to phrase back.
    cursor.value() += current_phrase->raw.size() - cursor_in_phrase;

    // move cursor and separator.
    size_t moved_chara_size;
    bool   left = share.key_config.match(Actions::MOVE_SEPARATOR_LEFT, event);
    if(left) {
        auto current_u32 = u8tou32(current_phrase->raw);
        moved_chara_size = fcitx_ucs4_char_len(current_u32.back());
        move_phrase->raw.insert(0, u32tou8(current_u32.back()));
        current_u32.pop_back();
        current_phrase->raw = u32tou8(current_u32);
    } else {
        auto move_u32    = u8tou32(move_phrase->raw);
        moved_chara_size = fcitx_ucs4_char_len(move_u32[0]);
        current_phrase->raw += u32tou8(move_u32[0]);
        move_u32.erase(0, 1);
        move_phrase->raw = u32tou8(move_u32);
    }
    if(left) {
        cursor.value() -= moved_chara_size;
    } else {
        cursor.value() += moved_chara_size;
    }

    current_phrase->protection = move_phrase->protection = Phrase::ProtectionLevel::PRESERVE_SEPARATION;
    apply_candidates();
    translate_phrases();
    return true;
}
bool MikanState::handle_convert_katakana(const fcitx::KeyEvent& event) {
    if(!share.key_config.match(Actions::CONVERT_KATAKANA, event)) {
        return false;
    }

    move_back_on_input = true;

    Phrase* current_phrase;
    calc_phrase_in_cursor(&current_phrase);
    if(current_phrase == nullptr) {
        return false;
    }

    current_phrase->translation.clear();
    for(auto c : u8tou32(current_phrase->raw)) {
        const char* kata = nullptr;
        for(size_t i = 0; i < hiragana_katakana_table_limit; ++i) {
            const auto& hirakata = hiragana_katakana_table[i];
            if(hirakata.hiragana == c) {
                kata = hirakata.katakana;
                break;
            }
        }
        if(kata == nullptr) {
            current_phrase->translation += u32tou8(c);
        } else {
            current_phrase->translation += kata;
        }
    }

    apply_candidates();
    current_phrase->protection = Phrase::ProtectionLevel::PRESERVE_TRANSLATION;
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
    } else if(!phrases.empty()) {
        event.filterAndAccept();
    }
    return;
}
void MikanState::handle_activate() {
}
void MikanState::handle_deactivate() {
    context.inputPanel().reset();
    context.updatePreedit();
    context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    apply_candidates();
    for(const auto& p : phrases) {
        commit_phrase(&p);
    }
    phrases.clear();
    to_kana.clear();
    cursor.reset();
}
const std::vector<std::function<bool(MikanState*, const fcitx::KeyEvent&)>> MikanState::handlers = {
    &MikanState::handle_candidates,
    &MikanState::handle_delete,
    &MikanState::handle_commit_phrases,
    &MikanState::handle_move_cursor_phrase,
    &MikanState::handle_split_phrase,
    &MikanState::handle_merge_phrase,
    &MikanState::handle_move_separator,
    &MikanState::handle_convert_katakana,
    &MikanState::handle_romaji,
};
MikanState::MikanState(fcitx::InputContext& context, MikanEngine& engine, Share& share) : context(context), engine(engine), share(share) {
    context.setCapabilityFlags(context.capabilityFlags() |= fcitx::CapabilityFlag::ClientUnfocusCommit);
}
