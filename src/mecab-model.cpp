#include <filesystem>
#include <stdexcept>
#include <string>

#include "mecab-model.hpp"

MeCabModel::MeCabModel(const char* dictionary, const char* user_dictionary) {
    std::string dict_path = "-d ";
    dict_path += dictionary;
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
