#include <filesystem>
#include <string>

#include "mecab-model.hpp"

namespace mikan {
MeCabModel::MeCabModel(const char* const dictionary, const char* const user_dictionary, const bool is_system_dictionary) : is_system_dictionary(is_system_dictionary) {
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
    valid   = true;
}
MeCabModel::~MeCabModel() {
    if(!valid) {
        return;
    }
    delete lattice;
    delete tagger;
    delete model;
}
} // namespace mikan
