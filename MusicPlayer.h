#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QTimer>
#include <QString>
#include <QMap>
#include <QVector>
#include <mpv/client.h>

struct Track {
    QString title;
    QString url;
};

class MusicPlayer : public QWidget {
    Q_OBJECT

public:
    MusicPlayer(QWidget *parent = nullptr);
    ~MusicPlayer();

private slots:
    void loadSelectedPlaylist();
    void filterTracks();
    void searchYT();
    void selectTrack();
    void togglePlay();
    void setVolume(int value);
    void seekStart();
    void seekEnd();
    void updateSeekSlider();

private:
    void initUI();
    void loadPlaylists();
    void loadPlaylist(const QString &url);
    void updateTrackList();
    void playTrack();

    QComboBox *comboPlaylist;
    QLineEdit *entryFilter;
    QListWidget *listTracks;
    QPushButton *buttonPlay;
    QSlider *sliderVolume;
    QSlider *sliderSeek;
    QLabel *labelTrack;
    QLabel *labelLyrics;
    QTimer *seekTimer;

    mpv_handle *mpv;
    QString trackUrl;
    bool isPaused;
    bool isPlaying;
    float trackLength;
    bool isUserDragging;

    QMap<QString, QString> playlist;
    QStringList playlistTitles;
    QString selectedPlaylist;
    QVector<Track> tracks;
};

#endif // MUSICPLAYER_H
