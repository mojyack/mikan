#pragma once
#include "configuration.hpp"
#include "mecab-model.hpp"
#include "util.hpp"

namespace mikan {
enum class InsertSpaceOptions {
    None,
    On,
    Off,
    Smart,
};

class Share {
  public:
    size_t                   auto_commit_threshold   = 8;
    std::string              dictionary_path         = "/usr/share/mikan-im/dic";
    int                      candidate_page_size     = 10;
    InsertSpaceOptions       insert_space            = InsertSpaceOptions::Smart;
    std::vector<MeCabModel*> additional_vocabularies = {};
    KeyConfig                key_config              = {};
    Critical<MeCabModel*>    primary_vocabulary      = nullptr;
};
} // namespace mikan
