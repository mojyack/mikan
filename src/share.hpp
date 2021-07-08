#pragma once
#include "configuration.hpp"
#include "mecab-model.hpp"
#include "thread.hpp"

namespace mikan {
enum class InsertSpaceOptions {
    None,
    On,
    Off,
    Smart,
};

class Share {
  public:
    int                      candidate_page_size;
    size_t                   auto_commit_threshold;
    SafeVar<MeCabModel*>     primary_vocabulary = nullptr;
    std::vector<MeCabModel*> additional_vocabularies;
    KeyConfig                key_config;
    InsertSpaceOptions       insert_space = InsertSpaceOptions::Smart;
};
} // namespace mikan
