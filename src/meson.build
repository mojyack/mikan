mikan_files = files(
    'mikan5.cpp',
    'misc.cpp',
    'candidate.cpp',
    'mecab-model.cpp',
    'romaji-table.cpp',
    'state.cpp',
)

config_data = configuration_data()
config_data.set('dictionary_path', get_option('dictionary_path'))
configure_file(input : 'config.h.in',
               output : 'config.h',
               configuration : config_data)
config_include = include_directories('.')

install_data('fcitx-mikan.conf', install_dir : get_option('datadir') / 'fcitx5/addon')
install_data('mikan.conf', install_dir : get_option('datadir') / 'fcitx5/inputmethod')
