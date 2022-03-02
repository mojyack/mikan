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
    auto get_feature() const -> const std::string& {
        return feature;
    }
    auto get_mutable_feature() -> std::string& {
        return feature;
    }
    auto has_valid_word_info() const -> bool {
        return is_word_info_valid;
    }
    auto has_same_attr(const MeCabWord& o) const -> bool {
        return is_word_info_valid && o.cattr_right == cattr_right && o.cattr_left == cattr_left;
    }
    auto get_cattr_right() const -> unsigned short {
        return cattr_right;
    }
    auto get_cattr_left() const -> unsigned short {
        return cattr_left;
    }
    auto get_word_cost() const -> short {
        return is_word_info_valid ? word_cost : 0;
    }
    auto override_word_cost(const short cost) -> void {
        word_cost = cost;
    }
    auto get_total_cost() const -> long {
        return is_word_info_valid ? total_cost : 0;
    }

    MeCabWord() = default;
    MeCabWord(const std::string& feature) : feature(feature) {}
    MeCabWord(const std::string& feature, const short cattr_right, const short cattr_left, const short word_cost, const long total_cost, const bool from_system_dic)
        : feature(feature),
          is_word_info_valid(true),
          cattr_right(from_system_dic ? cattr_right : 0),
          cattr_left(from_system_dic ? cattr_left : 0),
          word_cost(from_system_dic ? word_cost : 0),
          total_cost(from_system_dic ? total_cost : 0) {}
};

using MeCabWords = std::vector<MeCabWord>;
} // namespace mikan
