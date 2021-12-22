#include <array>
#include <optional>

#include <fcitx-utils/utf8.h>

#include "misc.hpp"
#include "romaji-table.hpp"

namespace {
auto get_xdg_path(const char* const xdg_env_name, const char* const fallback_dir_name) -> std::string {
    auto       ret = std::string();
    const auto xdg = std::getenv(xdg_env_name);
    if(xdg == nullptr) {
        ret = std::string(std::getenv("HOME")) + "/" + fallback_dir_name + "/";
    } else {
        ret = std::string(xdg) + "/";
    }
    ret.append("mikan/");
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
    constexpr auto COMMAND    = "mecab-config --libexecdir";
    constexpr auto BUFFER_LEN = 128;

    auto buffer = std::array<char, BUFFER_LEN>();
    auto result = std::string();

    auto pipe = popen(COMMAND, "r");

    if(!pipe) {
        return std::nullopt;
    }
    while(fgets(buffer.data(), BUFFER_LEN, pipe) != NULL) {
        result += buffer.data();
    }
    if(!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    const auto return_code = pclose(pipe);
    if(return_code != 0) {
        return std::nullopt;
    } else {
        return result;
    }
}
auto u8tou32(const std::string& u8) -> std::u32string {
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
auto u32tou8(const std::u32string& u32) -> std::string {
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
auto kana_to_romaji(const std::string& kana) -> std::string {
    for(auto i = uint64_t(0); i < romaji_table_limit; i += 1) {
        if(romaji_table[i].kana == kana) {
            return romaji_table[i].romaji;
        }
    }
    // printf("unknown %s\n", kana.data());
    throw std::runtime_error("unknown kana passed.");
}
auto pop_back_u8(std::string& u8) -> void {
    auto u32 = u8tou32(u8);
    u32.pop_back();
    u8 = u32tou8(u32);
}
} // namespace mikan
