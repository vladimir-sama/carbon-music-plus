#!/bin/bash
rm --force moc_MusicPlayer.cpp

/usr/lib/qt6/moc MusicPlayer.h -o moc_MusicPlayer.cpp

g++ -fPIC -std=c++17 main.cpp MusicPlayer.cpp moc_MusicPlayer.cpp -o carbon-music \
    $(pkg-config --cflags --libs Qt6Widgets Qt6Gui Qt6Core) \
    -lmpv -no-pie
