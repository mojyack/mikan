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

inline const auto romaji_table = std::array<RomajiKana,312> {{
    {"a", "あ"}, {"i", "い"}, {"u", "う"}, {"e", "え"}, {"o", "お"},
    {"ka", "か"}, {"ki", "き"}, {"ku", "く"}, {"ke", "け"}, {"ko", "こ"},
    {"sa", "さ"}, {"si", "し"}, {"su", "す"}, {"se", "せ"}, {"so", "そ"},
    {"ta", "た"}, {"ti", "ち"}, {"tu", "つ"}, {"te", "て"}, {"to", "と"},
    {"na", "な"}, {"ni", "に"}, {"nu", "ぬ"}, {"ne", "ね"}, {"no", "の"},
    {"ha", "は"}, {"hi", "ひ"}, {"hu", "ふ"}, {"he", "へ"}, {"ho", "ほ"},
    {"ma", "ま"}, {"mi", "み"}, {"mu", "む"}, {"me", "め"}, {"mo", "も"},
    {"ya", "や"}, {"yi", "い"}, {"yu", "ゆ"}, {"ye", "いぇ"}, {"yo", "よ"},
    {"ra", "ら"}, {"ri", "り"}, {"ru", "る"}, {"re", "れ"}, {"ro", "ろ"},
    {"wa", "わ"}, {"wi", "うぃ"}, {"wu", "う"}, {"we", "うぇ"}, {"wo", "を"},
    {"nn", "ん"},
    {"za", "ざ"}, {"zi", "じ"}, {"zu", "ず"}, {"ze", "ぜ"}, {"zo", "ぞ"},
    {"ja", "じゃ"}, {"ji", "じ"}, {"ju", "じゅ"}, {"je", "じぇ"}, {"jo", "じょ"},
    {"jya", "じゃ"}, {"jyi", "じ"}, {"jyu", "じゅ"}, {"jye", "じぇ"}, {"jyo", "じょ"},
    {"jha", "じゃ"}, {"jhi", "じ"}, {"jhu", "じゅ"}, {"jhe", "じぇ"}, {"jho", "じょ"},
    {"ga", "が"}, {"gi", "ぎ"}, {"gu", "ぐ"}, {"ge", "げ"}, {"go", "ご"},
    {"da", "だ"}, {"di", "ぢ"}, {"du", "づ"}, {"de", "で"}, {"do", "ど"},
    {"ba", "ば"}, {"bi", "び"}, {"bu", "ぶ"}, {"be", "べ"}, {"bo", "ぼ"},
    {"pa", "ぱ"}, {"pi", "ぴ"}, {"pu", "ぷ"}, {"pe", "ぺ"}, {"po", "ぽ"},
    {"fa", "ふぁ"}, {"fi", "ふぃ"}, {"fu", "ふ"}, {"fe", "ふぇ"}, {"fo", "ふぉ"},
    {"kya", "きゃ"}, {"kyi", "きぃ"}, {"kyu", "きゅ"}, {"kye", "きぇ"}, {"kyo", "きょ"},
    {"gya", "ぎゃ"}, {"gyi", "ぎぃ"}, {"gyu", "ぎゅ"}, {"gye", "ぎぇ"}, {"gyo", "ぎょ"},
    {"bya", "びゃ"}, {"byi", "びぃ"}, {"byu", "びゅ"}, {"bye", "びぇ"}, {"byo", "びょ"},
    {"cha", "ちゃ"}, {"chi","ち"}, {"chu","ちゅ"}, {"che","ちぇ"}, {"cho","ちょ"},
    {"cya", "ちゃ"}, {"cyi","ち"}, {"cyu","ちゅ"}, {"cye","ちぇ"}, {"cyo","ちょ"},
    {"tya", "ちゃ"}, {"tyi","ちぃ"}, {"tyu","ちゅ"}, {"tye","ちぇ"}, {"tyo","ちょ"},
    {"sya", "しゃ"}, {"syi", "しぃ"}, {"syu", "しゅ"}, {"sye", "しぇ"}, {"syo", "しょ"},
    {"sha", "しゃ"}, {"shi","し"}, {"shu","しゅ"}, {"she","しぇ"}, {"sho","しょ"},
    {"nya", "にゃ"}, {"nyi", "にぃ"}, {"nyu", "にゅ"}, {"nye", "にぇ"}, {"nyo", "にょ"},
    {"rya", "りゃ"}, {"ryi", "りぃ"}, {"ryu", "りゅ"}, {"rye", "りぇ"}, {"ryo", "りょ"},
    {"mya", "みゃ"}, {"myi", "みぃ"}, {"myu", "みゅ"}, {"mye", "みぇ"}, {"myo", "みょ"},
    {"pya", "ぴゃ"}, {"pyi", "ぴぃ"}, {"pyu", "ぴゅ"}, {"pye", "ぴぇ"}, {"pyo", "ぴょ"},
    {"dda", "っだ"}, {"ddi", "っぢ"}, {"ddu", "っづ"}, {"dde", "っで"}, {"ddo", "っど"},
    {"gga", "っが"}, {"ggi", "っぎ"}, {"ggu", "っぐ"}, {"gge", "っげ"}, {"ggo", "っご"},
    {"ppa", "っぱ"}, {"ppi", "っぴ"}, {"ppu", "っぷ"}, {"ppe", "っぺ"}, {"ppo", "っぽ"},
    {"ssa", "っさ"}, {"ssi", "っし"}, {"ssu", "っす"}, {"sse", "っせ"}, {"sso", "っそ"},
    {"hya", "ひゃ"}, {"hyi", "ひぃ"}, {"hyu", "ひゅ"}, {"hye", "ひぇ"}, {"hyo", "ひょ"},
    {"ffa", "っふぁ"}, {"ffi", "っふぃ"}, {"ffu", "っふ"}, {"ffe", "っふぇ"}, {"ffo", "っふぉ"},
    {"ttya", "っちゃ"}, {"ttyi", "っちぃ"}, {"ttyu", "っちゅ"}, {"ttye", "っちぇ"}, {"ttyo", "っちょ"},
    {"kkya", "っきゃ"}, {"kkyi", "っきぃ"}, {"kkyu", "っきゅ"}, {"kkye", "っきぇ"}, {"kkyo", "っきょ"},
    {"ssya", "っしゃ"}, {"ssyi", "っしぃ"}, {"ssyu", "っしゅ"}, {"ssye", "っしぇ"}, {"ssyo", "っしょ"},
    {"hhya", "っひゃ"}, {"hhyi", "っひ"}, {"hhyu", "っひゅ"}, {"hhye", "っひぇ"}, {"hhyo", "っひょ"},
    {"ppya", "っぴゃ"}, {"ppyi", "っぴぃ"}, {"ppyu", "っぴゅ"}, {"ppye", "っぴぇ"}, {"ppyo", "っぴょ"},
    {"va", "ゔぁ"}, {"vi","ゔぃ"}, {"vu","ゔ"}, {"ve","ゔぇ"}, {"vo","ゔぉ"},
    {"la", "ぁ"}, {"li","ぃ"}, {"lu","ぅ"}, {"le","ぇ"}, {"lo","ぉ"},
    {"lya", "ゃ"}, {"lyi","ぃ"}, {"lyu","ゅ"}, {"lye", "ぇ"}, {"lyo","ょ"},
    {"ltu", "っ"},
    {"kka", "っか"}, {"kki", "っき"}, {"kku", "っく"}, {"kke", "っけ"}, {"kko", "っこ"},
    {"tta", "った"}, {"tti", "っち"}, {"ttu", "っつ"}, {"tte", "って"}, {"tto", "っと"},
    {"hha", "っは"}, {"hhi", "っひ"}, {"hhu", "っふ"}, {"hhe", "っへ"}, {"hho", "っほ"},
    {"zza", "っざ"}, {"zzi", "っじ"}, {"zzu", "っず"}, {"zze", "っぜ"}, {"zzo", "っぞ"},
    {"jja", "っじゃ"}, {"jji", "っじ"}, {"jju", "っじゅ"}, {"jje", "っじぇ"}, {"jjo", "っじょ"},
    {"jjya", "っじゃ"}, {"jjyi", "っじ"}, {"jjyu", "っじゅ"}, {"jjye", "っじぇ"}, {"jjyo", "っじょ"},
    {"nk", "ん", "k"}, {"ns", "ん", "s"}, {"nt", "ん", "t"}, {"nh", "ん", "h"}, {"nm", "ん", "m"}, {"nr", "ん", "r"}, {"nw", "ん", "w"},
    {"nz", "ん", "z"}, {"nj", "ん", "j"}, {"ng", "ん", "g"}, {"nd", "ん", "d"}, {"nb", "ん", "b"}, {"np", "ん", "p"}, {"nf", "ん", "f"}, {"nv", "ん", "v"},
    {"`", "`"}, {"1", "1"}, {"2", "2"}, {"3", "3"}, {"4", "4"}, {"5", "5"}, {"6", "6"}, {"7", "7"}, {"8", "8"}, {"9", "9"}, {"0", "0"}, {"-", "ー"}, {"=", "="},
    {"~", "~"}, {"!", "!"}, {"@", "@"}, {"#", "#"}, {"$", "$"}, {"%", "%"}, {"^", "^"}, {"&", "&"}, {"*", "*"}, {"(", "("}, {")", ")"}, {"_", "_"}, {"+", "+"},
    {"[", "「"}, {"]", "」"}, {"\\", "\\"}, {"{", "{"}, {"}", "}"}, {"|", "|"},
    {";", ";"}, {":", ":"}, {"'", "'"}, {"\"", "\""}, {")", ")"}, {"_", "_"}, {"+", "+"},
    {",", "、"}, {"<", "<"}, {".", "。"}, {">", ">"}, {"/", "/"}, {"?", "?"},
}};

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
    {U'ぱ', U'パ'}, {U'ぴ', U'ピ'}, {U'ぷ', U'プ'}, {U'ぺ', U'ペ'}, {U'ぽ', U'ポ'},
    {U'ぁ', U'ァ'}, {U'ぃ', U'ィ'}, {U'ぅ', U'ゥ'}, {U'ぇ', U'ェ'}, {U'ぉ', U'ォ'},
    {U'っ', U'ッ'},
    {U'ゃ', U'ャ'}, {U'ぃ', U'ィ'}, {U'ゅ', U'ュ'}, {U'ぇ', U'ェ'}, {U'ょ', U'ョ'},
    {U'ゃ', U'ャ'}, {U'ぃ', U'ィ'}, {U'ゅ', U'ュ'}, {U'ぇ', U'ェ'}, {U'ょ', U'ョ'},
    {U'ゔ', U'ヴ'},
};
} // namespace mikan
