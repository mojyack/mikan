#pragma once
#include <vector>
#include <string>

namespace mikan {
struct RomajiKana;

enum class RomajiFilterStatus {
    EXACT_ONE,
    MULTI_RESULT,
    NO_RESULT,
};

struct RomajiIndex {
  private:
    std::vector<size_t> cache;
    std::string cache_source;

    auto search_by_nth_chara(char chara, size_t n, const RomajiKana** exact) const -> std::vector<size_t>;

  public:
    // returns nullptr if multiple results found or empty string was passed.
    // returns &romaji_table[romaji_table_indexs] if the cache became empty.
    auto filter(const std::string& to_kana) -> const RomajiKana*;
};

} // namespace mikan
