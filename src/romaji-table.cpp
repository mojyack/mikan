#include "romaji-table.hpp"

namespace mikan {
const RomajiKana romaji_table[] = {
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
    {"va", "う゛ぁ"}, {"vi","う゛ぃ"}, {"vu","う゛"}, {"ve","う゛ぇ"}, {"vo","う゛ぉ"},
    {"la", "ぁ"}, {"li","ぃ"}, {"lu","ぅ"}, {"le","ぇ"}, {"lo","ぉ"},
    {"lya", "ゃ"}, {"lyi","ぃ"}, {"lyu","ゅ"}, {"lye", "ぇ"}, {"lyo","ょ"},
    {"ltu", "っ"},
    {"kka", "っか"}, {"kki", "っき"}, {"kku", "っく"}, {"kke", "っけ"}, {"kko", "っこ"},
    {"tta", "った"}, {"tti", "っち"}, {"ttu", "っつ"}, {"tte", "って"}, {"tto", "っと"},
    {"hha", "っは"}, {"hhi", "っひ"}, {"hhu", "っふ"}, {"hhe", "っへ"}, {"hho", "っほ"},
    {"nk", "ん", "k"}, {"ns", "ん", "s"}, {"nt", "ん", "t"}, {"nh", "ん", "h"}, {"nm", "ん", "m"}, {"nr", "ん", "r"}, {"nw", "ん", "w"},
    {"nz", "ん", "z"}, {"nj", "ん", "j"}, {"ng", "ん", "g"}, {"nd", "ん", "d"}, {"nb", "ん", "b"}, {"np", "ん", "p"}, {"nf", "ん", "f"}, {"nv", "ん", "v"},
    {"`", "`"}, {"1", "1"}, {"2", "2"}, {"3", "3"}, {"4", "4"}, {"5", "5"}, {"6", "6"}, {"7", "7"}, {"8", "8"}, {"9", "9"}, {"0", "0"}, {"-", "ー"}, {"=", "="},
    {"~", "~"}, {"!", "!"}, {"@", "@"}, {"#", "#"}, {"$", "$"}, {"%", "%"}, {"^", "^"}, {"&", "&"}, {"*", "*"}, {"(", "("}, {")", ")"}, {"_", "_"}, {"+", "+"},
    {"[", "「"}, {"]", "」"}, {"\\", "\\"}, {"{", "{"}, {"}", "}"}, {"|", "|"},
    {";", ";"}, {":", ":"}, {"'", "'"}, {"\"", "\""}, {")", ")"}, {"_", "_"}, {"+", "+"},
    {",", "、"}, {"<", "<"}, {".", "。"}, {">", ">"}, {"/", "/"}, {"?", "?"},
};
const uint64_t romaji_table_limit = sizeof(romaji_table) / sizeof(romaji_table[0]);

const HiraKata hiragana_katakana_table[] = {
    {U"あ", "ア"}, {U"い", "イ"}, {U"う", "ウ"}, {U"え", "エ"}, {U"お", "オ"},
    {U"か", "カ"}, {U"き", "キ"}, {U"く", "ク"}, {U"け", "ケ"}, {U"こ", "コ"},
    {U"さ", "サ"}, {U"し", "シ"}, {U"す", "ス"}, {U"せ", "セ"}, {U"そ", "ソ"},
    {U"た", "タ"}, {U"ち", "チ"}, {U"つ", "ツ"}, {U"て", "テ"}, {U"と", "ト"},
    {U"な", "ナ"}, {U"に", "ニ"}, {U"ぬ", "ヌ"}, {U"ね", "ネ"}, {U"の", "ノ"},
    {U"は", "ハ"}, {U"ひ", "ヒ"}, {U"ふ", "フ"}, {U"へ", "ヘ"}, {U"ほ", "ホ"},
    {U"ま", "マ"}, {U"み", "ミ"}, {U"む", "ム"}, {U"め", "メ"}, {U"も", "モ"},
    {U"や", "ヤ"}, {U"い", "イ"}, {U"ゆ", "ユ"}, {U"え", "エ"}, {U"よ", "ヨ"},
    {U"ら", "ラ"}, {U"り", "リ"}, {U"る", "ル"}, {U"れ", "レ"}, {U"ろ", "ロ"},
    {U"わ", "ワ"}, {U"い", "イ"}, {U"う", "ウ"}, {U"え", "エ"}, {U"を", "ヲ"}, {U"ん", "ン"},
    {U"が", "ガ"}, {U"ぎ", "ギ"}, {U"ぐ", "グ"}, {U"げ", "ゲ"}, {U"ご", "ゴ"},
    {U"ざ", "ザ"}, {U"じ", "ジ"}, {U"ず", "ズ"}, {U"ぜ", "ゼ"}, {U"ぞ", "ゾ"},
    {U"だ", "ダ"}, {U"ぢ", "ヂ"}, {U"づ", "ヅ"}, {U"で", "デ"}, {U"ど", "ド"},
    {U"ば", "バ"}, {U"び", "ビ"}, {U"ぶ", "ブ"}, {U"べ", "ベ"}, {U"ぼ", "ボ"},
    {U"ぱ", "バ"}, {U"ぴ", "ピ"}, {U"ぷ", "プ"}, {U"ぺ", "ペ"}, {U"ぽ", "ボ"},
    {U"ぁ", "ァ"}, {U"ぃ", "ィ"}, {U"ぅ", "ゥ"}, {U"ぇ", "ェ"}, {U"ぉ", "ォ"},
    {U"っ", "ッ"},
    {U"ゃ", "ャ"}, {U"ぃ", "ィ"}, {U"ゅ", "ュ"}, {U"ぇ", "ェ"}, {U"ょ", "ョ"},
    {U"ゃ", "ャ"}, {U"ぃ", "ィ"}, {U"ゅ", "ュ"}, {U"ぇ", "ェ"}, {U"ょ", "ョ"},
    {U"う゛", "ヴ"},
};
const uint64_t hiragana_katakana_table_limit = sizeof(hiragana_katakana_table) / sizeof(hiragana_katakana_table[0]);
}
