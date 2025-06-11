#include <QApplication>
#include "MusicPlayer.h"
#include <clocale>
#include <iostream>

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);
    setlocale(LC_NUMERIC, "C");

    MusicPlayer player;
    player.show();

    return app.exec();
}
