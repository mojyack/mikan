#pragma once
#include <string>
#include <vector>

#include "romaji-table.hpp"
#include "util.hpp"

namespace mikan {
struct RomajiIndex {
  private:
    std::vector<size_t> cache;
    std::string         cache_source;

    auto search_by_nth_chara(const char chara, const size_t n, const RomajiKana** const exact) const -> std::vector<size_t> {
        auto r = std::vector<size_t>();
        if(n == 0) {
            for(auto i = std::begin(romaji_table); i < std::end(romaji_table); i += 1) {
                const auto& romaji = i->romaji;
                if(romaji[0] == chara) {
                    if(romaji.size() == n + 1) {
                        *exact = i;
                        return r;
                    }
                    r.emplace_back(std::end(romaji_table) - i);
                }
            }
        } else {
            for(const auto i : cache) {
                const auto& romaji = romaji_table[i].romaji;
                if(romaji.size() <= n) {
                    continue;
                }
                if(romaji[n] == chara) {
                    if(romaji.size() == n + 1) {
                        *exact = &romaji_table[i];
                        return r;
                    }
                    r.emplace_back(i);
                }
            }
        }
        return r;
    }

  public:
    struct MultiResult {};
    struct InvalidParam {};
    struct EmptyCache {};
    struct ExactOne {
        const RomajiKana* result;
    };
    using FilterResult = Variant<MultiResult, InvalidParam, EmptyCache, ExactOne>;

    auto filter(const std::string& to_kana) -> FilterResult {
        if(to_kana.empty()) {
            return InvalidParam{};
        }

        auto use_cache = true;
        if(!cache_source.empty() && to_kana.size() == cache_source.size() + 1) {
            for(auto i = size_t(0); i < cache_source.size(); i += 1) {
                if(cache_source[i] != to_kana[i]) {
                    use_cache = false;
                    break;
                }
            }
        } else {
            use_cache = false;
        }

        auto exact = (const RomajiKana*)nullptr;
        if(!use_cache) {
            cache.clear();
            for(auto i = size_t(0); i < to_kana.size(); i += 1) {
                cache = search_by_nth_chara(to_kana[i], i, &exact);
                if(exact != nullptr) {
                    goto end;
                }
            }
        } else {
            exact = (const RomajiKana*)nullptr;
            cache = search_by_nth_chara(to_kana.back(), to_kana.size() - 1, &exact);
            if(exact != nullptr) {
                goto end;
            }
        }
        if(cache.size() == 0) {
            return EmptyCache{};
        }
        if(cache.size() == 1) {
            exact = &romaji_table[cache[0]];
        }
    end:
        if(exact != nullptr) {
            cache_source.clear();
            return ExactOne{exact};
        } else {
            cache_source = to_kana;
            return MultiResult{};
        }
    }
};
} // namespace mikan
