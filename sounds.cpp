#include "sounds.h"
#include "define.h"
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QDateTime>
#include <QDebug>

Sounds::Sounds(QObject *parent) : QObject(parent) {
}

Sounds::~Sounds() {
    // Parent (this) will delete all children automatically
}

void Sounds::play(int soundId)
{
    const char sounds_files[LAST_SOUND + 1][127] = {
        "34201__themfish__glass-house1.wav",
        "240777__f4ngy__dealing-card.wav",
        "253886__themusicalnomad__negative-beeps.wav",
        "133283__fins__game-over.wav",
        "171671__fins__success-1.wav",
        "Soundwhich - Correct (LittleRainySeasons) [public].wav",
        "344013__reitanna__high-pitched-ah2.wav",
        "96127__bmaczero__contact1_padded.wav",
        "434472__dersuperanton__taking-card.wav",
        "423767__someonecool14__card-shuffling.wav",
        "400163__vitovsky1__fanfare-short.wav",
        "322897__rhodesmas__connected-01.wav",
        "322895__rhodesmas__disconnected-01.wav",
        "493696__stib__bingbong.wav",
        "403013__inspectorj__ui-confirmation-alert-b5.wav",
        "404553__inspectorj__clap-single-7_modified.wav"
    };

    if (soundId < 0 || (soundId > LAST_SOUND) || !enabled) return;

    if (activePlayers.size() >= 5) {
        // Stop the oldest one
        QMediaPlayer *oldest = activePlayers.takeFirst();
        oldest->stop();
        oldest->deleteLater();
    }

    QString basePath = "/usr/local/Hearts2/sounds/";
    if (!QFile::exists(basePath + sounds_files[0])) {
        basePath = QDir::homePath() + FOLDER + "sounds/";
    }

    QString filename = basePath + QString(sounds_files[soundId]);

    qDebug() << "Filename: " << filename;

    // Create completely new player/output every time
    QAudioOutput *output = new QAudioOutput(this);
    output->setVolume(0.5f);

    QMediaPlayer *player = new QMediaPlayer(this);

    player->setAudioOutput(output);
    player->setSource(QUrl::fromLocalFile(filename));

    // Auto-cleanup when finished
    connect(player, &QMediaPlayer::mediaStatusChanged, player, [player, output, this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia ||
            status == QMediaPlayer::InvalidMedia ||
            status == QMediaPlayer::NoMedia) {
            player->stop();
            activePlayers.removeAll(player);
            player->deleteLater();
            output->deleteLater();
        }
    });

    player->setPosition(0);
    activePlayers.append(player);
    player->play();
}

void Sounds::stopAllSounds()
{
  for (QMediaPlayer *p : activePlayers) {
     p->stop();
   }
  activePlayers.clear();
}

void Sounds::setEnabled(bool activate)
{
  enabled = activate;

  if (!enabled) {
    qDebug() << "Sounds stopped: " << activePlayers.size();
    stopAllSounds();
  }
}
