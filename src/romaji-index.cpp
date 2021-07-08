#include <fcitx-utils/utf8.h>

#include "romaji-index.hpp"
#include "romaji-table.hpp"

namespace mikan {
auto RomajiIndex::search_by_nth_chara(const char chara, const size_t n, const RomajiKana** const exact) const -> std::vector<size_t> {
    auto r = std::vector<size_t>();
    if(n == 0) {
        for(size_t i = 0; i < romaji_table_limit; i += 1) {
            const auto& romaji = romaji_table[i].romaji;
            if(romaji[0] == chara) {
                if(romaji.size() == n + 1) {
                    *exact = &romaji_table[i];
                    return r;
                }
                r.emplace_back(i);
            }
        }
    } else {
        for(auto i : cache) {
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
auto RomajiIndex::filter(const std::string& to_kana) -> const RomajiKana* {
    if(to_kana.empty()) {
        return nullptr;
    }

    bool use_cache = true;
    if(!cache_source.empty() && to_kana.size() == cache_source.size() + 1) {
        for(size_t i = 0; i < cache_source.size(); ++i) {
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
        for(size_t i = 0; i < to_kana.size(); i += 1) {
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
        return &romaji_table[romaji_table_limit];
    }
    if(cache.size() == 1) {
        exact = &romaji_table[cache[0]];
    }
end:
    if(exact != nullptr) {
        cache_source.clear();
    } else {
        cache_source = to_kana;
    }
    return exact;
}
} // namespace mikan
