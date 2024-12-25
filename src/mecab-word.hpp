#pragma once
#include <optional>
#include <string>
#include <vector>

namespace mikan {
struct WordParameters {
    unsigned short cattr_right = 0;
    unsigned short cattr_left  = 0;
    short          word_cost   = 0;
    long           total_cost  = 0;

    auto operator==(const WordParameters& o) const -> bool {
        return o.cattr_right == cattr_right && o.cattr_left == cattr_left;
    }
};

struct MeCabWord {
    std::string                   feature;
    std::optional<WordParameters> params = {};
};

using MeCabWords = std::vector<MeCabWord>;
} // namespace mikan
