#include "context.hpp"
#include "candidate-list.hpp"
#include "macros/assert.hpp"
#include "misc.hpp"
#include "romaji-table.hpp"

namespace mikan {
namespace {
auto get_candidate_list_base(fcitx::InputContext& context) -> CandidateListBase* {
    const auto sp = context.inputPanel().candidateList();
    if(!sp) {
        return nullptr;
    }

    return downcast<CandidateListBase>(sp.get());
}

auto is_candidate_list_for(fcitx::InputContext& context, const Candidates* const candidates) -> bool {
    const auto base = get_candidate_list_base(context);
    if(base == nullptr) {
        return false;
    }
    return base->get_candidates() == candidates;
}
} // namespace

auto Context::commit_word(const Word* const word) -> void {
    const auto& translated = word->get_translated();
    context.commitString(translated.get_feature());
}

auto Context::commit_wordchain() -> void {
    merge_branch_chains();
    auto current = (Word*)(nullptr);
    calc_word_in_cursor(&current);
    for(const auto& p : *chain) {
        commit_word(&p);
        if(current == &p && !to_kana.empty()) {
            context.commitString(to_kana);
            to_kana.clear();
        }
    }
}

auto Context::make_branch_chain() -> void {
    if(!translation_changed) {
        shrink_chains(true);
        translation_changed = true;
    }
}

auto Context::merge_branch_chains() -> bool {
    if(chain == nullptr) {
        return false;
    }
    if(chains.get_index() != 0) {
        // if chains.index != 0, it means user fixed translation manually.
        // so we have to check the differences between chains[0] and *chain.
        engine.request_fix_dictionary(*chain);
    }
    if(translation_changed) {
        shrink_chains();
        translation_changed = false;
        return true;
    } else {
        return false;
    }
}

auto Context::shrink_chains(const bool reserve_one) -> void {
    if(chain == nullptr) {
        return;
    }
    if(reserve_one) {
        chains.reset({*chain, *chain});
        chains.set_index(1);
    } else {
        chains.reset({*chain});
    }
    chain = chains.get_current_ptr();
}

auto Context::delete_surrounding_text() -> bool {
    if(chain != nullptr || !to_kana.empty() || !context.capabilityFlags().test(fcitx::CapabilityFlag::SurroundingText)) {
        return false;
    }
    const auto& text = context.surroundingText();
    if(!text.isValid()) {
        return false;
    }
    context.deleteSurroundingText(0, text.anchor() - text.cursor());
    return true;
}

auto Context::calc_word_in_cursor(Word** word, size_t* cursor_in_word) const -> void {
    if(chain == nullptr) {
        *word = nullptr;
        if(cursor_in_word != nullptr) {
            *cursor_in_word = 0;
        }
        return;
    }
    auto total_bytes = size_t(0);
    for(auto& p : *chain) {
        const auto word_end = total_bytes + p.get_raw().get_feature().size();
        if(cursor > total_bytes && cursor <= word_end) {
            *word = const_cast<Word*>(&p);
            if(cursor_in_word != nullptr) {
                *cursor_in_word = cursor - total_bytes;
            }
            return;
        }
        total_bytes = word_end;
    }
    PANIC("cursor is not in any word");
}

auto Context::move_cursor_back() -> void {
    if(chain == nullptr) {
        return;
    }
    // move cursor to back.
    cursor = 0;
    for(const auto& p : *chain) {
        cursor += p.get_raw().get_feature().size();
    }
}

auto Context::append_kana(const std::string& kana) -> void {
    // append kana to selected word
    if(chain == nullptr) {
        chains.reset({WordChain{Word{kana}}});
        chain  = chains.get_current_ptr();
        cursor = kana.size();
    } else {
        auto current    = (Word*)(nullptr);
        auto cursor_pos = size_t();
        calc_word_in_cursor(&current, &cursor_pos);
        if(current->get_protection_level() != ProtectionLevel::None) {
            chain->emplace_back();
            current    = &chain->back();
            cursor_pos = 0;
        }
        current->get_mutable_feature().insert(cursor_pos, kana);
        current->set_protection_level(ProtectionLevel::None);
        cursor += kana.size();
        chain_changed = true;
    }
    *chain = translate_wordchain(*chain, true)[0];
    auto_commit();
}

auto Context::translate_wordchain(const WordChain& source, const bool best_only) -> WordChains {
    if(chain == nullptr) {
        return WordChains();
    }
    return engine.translate_wordchain(source, best_only);
}

auto Context::build_preedit_text() const -> fcitx::Text {
    auto preedit = fcitx::Text();
    if(chain == nullptr) {
        preedit.append(to_kana, fcitx::TextFormatFlag::Underline);
        preedit.setCursor(to_kana.size());
        return preedit;
    }

    auto current        = (Word*)(nullptr);
    auto cursor_in_word = size_t();
    auto preedit_cursor = size_t(0);
    calc_word_in_cursor(&current, &cursor_in_word);
    for(auto i = size_t(0), limit = chain->size(); i < limit; i += 1) {
        const auto& word   = (*chain)[i];
        auto        text   = std::string();
        auto        format = fcitx::TextFormatFlag::NoFlag;
        if(&word == current) {
            if(!to_kana.empty()) {
                format = fcitx::TextFormatFlag::Underline;
                text   = word.get_raw().get_feature();
                text.insert(cursor_in_word, to_kana);
                preedit_cursor += cursor_in_word + to_kana.size();
            } else {
                format = fcitx::TextFormatFlag::Bold;
                text   = word.get_translated().get_feature();
                preedit_cursor += text.size();
            }
            preedit.setCursor(preedit_cursor);
        } else {
            format = fcitx::TextFormatFlag::Underline;
            text   = word.get_translated().get_feature();
            preedit_cursor += text.size();
        }
        const auto has_branches    = chains.get_data_size() >= 2;
        const auto is_current_last = current == &chain->back();
        const auto insert_space    = share.insert_space == InsertSpaceOptions::On || (share.insert_space == InsertSpaceOptions::Smart && (!is_current_last || has_branches));
        if(&word == current && insert_space) {
            text = "[" + text + "]";
        }
        preedit.append(text, format);
        // add space between words
        if(insert_space) {
            auto space = std::string("|");
            preedit.append("|");
            preedit_cursor += space.size();
        }
    }
    return preedit;
}

auto Context::build_kana_text() const -> std::string {
    if(chain == nullptr) {
        return to_kana;
    }

    auto text    = std::string();
    auto current = (Word*)(nullptr);
    calc_word_in_cursor(&current);
    for(auto i = size_t(0), limit = chain->size(); i < limit; i += 1) {
        const auto& word = (*chain)[i];
        text += word.get_raw().get_feature();
        if(&word == current) {
            text += to_kana;
        }
        if(share.insert_space != InsertSpaceOptions::Off && i + 1 != limit) {
            text += " ";
        }
    }

    return text;
}

auto Context::apply_candidates() -> void {
    const auto base = get_candidate_list_base(context);
    if(base == nullptr || base->get_kind() == CandidateListKind::KanaDisplay) {
        return;
    }
    context.inputPanel().setCandidateList(nullptr);
    context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
}

auto Context::auto_commit() -> void {
    if(chain == nullptr || chain->size() < share.auto_commit_threshold || chain->size() < 2) {
        return;
    }
    const auto commit_num = chain->size() - share.auto_commit_threshold + 1;
    auto       on_holds   = size_t(0);
    auto       commited   = false;
    for(auto i = size_t(0); i <= commit_num; i += 1) {
        // we have to ensure that the following word's translations will remain the same without this word
        if((*chain)[on_holds].get_protection_level() != ProtectionLevel::PreserveTranslation) {
            auto copy = translate_wordchain(std::vector<Word>(chain->begin() + on_holds + 1, chain->end()), true)[0];
            if(copy[0].get_translated().get_feature() != (*chain)[on_holds + 1].get_translated().get_feature()) {
                // translation result will be changed
                // but maybe we can commit this word with next one
                on_holds += 1;
                continue;
            }
        }

        // commit words
        for(auto i = size_t(0); i <= on_holds; i += 1) {
            commit_word(&(*chain)[0]);
            const auto commited_bytes = (*chain)[0].get_raw().get_feature().size();
            if(cursor >= commited_bytes) {
                cursor -= commited_bytes;
            } else {
                cursor = 0;
            }
            chain->erase(chain->begin());
        }
        on_holds = 0;
        commited = true;
    }
    if(commited) {
        apply_candidates();
        chain_changed = true;
    }
}

auto Context::reset() -> void {
    chains.clear();
    chain         = nullptr;
    chain_changed = true;
}

auto Context::handle_key_event(fcitx::KeyEvent& event) -> void {
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

        auto current_word = (Word*)(nullptr);
        calc_word_in_cursor(&current_word, nullptr);
        if(current_word == nullptr) {
            break;
        }

        make_branch_chain();

        // get candidate list
        if(!current_word->is_candidates_initialized()) {
            calc_word_in_cursor(&current_word, nullptr);
            auto dic  = share.primary_vocabulary;
            auto dics = std::vector<MeCabModel*>{dic.get()};
            for(auto& dic : share.additional_vocabularies) {
                dics.emplace_back(dic.get());
            }
            current_word->reset_candidates(WordCandidates(dics, current_word->get_candidates()));
            current_word->set_protection_level(ProtectionLevel::PreserveTranslation);
        }
        if(!current_word->has_candidates()) {
            break;
        }

        if(!is_candidate_list_for(context, &current_word->get_candidates())) {
            context.inputPanel().setCandidateList(std::make_unique<CandidateList>(&current_word->get_candidates(), share.candidate_page_size));
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
        context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
        goto end;
    } while(0);

    // handle delete
    do {
        if(!share.key_config.match(Actions::Backspace, event)) {
            break;
        }
        if(!to_kana.empty()) {
            pop_back_u8(to_kana);
            apply_candidates();
            goto end;
        }
        move_cursor_back();
        auto current_word   = (Word*)(nullptr);
        auto cursor_in_word = size_t();
        calc_word_in_cursor(&current_word, &cursor_in_word);
        if(current_word == nullptr) {
            break;
        }
        auto raw   = u8tou32(current_word->get_raw().get_feature());
        auto bytes = size_t(0);
        auto kana  = raw.begin();
        for(; kana != raw.end(); kana += 1) {
            bytes += fcitx_ucs4_char_len(*kana);
            if(bytes == cursor_in_word) {
                break;
            }
        }
        if(kana == raw.end()) {
            break;
        }

        // disassembly the kana to romaji and pop back
        auto kana8 = std::array<char, FCITX_UTF8_MAX_LENGTH>();
        fcitx_ucs4_to_utf8(*kana, kana8.data());
        if(auto romaji = kana_to_romaji(kana8.data())) {
            to_kana = std::move(*romaji);
            pop_back_u8(to_kana);
        }
        // move cursor
        cursor -= fcitx_ucs4_char_len(*kana);

        // delete character from the word
        raw.erase(kana);
        current_word->get_mutable_feature() = u32tou8(raw);

        // translate
        current_word->set_protection_level(ProtectionLevel::None);
        *chain = translate_wordchain(*chain, true)[0];

        // chain may be empty
        if(chain->empty()) {
            reset();
        }

        apply_candidates();
        goto end;
    } while(0);

    // handle reinterpret wordchain
    do {
        constexpr auto actions = std::array{Actions::ReinterpretNext, Actions::ReinterpretPrev};
        if(chain == nullptr || !share.key_config.match(actions, event)) {
            break;
        }

        translation_changed = true;

        if(chain_changed) {
            chains.reset(translate_wordchain(*chain, false));
            chain_changed = false;
        }
        if(!is_candidate_list_for(context, &chains)) {
            context.inputPanel().setCandidateList(std::make_unique<CandidateList>(&chains, share.candidate_page_size));
        }
        auto candidate_list = context.inputPanel().candidateList().get();
        if(share.key_config.match(Actions::ReinterpretNext, event)) {
            candidate_list->toCursorMovable()->nextCandidate();
        } else {
            candidate_list->toCursorMovable()->prevCandidate();
        }
        context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
        chain = chains.get_current_ptr();
        for(auto& p : *chain) {
            p.set_protection_level(ProtectionLevel::PreserveTranslation);
        }
        goto end;
    } while(0);

    // handle commit wordchain
    do {
        if(!share.key_config.match(Actions::Commit, event)) {
            break;
        }
        if(chain == nullptr) {
            if(to_kana.empty()) {
                break;
            }
            context.commitString(to_kana);
            to_kana.clear();
            goto end;
        }
        commit_wordchain();
        reset();
        apply_candidates();
        goto end;
    } while(0);

    // handle move cursor
    do {
        constexpr auto actions = std::array{Actions::WordNext, Actions::WordPrev};
        if(!share.key_config.match(actions, event)) {
            break;
        }
        const auto forward        = share.key_config.match(Actions::WordNext, event);
        auto       current_word   = (Word*)(nullptr);
        auto       new_word       = (Word*)(nullptr);
        auto       cursor_in_word = size_t();
        calc_word_in_cursor(&current_word, &cursor_in_word);
        if(current_word == nullptr) {
            break;
        }
        new_word = forward ? current_word - 1 : current_word + 1;
        if(new_word < &(*chain)[0] || new_word > &chain->back()) {
            break;
        }

        // move cursor
        to_kana.clear();
        cursor = 0;
        for(auto p = &(*chain)[0]; p <= new_word; p += 1) {
            cursor += p->get_raw().get_feature().size();
        }
        apply_candidates();
        goto end;
    } while(0);

    // handle split word
    do {
        constexpr auto actions = std::array{Actions::SplitWordLeft, Actions::SplitWordRight};
        if(!share.key_config.match(actions, event)) {
            break;
        }

        make_branch_chain();

        auto current_word   = (Word*)(nullptr);
        auto cursor_in_word = size_t();
        calc_word_in_cursor(&current_word, &cursor_in_word);
        if(current_word == nullptr || fcitx_utf8_strlen(current_word->get_raw().get_feature().data()) <= 1) {
            break;
        }

        // insert a new word right after the selected word
        const auto word_index = current_word - &(*chain)[0];

        chain->insert(chain->begin() + word_index + 1, Word());
        const auto a = &(*chain)[word_index], b = &(*chain)[word_index + 1];

        // split 'a' into two
        const auto raw_32        = u8tou32(a->get_raw().get_feature());
        const auto split_pos     = share.key_config.match(Actions::SplitWordLeft, event) ? 1 : raw_32.size() - 1;
        b->get_mutable_feature() = u32tou8(raw_32.substr(split_pos));
        a->get_mutable_feature() = u32tou8(raw_32.substr(0, split_pos));

        // protect them
        a->set_protection_level(ProtectionLevel::PreserveSeparation);
        b->set_protection_level(ProtectionLevel::PreserveSeparation);

        *chain        = translate_wordchain(*chain, true)[0];
        chain_changed = true;
        apply_candidates();
        goto end;
    } while(0);

    // handle merge words
    do {
        constexpr auto actions = std::array{Actions::MergeWordsLeft, Actions::MergeWordsRight};
        if(!share.key_config.match(actions, event)) {
            break;
        }

        make_branch_chain();

        auto current_word   = (Word*)(nullptr);
        auto cursor_in_word = size_t();
        calc_word_in_cursor(&current_word, &cursor_in_word);
        if(current_word == nullptr) {
            apply_candidates();
            goto end;
        }
        const auto left       = share.key_config.match(Actions::MergeWordsLeft, event);
        const auto merge_word = current_word + (left ? -1 : 1);
        if(merge_word < &(*chain)[0] || merge_word > &chain->back()) {
            apply_candidates();
            goto end;
        }

        // merge 'current_word' and 'merge_word' into 'current_word'
        const auto a = merge_word->get_raw().get_feature(), b = current_word->get_raw().get_feature();
        current_word->get_mutable_feature() = left ? a + b : b + a;
        current_word->set_protection_level(ProtectionLevel::PreserveSeparation);

        // remove 'merge_word'
        const auto word_index = merge_word - &(*chain)[0];
        chain->erase(chain->begin() + word_index);

        *chain = translate_wordchain(*chain, true)[0];
        apply_candidates();
        goto end;
    } while(0);

    // handle move separator
    do {
        constexpr auto actions = std::array{Actions::MoveSeparatorLeft, Actions::MoveSeparatorRight};
        if(!share.key_config.match(actions, event)) {
            break;
        }

        make_branch_chain();

        auto current_word   = (Word*)(nullptr);
        auto cursor_in_word = size_t();
        calc_word_in_cursor(&current_word, &cursor_in_word);
        if(current_word == nullptr) {
            goto end;
        }
        const auto move_word = current_word + 1;
        if(move_word > &chain->back()) {
            apply_candidates();
            goto end;
        }

        // align cursor to word's back
        cursor += current_word->get_raw().get_feature().size() - cursor_in_word;

        // move cursor and separator
        const auto left = share.key_config.match(Actions::MoveSeparatorLeft, event);
        if(left) {
            auto       current_u32 = u8tou32(current_word->get_raw().get_feature());
            const auto moved_char  = u32tou8(current_u32.back());
            move_word->get_mutable_feature().insert(0, moved_char);
            current_u32.pop_back();
            if(current_u32.empty()) {
                chain->erase(chain->begin() + (current_word - chain->data()));
            } else {
                current_word->get_mutable_feature() = u32tou8(current_u32);
            }
            cursor -= moved_char.size();
            if(cursor == 0) {
                // left most word merged
                cursor = move_word->get_mutable_feature().size();
            }
        } else {
            auto       move_u32   = u8tou32(move_word->get_raw().get_feature());
            const auto moved_char = u32tou8(move_u32[0]);
            current_word->get_mutable_feature() += moved_char;
            move_u32.erase(0, 1);
            if(move_u32.empty()) {
                chain->erase(chain->begin() + (move_word - chain->data()));
            } else {
                move_word->get_mutable_feature() = u32tou8(move_u32);
            }
            cursor += moved_char.size();
        }

        current_word->set_protection_level(ProtectionLevel::PreserveSeparation);
        move_word->set_protection_level(ProtectionLevel::PreserveSeparation);
        *chain        = translate_wordchain(*chain, true)[0];
        chain_changed = true;
        apply_candidates();
        goto end;
    } while(0);

    // handle convert katakana
    do {
        if(!share.key_config.match(Actions::ConvertKatakana, event)) {
            break;
        }

        make_branch_chain();

        auto current_word = (Word*)(nullptr);
        calc_word_in_cursor(&current_word);
        if(current_word == nullptr) {
            break;
        }

        auto        katakana32 = std::u32string();
        const auto& raw        = current_word->get_raw().get_feature();
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
            auto  dic     = share.primary_vocabulary;
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
                    found         = true;
                    *current_word = Word(MeCabWord(raw), MeCabWord(katakana, node->rcAttr, node->lcAttr, node->wcost, node->cost, true));
                    break;
                }
            }
            lattice.clear();
            if(!found) {
                current_word->override_translated(MeCabWord(katakana));
            }
        }

        current_word->set_protection_level(ProtectionLevel::PreserveTranslation);
        apply_candidates();
        goto end;
    } while(0);

    // handle romaji
    do {
        const auto key = event.key();
        if(event.isRelease() || (key.states() != fcitx::KeyState::NoState && key.states() != fcitx::KeyState::Shift)) {
            break;
        }
        const auto romaji_8 = fcitx::Key::keySymToUTF8(key.sym());
        if(fcitx_utf8_strlen(romaji_8.data()) != 1) {
            break;
        }
        merge_branch_chains();
        move_cursor_back();

        to_kana += romaji_8;
        auto filter_result = romaji_index.filter(to_kana);
        if(filter_result.get<RomajiIndex::EmptyCache>()) {
            to_kana       = romaji_8;
            filter_result = romaji_index.filter(to_kana);
            if(filter_result.get<RomajiIndex::EmptyCache>()) {
                to_kana.clear();
                break;
            }
        }

        apply_candidates();
        delete_surrounding_text();
        if(const auto data = filter_result.get<RomajiIndex::ExactOne>()) {
            const auto& exact = *data->result;
            append_kana(exact.kana);
            if(exact.refill != nullptr) {
                to_kana = exact.refill;
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
    } else if(chain != nullptr) {
        event.filterAndAccept();
    }

    if(chain == nullptr) {
        context.inputPanel().setCandidateList(nullptr);
        context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    } else {
        const auto base = get_candidate_list_base(context);
        if(base == nullptr) {
            context.inputPanel().setCandidateList(std::make_unique<KanaDisplay>(nullptr, build_kana_text()));
            context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
        } else if(base->get_kind() == CandidateListKind::KanaDisplay) {
            const auto display = downcast<KanaDisplay>(base);
            display->upate_text(build_kana_text());
            context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
        }
    }

    return;
}

auto Context::handle_activate() -> void {
    context.setCapabilityFlags(context.capabilityFlags() |= fcitx::CapabilityFlag::ClientUnfocusCommit);
}

auto Context::handle_deactivate() -> void {
    apply_candidates();
    context.inputPanel().reset();
    context.updatePreedit();
    context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    if(chain != nullptr) {
        commit_wordchain();
        reset();
    }
    if(!to_kana.empty()) {
        context.commitString(to_kana);
        to_kana.clear();
    }
}

Context::Context(fcitx::InputContext& context, engine::Engine& engine, Share& share)
    : context(context),
      engine(engine),
      share(share) {}
} // namespace mikan
