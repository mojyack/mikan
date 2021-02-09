#pragma once
#include <optional>
#include <string>
#include <vector>

std::string                get_user_config_dir();
std::string                get_user_cache_dir();
std::optional<std::string> get_dictionary_compiler_path();
std::u32string             u8tou32(const std::string& u8);
std::string                u32tou8(const std::u32string& u32);
std::string                u32tou8(const char32_t u32);
std::string                kana_to_romaji(const std::string& kana);
void                       pop_back_u8(std::string& u8);
template <typename T, typename E>
bool contains(const T& vec, const E& elm) {
    return std::find(vec.begin(), vec.end(), elm) != vec.end();
}

template <typename T>
bool emplace_unique(std::vector<T>& vec, const T& elm) {
    if(contains(vec, elm)) {
        return false;
    } else {
        vec.emplace_back(elm);
        return true;
    }
}
