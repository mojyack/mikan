#pragma once
#include <string>
#include <vector>

namespace mikan {
class MeCabWord {
  private:
    std::string    feature;
    bool           is_word_info_valid = false;
    unsigned short cattr_right        = 0;
    unsigned short cattr_left         = 0;
    short          word_cost          = 0;
    long           total_cost         = 0;

  public:
    auto get_feature() const -> const std::string&;
    auto get_mutable_feature() -> std::string&;
    auto has_valid_word_info() const -> bool;
    auto has_same_attr(const MeCabWord& o) const -> bool;
    auto get_cattr_right() const -> unsigned short;
    auto get_cattr_left() const -> unsigned short;
    auto get_word_cost() const -> short;
    auto override_word_cost(short cost) -> void;
    auto get_total_cost() const -> long;

    MeCabWord(const std::string& feature);
    MeCabWord(const std::string& feature, short cattr_right, short cattr_left, short word_cost, long total_cost, bool from_system_dic);
    MeCabWord() = default;
};

using MeCabWords = std::vector<MeCabWord>;
} // namespace mikan
