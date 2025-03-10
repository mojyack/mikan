# mikan
A japanese input method engine powered by mecab

https://github.com/user-attachments/assets/49903b53-235e-475b-a2fc-8cd7ce664628

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
- Ctrl+Alt+H/J: Take/Give one character from/to previous word
- Ctrl+Alt+L/K: Take/Give one character from/to next word
- Space: Start conversion of the whole sentence and select the next sentence candidate
- Shift+Space: Select previous sentence candidate
- Return: Commit sentence
- Slash: Enter to command mode

# Command mode
Start typing with slash('/') to enter to the command mode.

https://github.com/user-attachments/assets/a9df228e-c886-4379-8466-027f7803cf8d

## /def
Add a new word to a custom dictionary.    
Usage:  
1. Copy the converted word to the clipboard.
2. Type /def(Space)
3. Type the original word then Return.
Registered words are saved in `$HOME/.cache/mikan/defines.txt`
## /undef
Opposite of `/def`.  
Remove an input word from a custom dictionary.  
## /reload
Reload user dictionaries.
