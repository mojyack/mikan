# mikan
A japanese input method engine powered by mecab

https://user-images.githubusercontent.com/66899529/228751373-f00910ae-6932-439b-9258-e1c76ea9a6d8.mp4

# Dependencies
- fcitx 5
- mecab
# Install
If you are using Arch Linux, these packages are available in AUR.  
Just install `mecab-shogo82148-git`,`mikan-dictionary-git`,`fcitx5-mikan-git`
## Build mecab
```
git clone --depth=1 https://github.com/shogo82148/mecab.git
cd mecab/mecab
./autogen.sh
./configure
make -j$(nproc)
make install
```
## Build dictionary
```
git clone https://github.com/mojyack/mikan-dictionary.git
cd mikan-dictionary
./build.sh
./install.sh /usr/share/mikan-im
```
## Build mikan
```
git clone --recursive https://github.com/mojyack/mikan.git
cd mikan
meson setup -Dprefix=/usr -Dlibdir=lib/fcitx5 -Dbuildtype=release build
ninja -C build install
```

# Configurations
Copy `docs/mikan.conf` to `$HOME/.config/mikan.conf` and modify it.  

# Keybinds
mikan uses vim-inspired keybinds.
- Ctrl+H/L: Select previous/next word
- Ctrl+J/K: Select previous/next word conversion candidate
- Alt+H/L: Split first/last one character from current word
- Alt+J/K: Merge previous/next and current word
- Ctrl+Alt+H/L: Move separater between current and next words to left/right
- Space: Start conversion of the whole sentence and select the next sentence candidate
- Shift+Space: Select previous sentence candidate
- Return: Commit sentence
