#include <optional>

#include <fcitx-utils/utf8.h>

#include "misc.hpp"
#include "romaji-table.hpp"

namespace {
std::string get_xdg_path(const char* xdg_env_name, const char* fallback_dir_name) {
    std::string ret;
    auto        xdg = std::getenv(xdg_env_name);
    if(xdg == nullptr) {
        ret = std::string(std::getenv("HOME")) + "/" + fallback_dir_name + "/";
    } else {
        ret = std::string(xdg) + "/";
    }
    ret.append("mikan/");
    return ret;
}
} // namespace
std::string get_user_config_dir() {
    return get_xdg_path("XDG_CONFIG_HOME", ".config");
}
std::string get_user_cache_dir() {
    return get_xdg_path("XDG_CACHE_HOME", ".cache");
}
std::optional<std::string> get_dictionary_compiler_path() {
    constexpr const char*        COMMAND    = "mecab-config --libexecdir";
    constexpr size_t             BUFFER_LEN = 128;
    std::array<char, BUFFER_LEN> buffer;
    std::string                  result;

    FILE* pipe = popen(COMMAND, "r");
    if(!pipe) {
        return std::nullopt;
    }
    while(fgets(buffer.data(), BUFFER_LEN, pipe) != NULL) {
        result += buffer.data();
    }
    if(!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    auto return_code = pclose(pipe);
    if(return_code != 0) {
        return std::nullopt;
    } else {
        return result;
    }
}
std::u32string u8tou32(const std::string& u8) {
    std::u32string u32;

    auto   p        = &u8[0];
    size_t len_left = u8.size();
    while(len_left) {
        int len_read;
        u32 += fcitx_utf8_get_char_validated(p, len_left, &len_read);
        p += len_read;
        len_left -= len_read;
    }
    return u32;
}
std::string u32tou8(const std::u32string& u32) {
    std::string u8;
    for(auto c : u32) {
        char buffer[FCITX_UTF8_MAX_LENGTH] = {};
        fcitx_ucs4_to_utf8(c, buffer);
        u8 += buffer;
    }
    return u8;
}
std::string u32tou8(const char32_t u32) {
    char buffer[FCITX_UTF8_MAX_LENGTH] = {};
    fcitx_ucs4_to_utf8(u32, buffer);
    return buffer;
}
std::string kana_to_romaji(const std::string& kana) {
    for(uint64_t i = 0; i < romaji_table_limit; ++i) {
        if(romaji_table[i].kana == kana) {
            return u32tou8(romaji_table[i].romaji);
        }
    }
    printf("unknown %s\n", kana.data());
    throw std::runtime_error("unknown kana passed.");
    return std::string();
}
void pop_back_u8(std::string& u8) {
    auto u32 = u8tou32(u8);
    u32.pop_back();
    u8 = u32tou8(u32);
}
