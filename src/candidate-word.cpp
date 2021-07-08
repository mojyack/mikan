#include "candidate.hpp"

namespace mikan {
auto CandidateWord::select(fcitx::InputContext* inputContext) const -> void {}
CandidateWord::CandidateWord(fcitx::Text text) : fcitx::CandidateWord(std::move(text)){};
} // namespace mikan
