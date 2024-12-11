#pragma once
#include "configuration.hpp"
#include "mecab-model.hpp"

namespace mikan {
enum class InsertSpaceOptions {
    None,
    On,
    Off,
    Smart,
};

class Share {
  public:
    size_t                                   auto_commit_threshold   = 8;
    std::string                              dictionary_path         = "/usr/share/mikan-im/dic";
    int                                      candidate_page_size     = 10;
    InsertSpaceOptions                       insert_space            = InsertSpaceOptions::Smart;
    std::vector<std::unique_ptr<MeCabModel>> additional_vocabularies = {};
    std::shared_ptr<MeCabModel>              primary_vocabulary      = {};
    KeyConfig                                key_config              = {};
};
} // namespace mikan
