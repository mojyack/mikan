#include <array>

#include <fcitx-utils/utf8.h>

#include "macros/unwrap.hpp"
#include "misc.hpp"
#include "romaji-table.hpp"
#include "spawn/process.hpp"

namespace {
auto get_xdg_path(const char* const xdg_env_name, const char* const fallback_dir_name) -> std::string {
    auto       ret = std::string();
    const auto xdg = std::getenv(xdg_env_name);
    if(xdg == nullptr) {
        ret = std::string(std::getenv("HOME")) + "/" + fallback_dir_name + "/";
    } else {
        ret = std::string(xdg) + "/";
    }
    ret.append("mikan");
    return ret;
}
} // namespace

namespace mikan {
auto get_user_config_dir() -> std::string {
    return get_xdg_path("XDG_CONFIG_HOME", ".config");
}

auto get_user_cache_dir() -> std::string {
    return get_xdg_path("XDG_CACHE_HOME", ".cache");
}

auto get_dictionary_compiler_path() -> std::optional<std::string> {
    auto outputs      = std::string();
    auto process      = process::Process();
    process.on_stdout = [&outputs](std::span<char> output) {
        outputs.insert(outputs.end(), output.begin(), output.end());
    };
    const auto argv = std::vector{"/usr/bin/mecab-config", "--libexecdir", (const char*)nullptr};
    ensure(process.start({.argv = argv}));
    while(process.get_status() == process::Status::Running) {
        ensure(process.collect_outputs());
    }
    unwrap(result, process.join());
    ensure(result.code == 0);
    ensure(!outputs.empty());
    if(outputs.back() == '\n') {
        outputs.pop_back();
    }
    return outputs;
}

auto u8tou32(const std::string_view u8) -> std::u32string {
    auto u32 = std::u32string();

    auto p        = &u8[0];
    auto len_left = u8.size();
    while(len_left) {
        auto len_read = int();
        u32 += fcitx_utf8_get_char_validated(p, len_left, &len_read);
        p += len_read;
        len_left -= len_read;
    }
    return u32;
}

auto u32tou8(const std::u32string_view u32) -> std::string {
    auto u8 = std::string();
    for(const auto c : u32) {
        auto buffer = std::array<char, FCITX_UTF8_MAX_LENGTH>();
        fcitx_ucs4_to_utf8(c, buffer.data());
        u8 += buffer.data();
    }
    return u8;
}

auto u32tou8(const char32_t u32) -> std::string {
    auto buffer = std::array<char, FCITX_UTF8_MAX_LENGTH>();
    fcitx_ucs4_to_utf8(u32, buffer.data());
    return buffer.data();
}

auto kana_to_romaji(const std::string_view kana) -> std::optional<std::string> {
    for(auto i = uint64_t(0); i < sizeof(romaji_table) / sizeof(RomajiKana); i += 1) {
        if(romaji_table[i].kana == kana) {
            return romaji_table[i].romaji;
        }
    }
    return {};
}

auto pop_back_u8(std::string& u8) -> char32_t {
    auto u32 = u8tou32(u8);
    const auto ret = u32.back();
    u32.pop_back();
    u8 = u32tou8(u32);
    return ret;
}

auto press_event_to_single_char(const fcitx::KeyEvent& event) -> std::optional<char> {
    const auto key = event.key();
    if(event.isRelease() || (key.states() != fcitx::KeyState::NoState && key.states() != fcitx::KeyState::Shift)) {
        return {};
    }
    const auto chars = fcitx::Key::keySymToUTF8(key.sym());
    if(chars.size() != 1) {
        return {};
    }
    return chars[0];
}
} // namespace mikan
