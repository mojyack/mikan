#pragma once
#include <fcitx/candidatelist.h>

namespace mikan {
struct Candidates {
    size_t index = 0;

    auto set_index_wrapped(const size_t val) -> void {
        index = val % get_data_size();
    }

    virtual auto get_data_size() const -> size_t                = 0;
    virtual auto get_labels() const -> std::vector<std::string> = 0;

    Candidates()          = default;
    virtual ~Candidates() = default;
};

} // namespace mikan
