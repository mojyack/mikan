#pragma once
#include <string>
#include <vector>
#include <array>
#include <unordered_map>

namespace mikan {
struct RomajiKana {
    const std::string romaji;
    const char*       kana;
    const char*       refill = nullptr;
};

inline const auto romaji_table = std::array {
    RomajiKana{"a", "あ"}, RomajiKana{"i", "い"}, RomajiKana{"u", "う"}, RomajiKana{"e", "え"}, RomajiKana{"o", "お"},
    RomajiKana{"ka", "か"}, RomajiKana{"ki", "き"}, RomajiKana{"ku", "く"}, RomajiKana{"ke", "け"}, RomajiKana{"ko", "こ"},
    RomajiKana{"sa", "さ"}, RomajiKana{"si", "し"}, RomajiKana{"su", "す"}, RomajiKana{"se", "せ"}, RomajiKana{"so", "そ"},
    RomajiKana{"ta", "た"}, RomajiKana{"ti", "ち"}, RomajiKana{"tu", "つ"}, RomajiKana{"te", "て"}, RomajiKana{"to", "と"},
    RomajiKana{"na", "な"}, RomajiKana{"ni", "に"}, RomajiKana{"nu", "ぬ"}, RomajiKana{"ne", "ね"}, RomajiKana{"no", "の"},
    RomajiKana{"ha", "は"}, RomajiKana{"hi", "ひ"}, RomajiKana{"hu", "ふ"}, RomajiKana{"he", "へ"}, RomajiKana{"ho", "ほ"},
    RomajiKana{"ma", "ま"}, RomajiKana{"mi", "み"}, RomajiKana{"mu", "む"}, RomajiKana{"me", "め"}, RomajiKana{"mo", "も"},
    RomajiKana{"ya", "や"}, RomajiKana{"yi", "い"}, RomajiKana{"yu", "ゆ"}, RomajiKana{"ye", "いぇ"}, RomajiKana{"yo", "よ"},
    RomajiKana{"ra", "ら"}, RomajiKana{"ri", "り"}, RomajiKana{"ru", "る"}, RomajiKana{"re", "れ"}, RomajiKana{"ro", "ろ"},
    RomajiKana{"wa", "わ"}, RomajiKana{"wi", "うぃ"}, RomajiKana{"wu", "う"}, RomajiKana{"we", "うぇ"}, RomajiKana{"wo", "を"},
    RomajiKana{"nn", "ん"},
    RomajiKana{"za", "ざ"}, RomajiKana{"zi", "じ"}, RomajiKana{"zu", "ず"}, RomajiKana{"ze", "ぜ"}, RomajiKana{"zo", "ぞ"},
    RomajiKana{"ja", "じゃ"}, RomajiKana{"ji", "じ"}, RomajiKana{"ju", "じゅ"}, RomajiKana{"je", "じぇ"}, RomajiKana{"jo", "じょ"},
    RomajiKana{"jya", "じゃ"}, RomajiKana{"jyi", "じ"}, RomajiKana{"jyu", "じゅ"}, RomajiKana{"jye", "じぇ"}, RomajiKana{"jyo", "じょ"},
    RomajiKana{"jha", "じゃ"}, RomajiKana{"jhi", "じ"}, RomajiKana{"jhu", "じゅ"}, RomajiKana{"jhe", "じぇ"}, RomajiKana{"jho", "じょ"},
    RomajiKana{"ga", "が"}, RomajiKana{"gi", "ぎ"}, RomajiKana{"gu", "ぐ"}, RomajiKana{"ge", "げ"}, RomajiKana{"go", "ご"},
    RomajiKana{"da", "だ"}, RomajiKana{"di", "ぢ"}, RomajiKana{"du", "づ"}, RomajiKana{"de", "で"}, RomajiKana{"do", "ど"},
    RomajiKana{"ba", "ば"}, RomajiKana{"bi", "び"}, RomajiKana{"bu", "ぶ"}, RomajiKana{"be", "べ"}, RomajiKana{"bo", "ぼ"},
    RomajiKana{"pa", "ぱ"}, RomajiKana{"pi", "ぴ"}, RomajiKana{"pu", "ぷ"}, RomajiKana{"pe", "ぺ"}, RomajiKana{"po", "ぽ"},
    RomajiKana{"fa", "ふぁ"}, RomajiKana{"fi", "ふぃ"}, RomajiKana{"fu", "ふ"}, RomajiKana{"fe", "ふぇ"}, RomajiKana{"fo", "ふぉ"},
    RomajiKana{"kya", "きゃ"}, RomajiKana{"kyi", "きぃ"}, RomajiKana{"kyu", "きゅ"}, RomajiKana{"kye", "きぇ"}, RomajiKana{"kyo", "きょ"},
    RomajiKana{"gya", "ぎゃ"}, RomajiKana{"gyi", "ぎぃ"}, RomajiKana{"gyu", "ぎゅ"}, RomajiKana{"gye", "ぎぇ"}, RomajiKana{"gyo", "ぎょ"},
    RomajiKana{"bya", "びゃ"}, RomajiKana{"byi", "びぃ"}, RomajiKana{"byu", "びゅ"}, RomajiKana{"bye", "びぇ"}, RomajiKana{"byo", "びょ"},
    RomajiKana{"cha", "ちゃ"}, RomajiKana{"chi","ち"}, RomajiKana{"chu","ちゅ"}, RomajiKana{"che","ちぇ"}, RomajiKana{"cho","ちょ"},
    RomajiKana{"cya", "ちゃ"}, RomajiKana{"cyi","ち"}, RomajiKana{"cyu","ちゅ"}, RomajiKana{"cye","ちぇ"}, RomajiKana{"cyo","ちょ"},
    RomajiKana{"tya", "ちゃ"}, RomajiKana{"tyi","ちぃ"}, RomajiKana{"tyu","ちゅ"}, RomajiKana{"tye","ちぇ"}, RomajiKana{"tyo","ちょ"},
    RomajiKana{"sya", "しゃ"}, RomajiKana{"syi", "しぃ"}, RomajiKana{"syu", "しゅ"}, RomajiKana{"sye", "しぇ"}, RomajiKana{"syo", "しょ"},
    RomajiKana{"sha", "しゃ"}, RomajiKana{"shi","し"}, RomajiKana{"shu","しゅ"}, RomajiKana{"she","しぇ"}, RomajiKana{"sho","しょ"},
    RomajiKana{"nya", "にゃ"}, RomajiKana{"nyi", "にぃ"}, RomajiKana{"nyu", "にゅ"}, RomajiKana{"nye", "にぇ"}, RomajiKana{"nyo", "にょ"},
    RomajiKana{"rya", "りゃ"}, RomajiKana{"ryi", "りぃ"}, RomajiKana{"ryu", "りゅ"}, RomajiKana{"rye", "りぇ"}, RomajiKana{"ryo", "りょ"},
    RomajiKana{"mya", "みゃ"}, RomajiKana{"myi", "みぃ"}, RomajiKana{"myu", "みゅ"}, RomajiKana{"mye", "みぇ"}, RomajiKana{"myo", "みょ"},
    RomajiKana{"pya", "ぴゃ"}, RomajiKana{"pyi", "ぴぃ"}, RomajiKana{"pyu", "ぴゅ"}, RomajiKana{"pye", "ぴぇ"}, RomajiKana{"pyo", "ぴょ"},
    RomajiKana{"dda", "っだ"}, RomajiKana{"ddi", "っぢ"}, RomajiKana{"ddu", "っづ"}, RomajiKana{"dde", "っで"}, RomajiKana{"ddo", "っど"},
    RomajiKana{"gga", "っが"}, RomajiKana{"ggi", "っぎ"}, RomajiKana{"ggu", "っぐ"}, RomajiKana{"gge", "っげ"}, RomajiKana{"ggo", "っご"},
    RomajiKana{"ppa", "っぱ"}, RomajiKana{"ppi", "っぴ"}, RomajiKana{"ppu", "っぷ"}, RomajiKana{"ppe", "っぺ"}, RomajiKana{"ppo", "っぽ"},
    RomajiKana{"ssa", "っさ"}, RomajiKana{"ssi", "っし"}, RomajiKana{"ssu", "っす"}, RomajiKana{"sse", "っせ"}, RomajiKana{"sso", "っそ"},
    RomajiKana{"hya", "ひゃ"}, RomajiKana{"hyi", "ひぃ"}, RomajiKana{"hyu", "ひゅ"}, RomajiKana{"hye", "ひぇ"}, RomajiKana{"hyo", "ひょ"},
    RomajiKana{"ffa", "っふぁ"}, RomajiKana{"ffi", "っふぃ"}, RomajiKana{"ffu", "っふ"}, RomajiKana{"ffe", "っふぇ"}, RomajiKana{"ffo", "っふぉ"},
    RomajiKana{"ttya", "っちゃ"}, RomajiKana{"ttyi", "っちぃ"}, RomajiKana{"ttyu", "っちゅ"}, RomajiKana{"ttye", "っちぇ"}, RomajiKana{"ttyo", "っちょ"},
    RomajiKana{"kkya", "っきゃ"}, RomajiKana{"kkyi", "っきぃ"}, RomajiKana{"kkyu", "っきゅ"}, RomajiKana{"kkye", "っきぇ"}, RomajiKana{"kkyo", "っきょ"},
    RomajiKana{"ssya", "っしゃ"}, RomajiKana{"ssyi", "っしぃ"}, RomajiKana{"ssyu", "っしゅ"}, RomajiKana{"ssye", "っしぇ"}, RomajiKana{"ssyo", "っしょ"},
    RomajiKana{"hhya", "っひゃ"}, RomajiKana{"hhyi", "っひ"}, RomajiKana{"hhyu", "っひゅ"}, RomajiKana{"hhye", "っひぇ"}, RomajiKana{"hhyo", "っひょ"},
    RomajiKana{"ppya", "っぴゃ"}, RomajiKana{"ppyi", "っぴぃ"}, RomajiKana{"ppyu", "っぴゅ"}, RomajiKana{"ppye", "っぴぇ"}, RomajiKana{"ppyo", "っぴょ"},
    RomajiKana{"va", "ゔぁ"}, RomajiKana{"vi","ゔぃ"}, RomajiKana{"vu","ゔ"}, RomajiKana{"ve","ゔぇ"}, RomajiKana{"vo","ゔぉ"},
    RomajiKana{"la", "ぁ"}, RomajiKana{"li","ぃ"}, RomajiKana{"lu","ぅ"}, RomajiKana{"le","ぇ"}, RomajiKana{"lo","ぉ"},
    RomajiKana{"lya", "ゃ"}, RomajiKana{"lyi","ぃ"}, RomajiKana{"lyu","ゅ"}, RomajiKana{"lye", "ぇ"}, RomajiKana{"lyo","ょ"},
    RomajiKana{"ltu", "っ"},
    RomajiKana{"kka", "っか"}, RomajiKana{"kki", "っき"}, RomajiKana{"kku", "っく"}, RomajiKana{"kke", "っけ"}, RomajiKana{"kko", "っこ"},
    RomajiKana{"tta", "った"}, RomajiKana{"tti", "っち"}, RomajiKana{"ttu", "っつ"}, RomajiKana{"tte", "って"}, RomajiKana{"tto", "っと"},
    RomajiKana{"hha", "っは"}, RomajiKana{"hhi", "っひ"}, RomajiKana{"hhu", "っふ"}, RomajiKana{"hhe", "っへ"}, RomajiKana{"hho", "っほ"},
    RomajiKana{"zza", "っざ"}, RomajiKana{"zzi", "っじ"}, RomajiKana{"zzu", "っず"}, RomajiKana{"zze", "っぜ"}, RomajiKana{"zzo", "っぞ"},
    RomajiKana{"jja", "っじゃ"}, RomajiKana{"jji", "っじ"}, RomajiKana{"jju", "っじゅ"}, RomajiKana{"jje", "っじぇ"}, RomajiKana{"jjo", "っじょ"},
    RomajiKana{"nk", "ん", "k"}, RomajiKana{"ns", "ん", "s"}, RomajiKana{"nt", "ん", "t"}, RomajiKana{"nh", "ん", "h"}, RomajiKana{"nm", "ん", "m"}, RomajiKana{"nr", "ん", "r"}, RomajiKana{"nw", "ん", "w"},
    RomajiKana{"nz", "ん", "z"}, RomajiKana{"nj", "ん", "j"}, RomajiKana{"ng", "ん", "g"}, RomajiKana{"nd", "ん", "d"}, RomajiKana{"nb", "ん", "b"}, RomajiKana{"np", "ん", "p"}, RomajiKana{"nf", "ん", "f"}, RomajiKana{"nv", "ん", "v"},
    RomajiKana{"`", "`"}, RomajiKana{"1", "1"}, RomajiKana{"2", "2"}, RomajiKana{"3", "3"}, RomajiKana{"4", "4"}, RomajiKana{"5", "5"}, RomajiKana{"6", "6"}, RomajiKana{"7", "7"}, RomajiKana{"8", "8"}, RomajiKana{"9", "9"}, RomajiKana{"0", "0"}, RomajiKana{"-", "ー"}, RomajiKana{"=", "="},
    RomajiKana{"~", "~"}, RomajiKana{"!", "!"}, RomajiKana{"@", "@"}, RomajiKana{"#", "#"}, RomajiKana{"$", "$"}, RomajiKana{"%", "%"}, RomajiKana{"^", "^"}, RomajiKana{"&", "&"}, RomajiKana{"*", "*"}, RomajiKana{"(", "("}, RomajiKana{")", ")"}, RomajiKana{"_", "_"}, RomajiKana{"+", "+"},
    RomajiKana{"[", "「"}, RomajiKana{"]", "」"}, RomajiKana{"\\", "\\"}, RomajiKana{"RomajiKana{", "RomajiKana{"}, RomajiKana{"}", "}"}, RomajiKana{"|", "|"},
    RomajiKana{";", ";"}, RomajiKana{":", ":"}, RomajiKana{"'", "'"}, RomajiKana{"\"", "\""}, RomajiKana{")", ")"}, RomajiKana{"_", "_"}, RomajiKana{"+", "+"},
    RomajiKana{",", "、"}, RomajiKana{"<", "<"}, RomajiKana{".", "。"}, RomajiKana{">", ">"}, RomajiKana{"/", "/"}, RomajiKana{"?", "?"},
};

inline const auto hiragana_katakana_table = std::unordered_map<char32_t, char32_t> {
    {U'あ', U'ア'}, {U'い', U'イ'}, {U'う', U'ウ'}, {U'え', U'エ'}, {U'お', U'オ'},
    {U'か', U'カ'}, {U'き', U'キ'}, {U'く', U'ク'}, {U'け', U'ケ'}, {U'こ', U'コ'},
    {U'さ', U'サ'}, {U'し', U'シ'}, {U'す', U'ス'}, {U'せ', U'セ'}, {U'そ', U'ソ'},
    {U'た', U'タ'}, {U'ち', U'チ'}, {U'つ', U'ツ'}, {U'て', U'テ'}, {U'と', U'ト'},
    {U'な', U'ナ'}, {U'に', U'ニ'}, {U'ぬ', U'ヌ'}, {U'ね', U'ネ'}, {U'の', U'ノ'},
    {U'は', U'ハ'}, {U'ひ', U'ヒ'}, {U'ふ', U'フ'}, {U'へ', U'ヘ'}, {U'ほ', U'ホ'},
    {U'ま', U'マ'}, {U'み', U'ミ'}, {U'む', U'ム'}, {U'め', U'メ'}, {U'も', U'モ'},
    {U'や', U'ヤ'}, {U'い', U'イ'}, {U'ゆ', U'ユ'}, {U'え', U'エ'}, {U'よ', U'ヨ'},
    {U'ら', U'ラ'}, {U'り', U'リ'}, {U'る', U'ル'}, {U'れ', U'レ'}, {U'ろ', U'ロ'},
    {U'わ', U'ワ'}, {U'い', U'イ'}, {U'う', U'ウ'}, {U'え', U'エ'}, {U'を', U'ヲ'}, {U'ん', U'ン'},
    {U'が', U'ガ'}, {U'ぎ', U'ギ'}, {U'ぐ', U'グ'}, {U'げ', U'ゲ'}, {U'ご', U'ゴ'},
    {U'ざ', U'ザ'}, {U'じ', U'ジ'}, {U'ず', U'ズ'}, {U'ぜ', U'ゼ'}, {U'ぞ', U'ゾ'},
    {U'だ', U'ダ'}, {U'ぢ', U'ヂ'}, {U'づ', U'ヅ'}, {U'で', U'デ'}, {U'ど', U'ド'},
    {U'ば', U'バ'}, {U'び', U'ビ'}, {U'ぶ', U'ブ'}, {U'べ', U'ベ'}, {U'ぼ', U'ボ'},
    {U'ぱ', U'バ'}, {U'ぴ', U'ピ'}, {U'ぷ', U'プ'}, {U'ぺ', U'ペ'}, {U'ぽ', U'ポ'},
    {U'ぁ', U'ァ'}, {U'ぃ', U'ィ'}, {U'ぅ', U'ゥ'}, {U'ぇ', U'ェ'}, {U'ぉ', U'ォ'},
    {U'っ', U'ッ'},
    {U'ゃ', U'ャ'}, {U'ぃ', U'ィ'}, {U'ゅ', U'ュ'}, {U'ぇ', U'ェ'}, {U'ょ', U'ョ'},
    {U'ゃ', U'ャ'}, {U'ぃ', U'ィ'}, {U'ゅ', U'ュ'}, {U'ぇ', U'ェ'}, {U'ょ', U'ョ'},
    {U'ゔ', U'ヴ'},
};
} // namespace mikan
