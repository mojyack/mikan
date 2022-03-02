#pragma once
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include "context.hpp"
#include "engine.hpp"

namespace mikan {
class Factory final : public fcitx::InputMethodEngine {
  private:
    Share                      share;
    Engine                     engine;
    fcitx::FactoryFor<Context> factory;

  public:
    auto keyEvent(const fcitx::InputMethodEntry& entry, fcitx::KeyEvent& event) -> void override {
        auto& context = *event.inputContext();
        auto  state   = context.propertyFor(&factory);
        state->handle_key_event(event);
    }
    auto activate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) -> void override {
        auto& context = *event.inputContext();
        auto  state   = context.propertyFor(&factory);
        state->handle_activate();
        event.accept();
    }
    auto deactivate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) -> void override {
        auto& context = *event.inputContext();
        auto  state   = context.propertyFor(&factory);
        state->handle_deactivate();
        event.accept();
        reset(entry, event);
    }

    Factory(fcitx::Instance* const instance) : engine(share),
                                               factory([this](fcitx::InputContext& context) {
                                                   return new Context(context, engine, share);
                                               }) {
        instance->inputContextManager().registerProperty("mikan", &factory);
    }
};

class AddonFactory : public fcitx::AddonFactory {
  public:
    auto create(fcitx::AddonManager* const manager) -> fcitx::AddonInstance* override {
        // fcitx::registerDomain("mikan", FCITX_INSTALL_LOCALEDIR);
        return new Factory(manager->instance());
    }
};
} // namespace mikan
