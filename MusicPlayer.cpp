#include "MusicPlayer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

MusicPlayer::MusicPlayer(QWidget *parent) : QWidget(parent),
    isPaused(false), isPlaying(false), trackLength(0.0f), isUserDragging(false), mpv(nullptr) {

    setWindowTitle("Carbon Music Player Plus");
    setFixedSize(500, 600);
    setStyleSheet("font-family: 'PT Sans Narrow';");

    mpv = mpv_create();
    mpv_set_option_string(mpv, "terminal", "yes");
    mpv_set_option_string(mpv, "video", "no");
    mpv_set_option_string(mpv, "loop", "inf");
    mpv_initialize(mpv);

    loadPlaylists();
    initUI();

    seekTimer = new QTimer(this);
    connect(seekTimer, &QTimer::timeout, this, &MusicPlayer::updateSeekSlider);
    seekTimer->start(50);
}

MusicPlayer::~MusicPlayer() {
    if (mpv) {
        mpv_terminate_destroy(mpv);
    }
}

void MusicPlayer::loadPlaylists() {
    QFile ytFile("playlists_yt.json");
    QFile localFile("playlists_local.json");
    if (ytFile.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(ytFile.readAll()).object();
        for (auto key : obj.keys()) {
            playlist.insert("YT - " + key, obj[key].toString());
        }
    }
    if (localFile.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(localFile.readAll()).object();
        for (auto key : obj.keys()) {
            playlist.insert("LOCAL - " + key, obj[key].toString());
        }
    }
    playlist.insert("SEARCH YT", "SEARCH");
    playlistTitles = playlist.keys();
}

void MusicPlayer::initUI() {
    QVBoxLayout *layout = new QVBoxLayout(this);

    comboPlaylist = new QComboBox();
    comboPlaylist->addItems(playlistTitles);
    connect(comboPlaylist, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MusicPlayer::loadSelectedPlaylist);
    layout->addWidget(comboPlaylist);

    entryFilter = new QLineEdit();
    entryFilter->setPlaceholderText("Search or filter tracks...");
    connect(entryFilter, &QLineEdit::textChanged, this, &MusicPlayer::filterTracks);
    connect(entryFilter, &QLineEdit::returnPressed, this, [this]() {
        if (selectedPlaylist == "SEARCH YT")
            searchYT();
    });
    layout->addWidget(entryFilter);

    listTracks = new QListWidget();
    connect(listTracks, &QListWidget::itemDoubleClicked, this, &MusicPlayer::selectTrack);
    layout->addWidget(listTracks);

    QHBoxLayout *controlLayout = new QHBoxLayout();

    buttonPlay = new QPushButton("Play");
    connect(buttonPlay, &QPushButton::clicked, this, &MusicPlayer::togglePlay);
    controlLayout->addWidget(buttonPlay);

    sliderVolume = new QSlider(Qt::Horizontal);
    sliderVolume->setRange(0, 100);
    sliderVolume->setValue(100);
    connect(sliderVolume, &QSlider::valueChanged, this, &MusicPlayer::setVolume);
    controlLayout->addWidget(sliderVolume);

    sliderSeek = new QSlider(Qt::Horizontal);
    sliderSeek->setRange(0, 100);
    connect(sliderSeek, &QSlider::sliderPressed, this, &MusicPlayer::seekStart);
    connect(sliderSeek, &QSlider::sliderReleased, this, &MusicPlayer::seekEnd);
    controlLayout->addWidget(sliderSeek);

    layout->addLayout(controlLayout);

    labelTrack = new QLabel("(NA)");
    labelTrack->setAlignment(Qt::AlignCenter);
    layout->addWidget(labelTrack);

    comboPlaylist->setCurrentIndex(-1);
}

void MusicPlayer::loadSelectedPlaylist() {
    int index = comboPlaylist->currentIndex();
    if (index < 0) return;
    selectedPlaylist = playlistTitles[index];
    loadPlaylist(playlist[selectedPlaylist]);
}

void MusicPlayer::loadPlaylist(const QString &url) {
    tracks.clear();
    if (url == "SEARCH") {
        updateTrackList();
        return;
    }
    QFileInfo info(url);
    if (info.isDir()) {
        QDir dir(url);
        for (const QString &file : dir.entryList(QDir::Files)) {
            tracks.append({file, dir.absoluteFilePath(file)});
        }
    } else {
        QProcess ytdlp;
        ytdlp.start("yt-dlp", {"--flat-playlist", "--dump-single-json", url});
        ytdlp.waitForFinished();
        QJsonDocument doc = QJsonDocument::fromJson(ytdlp.readAllStandardOutput());
        QJsonArray entries = doc["entries"].toArray();
        for (const QJsonValue &entry : entries) {
            QString title = entry["title"].toString();
            QString vidUrl = entry["url"].toString();
            tracks.append({title, vidUrl});
        }
    }
    updateTrackList();
}

void MusicPlayer::updateTrackList() {
    listTracks->clear();
    for (int i = 0; i < tracks.size(); ++i) {
        listTracks->addItem(QString::number(i + 1) + ". " + tracks[i].title);
    }
}

void MusicPlayer::filterTracks() {
    if (selectedPlaylist == "SEARCH YT") return;
    QString keyword = entryFilter->text().toLower();
    listTracks->clear();
    for (int i = 0; i < tracks.size(); ++i) {
        if (tracks[i].title.toLower().contains(keyword)) {
            listTracks->addItem(QString::number(i + 1) + ". " + tracks[i].title);
        }
    }
}

void MusicPlayer::selectTrack() {
    QListWidgetItem *item = listTracks->currentItem();
    if (!item) return;
    int index = item->text().split(".")[0].toInt() - 1;
    if (index < 0 || index >= tracks.size()) return;
    trackUrl = tracks[index].url;
    playTrack();
}

void MusicPlayer::playTrack() {
    labelTrack->setText("(NA)");

    if (!trackUrl.isEmpty()) {
        if (trackUrl.startsWith("http")) {
            labelTrack->setText(trackUrl);

            for (const Track &track : tracks) {
                if (track.url == trackUrl) {
                    labelTrack->setText(track.title);
                    break;
                }
            }

        } else {
            QFileInfo info(trackUrl);
            labelTrack->setText(info.fileName());
        }
        trackLength = 0;
        const char *url = trackUrl.toUtf8().data();
        mpv_command(mpv, (const char*[]){"loadfile", url, NULL});
        isPlaying = true;
        isPaused = false;
        buttonPlay->setText("Pause");
    }
}

void MusicPlayer::togglePlay() {
    if (isPlaying) {
        isPaused = !isPaused;
        const char *val = isPaused ? "yes" : "no";
        mpv_set_property_string(mpv, "pause", val);
        buttonPlay->setText(isPaused ? "Play" : "Pause");
    } else if (!trackUrl.isEmpty()) {
        playTrack();
    }
}

void MusicPlayer::setVolume(int value) {
    double volume = static_cast<double>(value);
    mpv_set_property(mpv, "volume", MPV_FORMAT_DOUBLE, &volume);
}

void MusicPlayer::seekStart() {
    isUserDragging = true;
}

void MusicPlayer::seekEnd() {
    int value = sliderSeek->value();
    double v = static_cast<double>(value);
    mpv_set_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &v);
    isUserDragging = false;
}

void MusicPlayer::updateSeekSlider() {
    if (!isPaused && !isUserDragging) {
        double duration = 0;
        if (mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &duration) >= 0 && duration > 0.1) {
            if (static_cast<int>(duration) != sliderSeek->maximum()) {
                sliderSeek->setRange(0, static_cast<int>(duration));
            }
            double currentTime = 0;
            if (mpv_get_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &currentTime) >= 0) {
                sliderSeek->setValue(static_cast<int>(currentTime));
            }
        }
    }
}

void MusicPlayer::searchYT() {
    QString term = entryFilter->text();
    if (term.isEmpty()) return;

    QProcess ytdlp;
    QStringList args = {QString("ytsearch20:%1").arg(term), "--flat-playlist", "--dump-single-json", "--print-json", "--skip-download", "--no-playlist"};
    ytdlp.start("yt-dlp", args);
    ytdlp.waitForFinished();

    QByteArray output = ytdlp.readAllStandardOutput();
    QList<QByteArray> lines = output.split('\n');

    tracks.clear();
    for (const QByteArray &line : lines) {
        if (line.trimmed().isEmpty()) continue;
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (!doc.isObject()) continue;

        QJsonObject obj = doc.object();
        QString title = obj.value("title").toString();
        QString url = obj.value("webpage_url").toString();
        if (!title.isEmpty() && !url.isEmpty()) {
            tracks.append({title, url});
        }
    }

    updateTrackList();
}
