#include "candidate.hpp"

namespace mikan {
    auto Candidates::move_index(const int val) -> void {
    index = (index + val) % get_data_size();
    }
    auto Candidates::set_index(const uint64_t val) -> void {
    index = val % get_data_size();
    }
    auto  Candidates::       get_index() const -> uint64_t {
        return index;
    }
}
