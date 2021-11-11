#include <fcitx/inputcontext.h>
#include <fcitx/inputpanel.h>

#include "mikan5.hpp"
#include "state.hpp"

namespace mikan {
auto MikanState::commit_phrase(const Phrase* const phrase) -> void {
    const auto& translated = phrase->get_translated();
    context.commitString(translated.get_feature());
}
auto MikanState::commit_all_phrases() -> void {
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
auto MikanState::make_branch_sentence() -> void {
    if(!translation_changed) {
        shrink_sentences(true);
        translation_changed = true;
    }
}
auto MikanState::merge_branch_sentences() -> bool {
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
auto MikanState::shrink_sentences(const bool reserve_one) -> void {
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
auto MikanState::delete_surrounding_text() -> bool {
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
auto MikanState::calc_phrase_in_cursor(Phrase** const phrase, size_t* const cursor_in_phrase) const -> void {
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
    throw std::runtime_error("cursor is not in any phrase.");
}
auto MikanState::move_cursor_back() -> void {
    if(phrases == nullptr) {
        return;
    }
    // move cursor to back.
    cursor = 0;
    for(const auto& p : *phrases) {
        cursor += p.get_raw().get_feature().size();
    }
}
auto MikanState::append_kana(const std::string& kana) -> void {
    // append kana to selected phrase.
    if(phrases == nullptr) {
        sentences.reset({Phrases{Phrase{kana}}});
        phrases = sentences.get_current_ptr();
        cursor  = kana.size();
    } else {
        auto current    = (Phrase*)(nullptr);
        auto cursor_pos = size_t();
        calc_phrase_in_cursor(&current, &cursor_pos);
        if(current->get_protection_level() != ProtectionLevel::NONE) {
            phrases->emplace_back();
            current    = &phrases->back();
            cursor_pos = 0;
        }
        current->get_mutable_feature().insert(cursor_pos, kana);
        current->set_protection_level(ProtectionLevel::NONE);
        cursor += kana.size();
        sentence_changed = true;
    }
    *phrases = translate_phrases(*phrases, true)[0];
    auto_commit();
}
auto MikanState::translate_phrases(const Phrases& source, const bool best_only) -> Sentences {
    if(phrases == nullptr) {
        return Sentences();
    }
    return engine.translate_phrases(source, best_only);
}
auto MikanState::build_preedit_text() const -> fcitx::Text {
    auto preedit = fcitx::Text();
    if(phrases == nullptr) {
        std::string text = to_kana;
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
auto MikanState::apply_candidates() -> void {
    if(!context.inputPanel().candidateList()) {
        return;
    }
    // hide candidate list window.
    context.inputPanel().setCandidateList(nullptr);
}
auto MikanState::auto_commit() -> void {
    if(phrases == nullptr || phrases->size() < share.auto_commit_threshold || phrases->size() < 2) {
        return;
    }
    const auto commit_num = phrases->size() - share.auto_commit_threshold + 1;
    auto       on_holds   = size_t(0);
    auto       commited   = false;
    for(auto i = size_t(0); i <= commit_num; i += 1) {
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
auto MikanState::reset() -> void {
    sentences.clear();
    phrases          = nullptr;
    sentence_changed = true;
}
auto MikanState::handle_key_event(fcitx::KeyEvent& event) -> void {
    auto& panel  = context.inputPanel();
    auto  accept = false;
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
auto MikanState::handle_activate() -> void {
    context.setCapabilityFlags(context.capabilityFlags() |= fcitx::CapabilityFlag::ClientUnfocusCommit);
}
auto MikanState::handle_deactivate() -> void {
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
MikanState::MikanState(fcitx::InputContext& context, MikanEngine& engine, Share& share) : context(context), engine(engine), share(share) {}
} // namespace mikan
