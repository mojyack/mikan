mikan_files = files(
    'candidate-list.cpp',
    'candidate-word.cpp',
    'candidate.cpp',
    'configuration.cpp',
    'mecab-model.cpp',
    'mecab-word.cpp',
    'mikan5.cpp',
    'misc.cpp',
    'phrase-candidates.cpp',
    'phrase.cpp',
    'romaji-index.cpp',
    'romaji-table.cpp',
    'sentence-candidates.cpp',
    'state-handlers.cpp',
    'state.cpp',
)

config_data = configuration_data()
config_data.set('dictionary_path', get_option('prefix') / get_option('dictionary_path'))
configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : config_data)
config_include = include_directories('.')

install_data('fcitx-mikan.conf', install_dir : get_option('datadir') / 'fcitx5/addon')
install_data('mikan.conf', install_dir : get_option('datadir') / 'fcitx5/inputmethod')
