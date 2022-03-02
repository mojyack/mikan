#pragma once
#include <filesystem>
#include <string>

#include <mecab.h>

namespace mikan {
struct MeCabModel {
    const bool      is_valid             = false;
    const bool      is_system_dictionary = false;
    MeCab::Model*   model;
    MeCab::Tagger*  tagger;
    MeCab::Lattice* lattice;

    MeCabModel() = default;
    MeCabModel(const char* const dictionary, const char* const user_dictionary, const bool is_system_dictionary) : is_valid(true), is_system_dictionary(is_system_dictionary) {
        auto dict_path = std::string("-d ") + dictionary;
        if(user_dictionary != nullptr && std::filesystem::is_regular_file(user_dictionary)) {
            dict_path += " -u ";
            dict_path += user_dictionary;
        }
        model = MeCab::createModel(dict_path.data());
        if(model == nullptr) {
            throw std::runtime_error("failed to load system dictionary.");
        }
        tagger  = model->createTagger();
        lattice = model->createLattice();
    }
    ~MeCabModel() {
        if(is_valid) {
            delete lattice;
            delete tagger;
            delete model;
        }
    }
};
} // namespace mikan
