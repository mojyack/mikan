#pragma once
#include <string>
#include <vector>

namespace mikan {
struct RomajiKana {
    const std::string romaji;
    const char*       kana;
    const char*       refill = nullptr;
};
struct HiraKata {
    const std::u32string hiragana;
    const char*          katakana;
};
extern const RomajiKana romaji_table[];
extern const uint64_t   romaji_table_limit;

extern const HiraKata hiragana_katakana_table[];
extern const uint64_t hiragana_katakana_table_limit;
} // namespace mikan
