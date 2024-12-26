#pragma once
#include <memory>
#include <string>

#include <fcitx/event.h>
#include <fcitx/text.h>

namespace mikan {
struct CommandModeContext;
class Context;

struct Command {
    Context*            ctx;
    CommandModeContext* cctx;

    auto show_message(std::string message) -> void;

    virtual auto init() -> bool {
        return true;
    }

    virtual auto build_preedit(fcitx::Text& preedit) -> void {
    }

    virtual auto handle_key(fcitx::KeyEvent& event) -> bool {
        return false;
    }

    virtual ~Command() {
    }
};

struct CommandModeContext {
    std::string              to_kana;
    std::string              command;
    std::unique_ptr<Command> handler;
};
} // namespace mikan
