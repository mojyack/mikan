#include <fcitx-utils/keysym.h>

#include "configuration.hpp"

namespace mikan {
KeyConfigKey::KeyConfigKey(fcitx::KeySym sym, fcitx::KeyStates state) {
    key = fcitx::Key(sym, state);
}

auto KeyConfig::operator[](Actions action) -> std::vector<KeyConfigKey>& {
    return keys[static_cast<size_t>(action)];
}
auto KeyConfig::match(const std::vector<Actions>& actions, const fcitx::KeyEvent& event) const -> bool {
    for(auto a : actions) {
        if(match(a, event)) {
            return true;
        }
    }
    return false;
}
auto KeyConfig::match(Actions action, const fcitx::KeyEvent& event) const -> bool {
    const auto& key = event.key();
    for(const auto& k : keys[static_cast<size_t>(action)]) {
        if(key.check(k.key) && ((k.on_press && !event.isRelease()) || (k.on_release && event.isRelease()))) {
            return true;
        }
    }
    return false;
}
} // namespace mikan
