#pragma once
#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputpanel.h>

#include "engine.hpp"
#include "misc.hpp"
#include "phrase.hpp"
#include "romaji-index.hpp"
#include "romaji-table.hpp"
#include "share.hpp"

namespace mikan {
class Context final : public fcitx::InputContextProperty {
  private:
    fcitx::InputContext& context;
    Engine&              engine;
    Share&               share;
    Phrases*             phrases = nullptr;
    size_t               cursor;       // in bytes
    RomajiIndex          romaji_index; // used in handle_romaji().
    std::string          to_kana;
    bool                 translation_changed = false;
    SentenceCandidates   sentences;
    bool                 sentence_changed = false;

    static auto is_candidate_list_for(fcitx::InputContext& context, void* ptr) -> bool {
        const auto sp = context.inputPanel().candidateList();
        return sp && !(sp.get() != ptr);
    }

    auto commit_phrase(const Phrase* const phrase) -> void {
        const auto& translated = phrase->get_translated();
        context.commitString(translated.get_feature());
    }

    auto commit_all_phrases() -> void {
        merge_branch_sentences();
        auto current = (Phrase*)(nullptr);
        calc_phrase_in_cursor(&current);
        for(const auto& p : *phrases) {
            commit_phrase(&p);
            if(current == &p && !to_kana.empty()) {
                context.commitString(to_kana);
                to_kana.clear();
            }
        }
    }

    auto make_branch_sentence() -> void {
        if(!translation_changed) {
            shrink_sentences(true);
            translation_changed = true;
        }
    }

    auto merge_branch_sentences() -> bool {
        if(phrases == nullptr) {
            return false;
        }
        if(share.enable_history && sentences.get_index() != 0) {
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

    auto shrink_sentences(const bool reserve_one = false) -> void {
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

    auto delete_surrounding_text() -> bool {
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

    auto calc_phrase_in_cursor(Phrase** phrase, size_t* cursor_in_phrase = nullptr) const -> void {
        if(phrases == nullptr) {
            *phrase = nullptr;
            if(cursor_in_phrase != nullptr) {
                *cursor_in_phrase = 0;
            }
            return;
        }
        auto total_bytes = size_t(0);
        for(const auto& p : *phrases) {
            const auto phrase_end = total_bytes + p.get_raw().get_feature().size();
            if(cursor > total_bytes && cursor <= phrase_end) {
                *phrase = const_cast<Phrase*>(&p);
                if(cursor_in_phrase != nullptr) {
                    *cursor_in_phrase = cursor - total_bytes;
                }
                return;
            }
            total_bytes = phrase_end;
        }
        panic("cursor is not in any phrase.");
    }

    auto move_cursor_back() -> void {
        if(phrases == nullptr) {
            return;
        }
        // move cursor to back.
        cursor = 0;
        for(const auto& p : *phrases) {
            cursor += p.get_raw().get_feature().size();
        }
    }

    auto append_kana(const std::string& kana) -> void {
        // append kana to selected phrase.
        if(phrases == nullptr) {
            sentences.reset({Phrases{Phrase{kana}}});
            phrases = sentences.get_current_ptr();
            cursor  = kana.size();
        } else {
            auto current    = (Phrase*)(nullptr);
            auto cursor_pos = size_t();
            calc_phrase_in_cursor(&current, &cursor_pos);
            if(current->get_protection_level() != ProtectionLevel::None) {
                phrases->emplace_back();
                current    = &phrases->back();
                cursor_pos = 0;
            }
            current->get_mutable_feature().insert(cursor_pos, kana);
            current->set_protection_level(ProtectionLevel::None);
            cursor += kana.size();
            sentence_changed = true;
        }
        *phrases = translate_phrases(*phrases, true)[0];
        auto_commit();
    }

    auto translate_phrases(const Phrases& source, const bool best_only) -> Sentences {
        if(phrases == nullptr) {
            return Sentences();
        }
        return engine.translate_phrases(source, best_only);
    }

    auto build_preedit_text() const -> fcitx::Text {
        auto preedit = fcitx::Text();
        if(phrases == nullptr) {
            preedit.append(to_kana, fcitx::TextFormatFlag::Underline);
            preedit.setCursor(to_kana.size());
            return preedit;
        }

        auto current          = (Phrase*)(nullptr);
        auto cursor_in_phrase = size_t();
        auto preedit_cursor   = size_t(0);
        calc_phrase_in_cursor(&current, &cursor_in_phrase);
        for(auto i = size_t(0), limit = phrases->size(); i < limit; i += 1) {
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
            const auto has_branches    = sentences.get_data_size() >= 2;
            const auto is_current_last = current == &phrases->back();
            const auto insert_space    = share.insert_space == InsertSpaceOptions::On || (share.insert_space == InsertSpaceOptions::Smart && (!is_current_last || has_branches));
            if(&phrase == current && insert_space) {
                text = "[" + text + "]";
            }
            preedit.append(text, format);
            // add space between phrases.
            if(insert_space) {
                auto space = std::string("|");
                preedit.append("|");
                preedit_cursor += space.size();
            }
        }
        return preedit;
    }

    auto apply_candidates() -> void {
        if(!context.inputPanel().candidateList()) {
            return;
        }
        // hide candidate list window
        context.inputPanel().setCandidateList(nullptr);
    }

    auto auto_commit() -> void {
        if(phrases == nullptr || phrases->size() < share.auto_commit_threshold || phrases->size() < 2) {
            return;
        }
        const auto commit_num = phrases->size() - share.auto_commit_threshold + 1;
        auto       on_holds   = size_t(0);
        auto       commited   = false;
        for(auto i = size_t(0); i <= commit_num; i += 1) {
            // we have to ensure that the following phrase's translations will remain the same without this phrase
            if((*phrases)[on_holds].get_protection_level() != ProtectionLevel::PreserveTranslation) {
                auto copy = translate_phrases(std::vector<Phrase>(phrases->begin() + on_holds + 1, phrases->end()), true)[0];
                if(copy[0].get_translated().get_feature() != (*phrases)[on_holds + 1].get_translated().get_feature()) {
                    // translation result will be changed
                    // but maybe we can commit this phrase with next one
                    on_holds += 1;
                    continue;
                }
            }

            // commit phrases
            for(auto i = size_t(0); i <= on_holds; i += 1) {
                commit_phrase(&(*phrases)[0]);
                const auto commited_bytes = (*phrases)[0].get_raw().get_feature().size();
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

    auto reset() -> void {
        sentences.clear();
        phrases          = nullptr;
        sentence_changed = true;
    }

  public:
    auto handle_key_event(fcitx::KeyEvent& event) -> void {
        auto& panel  = context.inputPanel();
        auto  accept = true;
        // handle candidates
        do {
            constexpr auto actions = std::array{Actions::CandidateNext, Actions::CandidatePrev, Actions::CandidatePageNext, Actions::CandidatePagePrev};
            if(!share.key_config.match(actions, event)) {
                break;
            }

            if(!context.inputPanel().candidateList() && !share.key_config.match(Actions::CandidateNext, event)) {
                break;
            }

            auto current_phrase = (Phrase*)(nullptr);
            calc_phrase_in_cursor(&current_phrase, nullptr);
            if(current_phrase == nullptr) {
                break;
            }

            make_branch_sentence();

            // get candidate list
            if(!current_phrase->is_candidates_initialized()) {
                calc_phrase_in_cursor(&current_phrase, nullptr);
                auto [lock, dic] = share.primary_vocabulary.access();

                auto dics = std::vector<MeCabModel*>{dic};
                std::copy(share.additional_vocabularies.begin(), share.additional_vocabularies.end(), std::back_inserter(dics));
                current_phrase->reset_candidates(PhraseCandidates(dics, current_phrase->get_candidates()));
                current_phrase->set_protection_level(ProtectionLevel::PreserveTranslation);
            }
            if(!current_phrase->has_candidates()) {
                break;
            }

            if(!is_candidate_list_for(context, current_phrase)) {
                context.inputPanel().setCandidateList(std::make_unique<CandidateList>(&current_phrase->get_candidates(), share.candidate_page_size));
            }
            auto candidate_list = context.inputPanel().candidateList().get();

            // move candidate list index
            if(share.key_config.match(Actions::CandidateNext, event)) {
                candidate_list->toCursorMovable()->nextCandidate();
            } else if(share.key_config.match(Actions::CandidatePrev, event)) {
                candidate_list->toCursorMovable()->prevCandidate();
            } else if(share.key_config.match(Actions::CandidatePageNext, event)) {
                candidate_list->toPageable()->prev();
            } else if(share.key_config.match(Actions::CandidatePagePrev, event)) {
                candidate_list->toPageable()->next();
            }
            goto end;
        } while(0);

        // handle delete
        do {
            if(!share.key_config.match(Actions::Backspace, event)) {
                break;
            }
            if(!to_kana.empty()) {
                pop_back_u8(to_kana);
                goto end;
            }
            move_cursor_back();
            auto current_phrase   = (Phrase*)(nullptr);
            auto cursor_in_phrase = size_t();
            calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
            if(current_phrase == nullptr) {
                break;
            }
            auto raw   = u8tou32(current_phrase->get_raw().get_feature());
            auto bytes = size_t(0);
            auto kana  = raw.begin();
            for(; kana != raw.end(); kana += 1) {
                bytes += fcitx_ucs4_char_len(*kana);
                if(bytes == cursor_in_phrase) {
                    break;
                }
            }
            if(kana == raw.end()) {
                break;
            }

            // disassembly the kana to romaji and pop back.
            auto kana8 = std::array<char, FCITX_UTF8_MAX_LENGTH>();
            fcitx_ucs4_to_utf8(*kana, kana8.data());
            auto success = true;
            try {
                to_kana = kana_to_romaji(kana8.data());
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
            current_phrase->set_protection_level(ProtectionLevel::None);
            *phrases = translate_phrases(*phrases, true)[0];

            // phrases may be empty.
            if(phrases->empty()) {
                reset();
            }

            apply_candidates();
            goto end;
        } while(0);

        // handle reinterpret phrases
        do {
            constexpr auto actions = std::array{Actions::ReinterpretNext, Actions::ReinterpretPrev};
            if(phrases == nullptr || !share.key_config.match(actions, event)) {
                break;
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
            if(share.key_config.match(Actions::ReinterpretNext, event)) {
                candidate_list->toCursorMovable()->nextCandidate();
            } else {
                candidate_list->toCursorMovable()->prevCandidate();
            }
            phrases = sentences.get_current_ptr();
            for(auto& p : *phrases) {
                p.set_protection_level(ProtectionLevel::PreserveTranslation);
            }
            goto end;
        } while(0);

        // handle commit phrases
        do {
            if(!share.key_config.match(Actions::Commit, event)) {
                break;
            }
            if(phrases == nullptr) {
                if(to_kana.empty()) {
                    break;
                }
                context.commitString(to_kana);
                to_kana.clear();
                goto end;
            }
            commit_all_phrases();
            reset();
            apply_candidates();
            goto end;
        } while(0);

        // handle move cursor phrase
        do {
            constexpr auto actions = std::array{Actions::PhraseNext, Actions::PhrasePrev};
            if(!share.key_config.match(actions, event)) {
                break;
            }
            const auto forward          = share.key_config.match(Actions::PhraseNext, event);
            auto       current_phrase   = (Phrase*)(nullptr);
            auto       new_phrase       = (Phrase*)(nullptr);
            auto       cursor_in_phrase = size_t();
            calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
            if(current_phrase == nullptr) {
                break;
            }
            new_phrase = forward ? current_phrase - 1 : current_phrase + 1;
            if(new_phrase < &(*phrases)[0] || new_phrase > &phrases->back()) {
                break;
            }

            // move cursor
            to_kana.clear();
            cursor = 0;
            for(auto p = &(*phrases)[0]; p <= new_phrase; p += 1) {
                cursor += p->get_raw().get_feature().size();
            }
            apply_candidates();
            goto end;
        } while(0);

        // handle split phrase
        do {
            constexpr auto actions = std::array{Actions::SplitPhraseLeft, Actions::SplitPhraseRight};
            if(!share.key_config.match(actions, event)) {
                break;
            }

            make_branch_sentence();

            auto current_phrase   = (Phrase*)(nullptr);
            auto cursor_in_phrase = size_t();
            calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
            if(current_phrase == nullptr || fcitx_utf8_strlen(current_phrase->get_raw().get_feature().data()) <= 1) {
                break;
            }

            // insert a new phrase right after the selected phrase
            const auto phrase_index = current_phrase - &(*phrases)[0];

            phrases->insert(phrases->begin() + phrase_index + 1, Phrase());
            const auto a = &(*phrases)[phrase_index], b = &(*phrases)[phrase_index + 1];

            // split 'a' into two
            const auto raw_32        = u8tou32(a->get_raw().get_feature());
            const auto split_pos     = share.key_config.match(Actions::SplitPhraseLeft, event) ? 1 : raw_32.size() - 1;
            b->get_mutable_feature() = u32tou8(raw_32.substr(split_pos));
            a->get_mutable_feature() = u32tou8(raw_32.substr(0, split_pos));

            // protect them
            a->set_protection_level(ProtectionLevel::PreserveSeparation);
            b->set_protection_level(ProtectionLevel::PreserveSeparation);

            apply_candidates();
            *phrases         = translate_phrases(*phrases, true)[0];
            sentence_changed = true;
            goto end;
        } while(0);

        // handle merge phrase
        do {
            constexpr auto actions = std::array{Actions::MergePhraseLeft, Actions::MergePhraseRight};
            if(!share.key_config.match(actions, event)) {
                break;
            }

            make_branch_sentence();

            auto current_phrase   = (Phrase*)(nullptr);
            auto cursor_in_phrase = size_t();
            calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
            if(current_phrase == nullptr) {
                goto end;
            }
            const auto left         = share.key_config.match(Actions::MergePhraseLeft, event);
            const auto merge_phrase = current_phrase + (left ? -1 : 1);
            if(merge_phrase < &(*phrases)[0] || merge_phrase > &phrases->back()) {
                goto end;
            }

            // merge 'current_phrase' and 'merge_phrase' into 'current_phrase'
            const auto a = merge_phrase->get_raw().get_feature(), b = current_phrase->get_raw().get_feature();
            current_phrase->get_mutable_feature() = left ? a + b : b + a;
            current_phrase->set_protection_level(ProtectionLevel::PreserveSeparation);

            // remove 'merge_phrase'
            const auto phrase_index = merge_phrase - &(*phrases)[0];
            phrases->erase(phrases->begin() + phrase_index);

            apply_candidates();
            *phrases = translate_phrases(*phrases, true)[0];
            goto end;
        } while(0);

        // handle move separator
        do {
            constexpr auto actions = std::array{Actions::MoveSeparatorLeft, Actions::MoveSeparatorRight};
            if(!share.key_config.match(actions, event)) {
                break;
            }

            make_branch_sentence();

            auto current_phrase   = (Phrase*)(nullptr);
            auto cursor_in_phrase = size_t();
            calc_phrase_in_cursor(&current_phrase, &cursor_in_phrase);
            if(current_phrase == nullptr) {
                goto end;
            }
            const auto move_phrase = current_phrase + 1;
            if(move_phrase > &phrases->back()) {
                goto end;
            }

            // align cursor to phrase back
            cursor += current_phrase->get_raw().get_feature().size() - cursor_in_phrase;

            // move cursor and separator
            auto       moved_chara_size = size_t();
            const auto left             = share.key_config.match(Actions::MoveSeparatorLeft, event);
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

            current_phrase->set_protection_level(ProtectionLevel::PreserveSeparation);
            move_phrase->set_protection_level(ProtectionLevel::PreserveSeparation);
            apply_candidates();
            *phrases         = translate_phrases(*phrases, true)[0];
            sentence_changed = true;
            goto end;
        } while(0);

        // handle convert katakana
        do {
            if(!share.key_config.match(Actions::ConvertKatakana, event)) {
                break;
            }

            make_branch_sentence();

            auto current_phrase = (Phrase*)(nullptr);
            calc_phrase_in_cursor(&current_phrase);
            if(current_phrase == nullptr) {
                break;
            }

            auto        katakana32 = std::u32string();
            const auto& raw        = current_phrase->get_raw().get_feature();
            auto        u32        = u8tou32(raw);
            for(const auto c : u8tou32(raw)) {
                if(const auto p = hiragana_katakana_table.find(c); p != hiragana_katakana_table.end()) {
                    katakana32 += p->second;
                } else {
                    katakana32 += c;
                }
            }
            const auto katakana = u32tou8(katakana32);
            {
                // try to get word information(if exists in dictionary)
                auto [lock, dic] = share.primary_vocabulary.access();
                auto& lattice    = *dic->lattice;
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
            current_phrase->set_protection_level(ProtectionLevel::PreserveTranslation);
            goto end;
        } while(0);

        // handle romaji
        do {
            if(event.isRelease()) {
                break;
            }
            const auto romaji_8 = fcitx::Key::keySymToUTF8(event.key().sym());
            if(fcitx_utf8_strlen(romaji_8.data()) != 1) {
                break;
            }
            merge_branch_sentences();
            move_cursor_back();

            to_kana += romaji_8;
            auto filter_result = romaji_index.filter(to_kana);
            if(filter_result.index() == filter_result.index_of<RomajiIndex::EmptyCache>()) {
                to_kana       = romaji_8;
                filter_result = romaji_index.filter(to_kana);
                if(filter_result.index() == filter_result.index_of<RomajiIndex::EmptyCache>()) {
                    to_kana.clear();
                    break;
                }
            }

            apply_candidates();
            delete_surrounding_text();
            if(filter_result.index() == filter_result.index_of<RomajiIndex::ExactOne>()) {
                const auto exact = filter_result.get<RomajiIndex::ExactOne>().result;
                append_kana(exact->kana);
                if(exact->refill != nullptr) {
                    to_kana = exact->refill;
                } else {
                    to_kana.clear();
                }
            }
            goto end;
        } while(0);

        accept = false;

    end:
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

    auto handle_activate() -> void {
        context.setCapabilityFlags(context.capabilityFlags() |= fcitx::CapabilityFlag::ClientUnfocusCommit);
    }

    auto handle_deactivate() -> void {
        apply_candidates();
        context.inputPanel().reset();
        context.updatePreedit();
        context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
        if(phrases != nullptr) {
            commit_all_phrases();
            reset();
        }
        if(!to_kana.empty()) {
            context.commitString(to_kana);
            to_kana.clear();
        }
    }

    Context(fcitx::InputContext& context, Engine& engine, Share& share) : context(context), engine(engine), share(share) {}
};
} // namespace mikan
