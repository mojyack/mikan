#pragma once
#include <mecab.h>

class MeCabModel {
  private:
    bool valid = false;

  public:
    MeCab::Model*   model;
    MeCab::Tagger*  tagger;
    MeCab::Lattice* lattice;
    MeCabModel(const char* dictionary, const char* user_dictionary = nullptr);
    MeCabModel() {}
    ~MeCabModel();
};
