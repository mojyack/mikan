project('fcitx5-mikan', 'cpp', version : '1.6.2', license : 'MIT', default_options : ['c_std=c20', 'cpp_std=c++20'])
add_global_arguments('-Wno-non-virtual-dtor', language : 'cpp')
        
subdir('src')

fcitx_dep = [
    dependency('Fcitx5Core'),
    dependency('Fcitx5Utils'),
]

cpp = meson.get_compiler('cpp')
mecab_lib = cpp.find_library('mecab')

shared_module(
    'mikan', mikan_files,
    dependencies : [fcitx_dep, mecab_lib],
    name_prefix : '',
    install : true,
)

install_subdir('data/hicolor/', install_dir : get_option('datadir') / 'icons')
