#include <Fcitx5/Core/fcitx/instance.h>
#include <Fcitx5/Module/fcitx-module/clipboard/clipboard_public.h>

#include "context.hpp"
#include "macros/unwrap.hpp"
#include "misc.hpp"
#include "romaji-index.hpp"

namespace mikan {
auto Command::show_message(std::string message) -> void {
    ctx->share.instance->showCustomInputMethodInformation(&ctx->context, message);
}

namespace {
struct KanaArgumentCommand : Command {
    std::string raw;
    std::string to_kana;
    RomajiIndex romaji_index;

    virtual auto on_commit() -> void = 0;

    auto handle_key(fcitx::KeyEvent& event) -> bool override {
        using enum Actions;

        auto accept = true;
        auto done   = false;

        // handle backspace
        do {
            if(!ctx->share.key_config.match(Backspace, event)) {
                break;
            }
            if(!to_kana.empty()) {
                pop_back_u8(to_kana);
                goto end;
            }
            if(raw.empty()) {
                goto end;
            }

            // delete character from the word
            const auto back = pop_back_u8(raw);
            // try to disassemble the kana character into romaji
            auto kana8 = std::array<char, FCITX_UTF8_MAX_LENGTH>();
            fcitx_ucs4_to_utf8(back, kana8.data());
            if(auto romaji = kana_to_romaji(kana8.data())) {
                to_kana = std::move(*romaji);
                pop_back_u8(to_kana);
            }
            goto end;
        } while(0);

        // handle commit
        do {
            if(!ctx->share.key_config.match(Commit, event)) {
                break;
            }
            on_commit();
            done = true;
            goto end;
        } while(0);

        // handle romaji
        do {
            const auto c8 = press_event_to_single_char(event);
            if(!c8) {
                goto end;
            }

            to_kana += *c8;
            auto filter_result = romaji_index.filter(to_kana);
            if(filter_result.get<RomajiIndex::EmptyCache>()) {
                to_kana       = *c8;
                filter_result = romaji_index.filter(to_kana);
                if(filter_result.get<RomajiIndex::EmptyCache>()) {
                    to_kana.clear();
                    goto end;
                }
            }

            if(auto data = filter_result.get<RomajiIndex::ExactOne>()) {
                auto& exact = *data->result;
                raw += exact.kana;
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
        }

        return done;
    }
};

struct DefCommand : KanaArgumentCommand {
    std::string converted;

    auto init() -> bool override {
        unwrap_mut(clipboard, ctx->share.clipboard, "no clipboard plugin");
        converted = fcitx::stringutils::replaceAll(clipboard.call<fcitx::IClipboard::primary>(&ctx->context), "\n", "");
        if(converted.empty()) {
            show_message("clipboard is empty");
            return false;
        } else {
            return true;
        }
    }

    auto build_preedit(fcitx::Text& preedit) -> void override {
        preedit.append(raw, fcitx::TextFormatFlag::Underline);
        preedit.append(to_kana, fcitx::TextFormatFlag::Underline);
        preedit.setCursor(preedit.textLength());
        preedit.append(" = ");
        preedit.append(converted);
    }

    auto on_commit() -> void override {
        ctx->engine.add_convert_definition(raw, converted);
    }
};

struct UndefCommand : KanaArgumentCommand {
    auto build_preedit(fcitx::Text& preedit) -> void override {
        preedit.append(raw);
        preedit.append(to_kana);
        preedit.setCursor(preedit.textLength());
    }

    auto on_commit() -> void override {
        ctx->engine.remove_convert_definition(raw);
    }
};

auto find_command(const std::string_view name) -> Command* {
    if(name == "/def") return new DefCommand();
    if(name == "/undef") return new UndefCommand();
    return nullptr;
}
} // namespace

auto Context::exit_command_mode() -> void {
    command_mode_context.reset();
    context.inputPanel().reset();
    context.updatePreedit();
}

auto Context::handle_key_event_command(fcitx::KeyEvent& event) -> void {
    using enum Actions;
    auto& panel  = context.inputPanel();
    auto& ctx    = *command_mode_context;
    auto  accept = true;

    // exit command mode
    do {
        if(!share.key_config.match(ExitCommandMode, event)) {
            break;
        }
        exit_command_mode();
        goto end;
    } while(0);

    // derive to command argument
    if(ctx.handler) {
        const auto done = ctx.handler->handle_key(event);
        if(event.accepted()) {
            if(done) {
                exit_command_mode();
            }
            goto end;
        }
    }

    // simple commands
    do {
        if(!share.key_config.match(Commit, event)) {
            break;
        }
        if(ctx.command == "/reload") {
            engine.compile_and_reload_user_dictionary();
            exit_command_mode();
        } else {
            share.instance->showCustomInputMethodInformation(&context, "unknown command");
            ctx.command.clear();
        }
        goto end;
    } while(0);

    // backspace
    do {
        if(!share.key_config.match(Backspace, event)) {
            break;
        }
        if(!ctx.command.empty()) {
            pop_back_u8(ctx.command);
        }
        goto end;
    } while(0);

    // command name
    do {
        const auto c8 = press_event_to_single_char(event);
        if(!c8) {
            break;
        }
        if(*c8 == ' ') {
            ctx.handler.reset(find_command(ctx.command));
            if(ctx.handler) {
                ctx.handler->ctx  = this;
                ctx.handler->cctx = &ctx;
                if(!ctx.handler->init()) {
                    ctx.command.clear();
                    ctx.handler.reset();
                }
            } else {
                share.instance->showCustomInputMethodInformation(&context, "unknown command");
                ctx.command.clear();
            }
        } else {
            ctx.command += *c8;
        }

        goto end;
    } while(0);

    accept = false;

end:
    if(!command_mode_context) {
        // exitted
        event.filterAndAccept();
        return;
    }
    if(accept) {
        event.filterAndAccept();
        auto preedit = fcitx::Text("[ ");
        preedit.append(ctx.command, ctx.handler ? fcitx::TextFormatFlag::NoFlag : fcitx::TextFormatFlag::Underline);
        if(ctx.handler) {
            preedit.append(" ");
            ctx.handler->build_preedit(preedit);
        } else {
            preedit.setCursor(preedit.textLength());
        }
        preedit.append(" ]");
        panel.setClientPreedit(preedit);
        context.updatePreedit();
    }
    return;
}

auto Context::handle_deactivate_command() -> void {
    exit_command_mode();
}
} // namespace mikan
