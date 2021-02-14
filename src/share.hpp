#pragma once
#include <cstdint>
#include <vector>

#include "configuration.hpp"
#include "mecab-model.hpp"
#include "type.hpp"

class Share {
  public:
    int                      candidate_page_size;
    size_t                   auto_commit_threshold;
    SafeVar<MeCabModel*>     primary_vocabulary = nullptr;
    std::vector<MeCabModel*> additional_vocabularies;
    KeyConfig                key_config;
};
