#include <filesystem>
#include <string>

#include "macros/assert.hpp"
#include "mecab-model.hpp"

namespace mikan {
MeCabModel::MeCabModel(const char* const dictionary, const char* const user_dictionary, const bool is_system_dictionary)
    : is_valid(true),
      is_system_dictionary(is_system_dictionary) {
    auto dict_path = std::string("-d ") + dictionary;
    if(user_dictionary != nullptr && std::filesystem::is_regular_file(user_dictionary)) {
        dict_path += " -u ";
        dict_path += user_dictionary;
    }
    model.reset(MeCab::createModel(dict_path.data()));
    ASSERT(model != nullptr, "failed to load system dictionary");
    tagger.reset(model->createTagger());
    lattice.reset(model->createLattice());
}
} // namespace mikan
