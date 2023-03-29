#pragma once
#include <array>
#include <optional>
#include <string>
#include <vector>

#include <fcitx-utils/utf8.h>

#include "romaji-table.hpp"
#include "util.hpp"

namespace mikan {
inline auto get_xdg_path(const char* const xdg_env_name, const char* const fallback_dir_name) -> std::string {
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

inline auto get_user_config_dir() -> std::string {
    return get_xdg_path("XDG_CONFIG_HOME", ".config");
}

inline auto get_user_cache_dir() -> std::string {
    return get_xdg_path("XDG_CACHE_HOME", ".cache");
}

inline auto get_dictionary_compiler_path() -> std::optional<std::string> {
    const auto result = Process({"/usr/bin/mecab-config", "--libexecdir", nullptr}).join();
    if(result.status.code != 0) {
        return std::nullopt;
    }
    const auto& out = result.out;
    if(out.empty()) {
        return std::nullopt;
    }
    if(out.back() == '\n') {
        return out.substr(0, out.size() - 1);
    } else {
        return out;
    }
}

inline auto u8tou32(const std::string_view u8) -> std::u32string {
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

inline auto u32tou8(const std::u32string_view u32) -> std::string {
    auto u8 = std::string();
    for(const auto c : u32) {
        auto buffer = std::array<char, FCITX_UTF8_MAX_LENGTH>();
        fcitx_ucs4_to_utf8(c, buffer.data());
        u8 += buffer.data();
    }
    return u8;
}

inline auto u32tou8(const char32_t u32) -> std::string {
    auto buffer = std::array<char, FCITX_UTF8_MAX_LENGTH>();
    fcitx_ucs4_to_utf8(u32, buffer.data());
    return buffer.data();
}

inline auto kana_to_romaji(const std::string_view kana) -> std::string {
    for(auto i = uint64_t(0); i < sizeof(romaji_table) / sizeof(RomajiKana); i += 1) {
        if(romaji_table[i].kana == kana) {
            return romaji_table[i].romaji;
        }
    }
    panic("unknown kana passed");
}

inline auto pop_back_u8(std::string& u8) -> void {
    auto u32 = u8tou32(u8);
    u32.pop_back();
    u8 = u32tou8(u32);
}

template <typename T, typename E>
auto contains(const T& vec, const E& elm) -> bool {
    return std::find(vec.begin(), vec.end(), elm) != vec.end();
}

template <typename T, typename E, typename P>
auto contains(const T& vec, const E& elm, P pred) -> bool {
    return std::find_if(vec.begin(), vec.end(), [&elm, &pred](const E& o) { return pred(elm, o); }) != vec.end();
}

template <typename T>
auto emplace_unique(std::vector<T>& vec, const T& elm) -> bool {
    if(contains(vec, elm)) {
        return false;
    } else {
        vec.emplace_back(elm);
        return true;
    }
}

template <typename T, typename P>
auto emplace_unique(std::vector<T>& vec, const T& elm, P pred) -> bool {
    if(contains(vec, elm, pred)) {
        return false;
    } else {
        vec.emplace_back(elm);
        return true;
    }
}
} // namespace mikan
