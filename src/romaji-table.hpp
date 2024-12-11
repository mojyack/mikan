#pragma once
#include <string>
#include <unordered_map>

namespace mikan {
struct RomajiKana {
    const std::string romaji;
    const char*       kana;
    const char*       refill = nullptr;
};

inline const RomajiKana romaji_table[] = {
#include "romaji-table.txt"
};

inline const auto hiragana_katakana_table = std::unordered_map<char32_t, char32_t>{
#include "hiragana-katakana-table.txt"
};
} // namespace mikan
