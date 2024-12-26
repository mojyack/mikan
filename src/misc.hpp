#pragma once
#include <optional>
#include <string>
#include <vector>

#include <fcitx/event.h>

namespace mikan {
auto get_user_config_dir() -> std::string;
auto get_user_cache_dir() -> std::string;
auto get_dictionary_compiler_path() -> std::optional<std::string>;
auto u8tou32(std::string_view u8) -> std::u32string;
auto u32tou8(std::u32string_view u32) -> std::string;
auto u32tou8(char32_t u32) -> std::string;
auto kana_to_romaji(std::string_view kana) -> std::optional<std::string>;
auto pop_back_u8(std::string& u8) -> char32_t;
auto press_event_to_single_char(const fcitx::KeyEvent& event) -> std::optional<char>;

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
