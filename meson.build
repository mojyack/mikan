project('fcitx5-mikan', 'cpp', version : '1.6.5', license : 'MIT', default_options : ['c_std=c20', 'cpp_std=c++20'])
add_global_arguments('-Wno-non-virtual-dtor', language : 'cpp')
        
cpp = meson.get_compiler('cpp')

shared_module('mikan',
  files(
    'src/command.cpp',
    'src/context.cpp',
    'src/engine.cpp',
    'src/lib.cpp',
    'src/mecab-model.cpp',
    'src/misc.cpp',
    'src/romaji-index.cpp',
    'src/spawn/process.cpp',
    'src/word.cpp',
  ),
  dependencies : [
    dependency('Fcitx5Core'),
    dependency('Fcitx5Utils'),
    cpp.find_library('mecab'),
  ],
  name_prefix : '',
  install : true,
)

install_data('data/fcitx-mikan.conf', install_dir : get_option('datadir') / 'fcitx5/addon')
install_data('data/mikan.conf', install_dir : get_option('datadir') / 'fcitx5/inputmethod')
install_subdir('data/hicolor/', install_dir : get_option('datadir') / 'icons')
