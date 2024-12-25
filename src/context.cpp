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

auto Context::is_empty() const -> bool {
    return chains.empty();
}

auto Context::get_current_chain() -> WordChain& {
    ASSERT(!is_empty());
    return chains[chains.index];
}

auto Context::get_current_chain() const -> const WordChain& {
    return ((Context*)this)->get_current_chain();
}

auto Context::commit_word(const Word& word) -> void {
    context.commitString(word.feature());
}

auto Context::commit_wordchain() -> void {
    merge_branch_chains();
    for(const auto& word : get_current_chain()) {
        commit_word(word);
    }
    context.commitString(to_kana);
    to_kana.clear();
}

auto Context::make_branch_chain() -> void {
    if(!translation_changed) {
        shrink_chains(true);
        translation_changed = true;
    }
}

auto Context::merge_branch_chains() -> bool {
    if(is_empty()) {
        return false;
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
    if(is_empty()) {
        return;
    }
    auto& current = get_current_chain();
    if(reserve_one) {
        chains.reset({current, current});
        chains.set_index_wrapped(1);
    } else {
        chains.reset({current});
    }
}

auto Context::delete_surrounding_text() -> bool {
    if(!is_empty() || !to_kana.empty() || !context.capabilityFlags().test(fcitx::CapabilityFlag::SurroundingText)) {
        return false;
    }
    const auto& text = context.surroundingText();
    if(!text.isValid()) {
        return false;
    }
    context.deleteSurroundingText(0, text.anchor() - text.cursor());
    return true;
}

auto Context::translate_wordchain(const WordChain& source, const bool best_only) -> WordChains {
    if(is_empty()) {
        return WordChains();
    }
    return engine.convert_wordchain(source, best_only);
}

auto Context::build_preedit_text() const -> fcitx::Text {
    auto preedit = fcitx::Text();
    if(is_empty()) {
        preedit.append(to_kana, fcitx::TextFormatFlag::Underline);
        preedit.setCursor(to_kana.size());
        return preedit;
    }
    auto& chain = get_current_chain();

    const auto is_current_last = cursor == chain.size() - 1;
    const auto has_branches    = chains.get_data_size() >= 2;
    for(auto i = 0u; i < chain.size(); i += 1) {
        const auto& word = chain[i];

        auto text   = std::string();
        auto format = fcitx::TextFormatFlag();
        if(i == cursor) {
            if(!to_kana.empty()) {
                text   = word.raw() + to_kana;
                format = fcitx::TextFormatFlag::Underline;
            } else {
                text   = word.feature();
                format = fcitx::TextFormatFlag::Bold;
            }
        } else {
            text   = word.feature();
            format = fcitx::TextFormatFlag::Underline;
        }
        const auto insert_space = share.insert_space == InsertSpaceOptions::On || (share.insert_space == InsertSpaceOptions::Smart && (!is_current_last || has_branches));
        if(i == cursor && insert_space) {
            text = "[" + text + "]";
        }
        preedit.append(text, format);
        // add space between words
        if(insert_space) {
            preedit.append("|");
        }
        if(i == cursor) {
            preedit.setCursor(preedit.textLength());
        }
    }
    return preedit;
}

auto Context::build_kana_text() const -> std::string {
    if(is_empty()) {
        return to_kana;
    }
    const auto& chain = get_current_chain();

    auto text = std::string();
    for(const auto& word : chain) {
        text += word.raw();
        const auto last = &word + 1 == &get_current_chain().back();
        if(share.insert_space != InsertSpaceOptions::Off && !last) {
            text += " ";
        }
    }
    text += to_kana;

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
    if(is_empty()) {
        return;
    }
    auto& chain = get_current_chain();
    if(chain.size() < share.auto_commit_threshold || chain.size() < 2) {
        return;
    }
    const auto commit_num = chain.size() - share.auto_commit_threshold + 1;
    auto       on_holds   = size_t(0);
    auto       commited   = false;
    for(auto i = size_t(0); i <= commit_num; i += 1) {
        // we have to ensure that the following word's translations will remain the same without this word
        if(chain[on_holds].protection != ProtectionLevel::PreserveTranslation) {
            auto copy = translate_wordchain(std::vector<Word>(chain.begin() + on_holds + 1, chain.end()), true)[0];
            if(copy[0].feature() != chain[on_holds + 1].feature()) {
                // translation result will be changed
                // but maybe we can commit this word with next one
                on_holds += 1;
                continue;
            }
        }

        // commit words
        for(auto i = size_t(0); i <= on_holds; i += 1) {
            commit_word(chain[0]);
            chain.erase(chain.begin());
            cursor -= 1;
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
    chain_changed = true;
}

auto Context::handle_key_event(fcitx::KeyEvent& event) -> void {
    using enum Actions;
    auto& panel  = context.inputPanel();
    auto  accept = true;
    // handle candidates
    do {
        const auto action = share.key_config.match({CandidateNext, CandidatePrev, CandidatePageNext, CandidatePagePrev}, event);
        if(!action) {
            break;
        }
        if(!context.inputPanel().candidateList() && action != CandidateNext) {
            break;
        }
        if(is_empty()) {
            break;
        }
        make_branch_chain();
        auto& chain = get_current_chain();

        // get candidate list
        auto& word = chain[cursor];
        if(!word.has_candidates()) {
            auto dic  = share.primary_vocabulary;
            auto dics = std::vector<MeCabModel*>{dic.get()};
            for(auto& dic : share.additional_vocabularies) {
                dics.emplace_back(dic.get());
            }

            auto new_word = Word::from_dictionaries(dics, word);

            const auto insert_candidate = [&new_word](const size_t pos, const std::string& str) {
                auto& arr = new_word.candidates;
                if(const auto iter = std::ranges::find(arr, str); iter == arr.end()) {
                    arr.erase(iter);
                }
                arr.insert(arr.begin() + pos, std::string(str));
            };
            // insert current feature to top
            insert_candidate(2, word.feature());
            // then hiragana
            if(word.raw() != word.feature()) {
                insert_candidate(3, word.raw());
            }
            new_word.protection = ProtectionLevel::PreserveTranslation;
            word                = std::move(new_word);
        }
        if(!word.has_candidates()) {
            goto end;
        }

        if(!is_candidate_list_for(context, &word)) {
            context.inputPanel().setCandidateList(std::make_unique<CandidateList>(&word, share.candidate_page_size));
        }
        auto candidate_list = context.inputPanel().candidateList().get();

        // move candidate list index
        switch(*action) {
        case CandidateNext:
            candidate_list->toCursorMovable()->nextCandidate();
            break;
        case CandidatePrev:
            candidate_list->toCursorMovable()->prevCandidate();
            break;
        case CandidatePageNext:
            candidate_list->toPageable()->prev();
            break;
        case CandidatePagePrev:
            candidate_list->toPageable()->next();
            break;
        default:
            break;
        }
        context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
        goto end;
    } while(0);

    // handle delete
    do {
        if(!share.key_config.match(Backspace, event)) {
            break;
        }
        if(!to_kana.empty()) {
            pop_back_u8(to_kana);
            apply_candidates();
            goto end;
        }
        if(is_empty()) {
            break;
        }
        auto& chain = get_current_chain();

        auto& word = chain.back();

        auto       u32  = u8tou32(word.raw());
        const auto back = u32.back();
        u32.pop_back();
        // delete character from the word
        word.raw()      = u32tou8(u32);
        word.protection = ProtectionLevel::None;

        // try to disassemble the kana character into romaji
        auto kana8 = std::array<char, FCITX_UTF8_MAX_LENGTH>();
        fcitx_ucs4_to_utf8(back, kana8.data());
        if(auto romaji = kana_to_romaji(kana8.data())) {
            to_kana = std::move(*romaji);
            pop_back_u8(to_kana);
        }

        // translate
        chain  = translate_wordchain(chain, true)[0];
        cursor = chain.size() - 1;

        // chain may be empty
        if(chain.empty()) {
            reset();
        }

        apply_candidates();
        goto end;
    } while(0);

    // handle reinterpret wordchain
    do {
        const auto action = share.key_config.match({ReinterpretNext, ReinterpretPrev}, event);
        if(!action) {
            break;
        }
        if(is_empty()) {
            break;
        }

        translation_changed = true;

        if(chain_changed) {
            chains.reset(translate_wordchain(get_current_chain(), false));
            cursor        = get_current_chain().size() - 1;
            chain_changed = false;
        }
        if(!is_candidate_list_for(context, &chains)) {
            context.inputPanel().setCandidateList(std::make_unique<CandidateList>(&chains, share.candidate_page_size));
        }
        auto candidate_list = context.inputPanel().candidateList().get();
        if(action == ReinterpretNext) {
            candidate_list->toCursorMovable()->nextCandidate();
        } else {
            candidate_list->toCursorMovable()->prevCandidate();
        }
        context.updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
        for(auto& word : get_current_chain()) {
            word.protection = ProtectionLevel::PreserveTranslation;
        }
        goto end;
    } while(0);

    // handle commit wordchain
    do {
        if(!share.key_config.match(Commit, event)) {
            break;
        }
        if(is_empty()) {
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
        const auto action = share.key_config.match({WordNext, WordPrev}, event);
        if(!action) {
            break;
        }
        if(is_empty()) {
            break;
        }
        const auto& chain = get_current_chain();

        const auto forward    = action == WordNext;
        const auto new_cursor = int(cursor) + (forward ? 1 : -1);
        if(new_cursor >= chain.size() || new_cursor < 0) {
            goto end;
        }

        // move cursor
        to_kana.clear();
        cursor = new_cursor;
        apply_candidates();
        goto end;
    } while(0);

    // handle split word
    do {
        const auto action = share.key_config.match({SplitWordLeft, SplitWordRight}, event);
        if(!action) {
            break;
        }
        if(is_empty()) {
            break;
        }
        make_branch_chain();
        auto& chain = get_current_chain();

        const auto left   = action == SplitWordLeft;
        const auto raw_32 = u8tou32(chain[cursor].raw());
        if(raw_32.size() == 1) {
            // cannot split this anymore
            goto end;
        }

        auto& b = *chain.insert(chain.begin() + cursor + 1, Word());
        auto& a = chain[cursor]; // must be this order
        if(left) {
            cursor += 1;
        }

        // split 'a' into two
        const auto split_pos = left ? 1 : raw_32.size() - 1;

        a = Word::from_raw(u32tou8(raw_32.substr(0, split_pos)));
        b = Word::from_raw(u32tou8(raw_32.substr(split_pos)));

        // protect them
        a.protection = ProtectionLevel::PreserveSeparation;
        b.protection = ProtectionLevel::PreserveSeparation;

        chain         = translate_wordchain(chain, true)[0];
        cursor        = std::min(cursor, chain.size() - 1); // TODO: retrieve correct cursor position
        chain_changed = true;
        apply_candidates();
        goto end;
    } while(0);

    // handle merge words
    do {
        const auto action = share.key_config.match({MergeWordsLeft, MergeWordsRight}, event);
        if(!action) {
            break;
        }
        if(is_empty()) {
            break;
        }
        make_branch_chain();
        auto& chain = get_current_chain();

        const auto left        = share.key_config.match(MergeWordsLeft, event);
        const auto merge_index = int(cursor) + (left ? -1 : 1);
        if(merge_index >= chain.size() || merge_index < 0) {
            goto end;
        }

        auto& a = chain[cursor].raw();
        auto& b = chain[merge_index].raw();
        // merge them
        chain[cursor].raw()      = left ? b + a : a + b;
        chain[cursor].protection = ProtectionLevel::PreserveSeparation;
        if(left) {
            cursor -= 1;
        }

        // remove merged word
        chain.erase(chain.begin() + merge_index);

        // translate
        chain  = translate_wordchain(chain, true)[0];
        cursor = std::min(cursor, chain.size() - 1); // TODO: retrieve correct cursor position
        apply_candidates();
        goto end;
    } while(0);

    // handle move separator
    do {
        const auto action = share.key_config.match({TakeFromLeft, TakeFromRight, GiveToLeft, GiveToRight}, event);
        if(!action) {
            break;
        }
        if(is_empty()) {
            break;
        }
        make_branch_chain();
        auto& chain = get_current_chain();

        const auto target_index = int(cursor) + (action == TakeFromLeft || action == GiveToLeft ? -1 : 1);
        const auto take         = action == TakeFromLeft || action == TakeFromRight;
        if(target_index >= chain.size() || target_index < 0) {
            goto end;
        }

        auto& word         = chain[cursor];
        auto& target       = chain[target_index];
        auto  word_feature = u8tou32(word.raw());
        auto  tarfeature   = u8tou32(target.raw());
        if((take && tarfeature.size() == 1) || (!take && word_feature.size() == 1)) {
            goto end;
        }
        switch(*action) {
        case TakeFromLeft:
            word_feature = tarfeature.back() + word_feature;
            tarfeature.pop_back();
            break;
        case TakeFromRight:
            word_feature = word_feature + tarfeature.front();
            tarfeature.erase(tarfeature.begin());
            break;
        case GiveToLeft:
            tarfeature = tarfeature + word_feature.front();
            word_feature.erase(word_feature.begin());
            break;
        case GiveToRight:
            tarfeature = word_feature.back() + tarfeature;
            word_feature.pop_back();
            break;
        default:
            break;
        }
        word.raw()        = u32tou8(word_feature);
        target.raw()      = u32tou8(tarfeature);
        word.protection   = ProtectionLevel::PreserveSeparation;
        target.protection = ProtectionLevel::PreserveSeparation;

        chain         = translate_wordchain(chain, true)[0];
        cursor        = std::min(cursor, chain.size() - 1); // TODO: retrieve correct cursor position
        chain_changed = true;
        apply_candidates();
        goto end;
    } while(0);

    // handle convert katakana
    do {
        if(!share.key_config.match(Actions::ConvertKatakana, event)) {
            break;
        }
        if(is_empty()) {
            break;
        }
        make_branch_chain();
        auto& chain = get_current_chain();

        auto& word = chain[cursor];

        auto katakana32 = std::u32string();
        for(const auto c : u8tou32(word.raw())) {
            if(const auto p = hiragana_katakana_table.find(c); p != hiragana_katakana_table.end()) {
                katakana32 += p->second;
            } else {
                katakana32 += c;
            }
        }
        word.candidates.resize(2);
        word.candidates[1] = u32tou8(katakana32);
        word.protection    = ProtectionLevel::PreserveTranslation;

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
        if(auto data = filter_result.get<RomajiIndex::ExactOne>()) {
            auto& exact = *data->result;
            if(is_empty()) {
                auto word = Word::from_raw(std::move(exact.kana));
                chains.reset({WordChain{word}});
            } else {
                auto& chain = get_current_chain();
                auto  word  = &chain.back();
                // is the last word is not modifiable, we have to append new one
                if(word->protection != ProtectionLevel::None) {
                    word = &chain.emplace_back();
                }
                word->raw() += exact.kana;
                chain_changed = true;
            }

            auto& chain = get_current_chain();
            chain       = translate_wordchain(chain, true)[0];
            cursor      = chain.size() - 1;
            auto_commit();

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
    } else if(!is_empty()) {
        event.filterAndAccept();
    }

    if(is_empty()) {
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
    if(!is_empty()) {
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
