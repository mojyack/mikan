#pragma once
#include <fcitx/candidatelist.h>

namespace mikan {
class Candidates {
  protected:
    uint64_t index; // TODO: size_t

  public:
    auto move_index(const int val) -> void {
        index = (index + val) % get_data_size();
    }

    auto set_index(const uint64_t val) -> void {
        index = val % get_data_size();
    }

    auto get_index() const -> uint64_t {
        return index;
    }

    virtual auto get_data_size() const -> size_t                = 0;
    virtual auto get_labels() const -> std::vector<std::string> = 0;

    Candidates()          = default;
    virtual ~Candidates() = default;
};

} // namespace mikan
