#pragma once
#include <memory>

#include <mecab.h>

namespace mikan {
struct MeCabModel {
    const bool                      is_valid             = false;
    const bool                      is_system_dictionary = false;
    std::unique_ptr<MeCab::Model>   model;
    std::unique_ptr<MeCab::Tagger>  tagger;
    std::unique_ptr<MeCab::Lattice> lattice;

    MeCabModel() = default;
    MeCabModel(const char* dictionary, const char* user_dictionary, bool is_system_dictionary);
};
} // namespace mikan
