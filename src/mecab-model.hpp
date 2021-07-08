#pragma once
#include <mecab.h>

namespace mikan {
class MeCabModel {
  private:
    bool valid = false;

  public:
    const bool is_system_dictionary = false;
    MeCab::Model*   model;
    MeCab::Tagger*  tagger;
    MeCab::Lattice* lattice;
    MeCabModel(const char* dictionary, const char* user_dictionary = nullptr, bool is_system_dictionary = false);
    MeCabModel() = default;
    ~MeCabModel();
};
}
