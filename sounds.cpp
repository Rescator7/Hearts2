#include "sounds.h"
#include "resourcepaths.h"
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QDateTime>
#include <QTimer>
#include <QDebug>

Sounds::Sounds(QObject *parent) : QObject(parent) {
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

    // 1 player per sound to allow overlapping sounds.
    for (int i = 0; i < LAST_SOUND + 1; ++i) {
        QString path = getResourceFile(QString("sounds/") + sounds_files[i]);
        if (path.isEmpty()) continue;

        QMediaPlayer *player = new QMediaPlayer(this);
        QAudioOutput *output = new QAudioOutput(this);
        output->setVolume(0.5f);
        player->setAudioOutput(output);
        player->setSource(QUrl::fromLocalFile(path));

        soundPlayers.append(player);
    }
}

Sounds::~Sounds() {
    // Parent (this) will delete all children automatically
}

void Sounds::play(int soundId)
{
    if (!enabled || soundId < 0 || soundId >= LAST_SOUND) return;

    QMediaPlayer *player = soundPlayers[soundId];
    if (!player) return;

  // Étape 1 : Sauvegarde la source actuelle AVANT de la vider
    QUrl savedSource = player->source();

    // Étape 2 : Arrêt + reset complet
    player->stop();
    player->setPosition(0);

    // Étape 3 : Vide puis recharge la source (force la réinitialisation interne)
    player->setSource(QUrl());           // vide
    player->setSource(savedSource);      // remet l'original

// Petit délai pour laisser le backend "respirer" (souvent 10–50 ms suffit)
    QTimer::singleShot(50, player, [player]() {
        player->play();
    });
}

void Sounds::stopAllSounds()
{
    for (QMediaPlayer *p : soundPlayers) {
        if (p) p->stop();
    }
}

void Sounds::setEnabled(bool activate)
{
  enabled = activate;

  if (!enabled) {
    qDebug() << "Sounds: Sounds stopped: " << soundPlayers.size();
    stopAllSounds();
  }
}
