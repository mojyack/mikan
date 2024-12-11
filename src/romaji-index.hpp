#pragma once
#include <string>
#include <vector>

#include "romaji-table.hpp"
#include "util/variant.hpp"

namespace mikan {
struct RomajiIndex {
  private:
    std::vector<size_t> cache;
    std::string         cache_source;

    auto search_by_nth_chara(char chara, size_t n, const RomajiKana** exact) const -> std::vector<size_t>;

  public:
    struct MultiResult {};
    struct InvalidParam {};
    struct EmptyCache {};
    struct ExactOne {
        const RomajiKana* result;
    };
    using FilterResult = Variant<MultiResult, InvalidParam, EmptyCache, ExactOne>;

    auto filter(std::string_view to_kana) -> FilterResult;
};
} // namespace mikan
