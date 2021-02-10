project('fcitx5-mikan', 'cpp', version : '1.1', license : 'MIT', default_options : ['c_std=c2x', 'cpp_std=c++2a'])
        
subdir('src')

fcitx_dep = [
    dependency('Fcitx5Core'),
    dependency('Fcitx5Utils'),
]

cpp = meson.get_compiler('cpp')
mecab_lib = cpp.find_library('mecab')

shared_module(
    'mikan', mikan_files,
    include_directories : config_include,
    dependencies : [fcitx_dep, mecab_lib],
    name_prefix : '',
    install : true,
)

install_subdir('data/hicolor/', install_dir : get_option('datadir') / 'icons')
