#pragma once

#ifndef SOUNDS_H
#define SOUNDS_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>

enum SOUNDS {
     SOUND_BREAKING_HEARTS  = 0,
     SOUND_DEALING_CARD     = 1,
     SOUND_ERROR            = 2,
     SOUND_GAME_OVER        = 3,
     SOUND_SHOOT_MOON       = 4,
     SOUND_YOUR_TURN        = 5,
     SOUND_QUEEN_SPADE      = 6,
     SOUND_CONTACT          = 7,
     SOUND_PASSING_CARDS    = 8,
     SOUND_SHUFFLING_CARDS  = 9,
     SOUND_PERFECT_100      = 10,
     SOUND_CONNECTED        = 11,
     SOUND_DISCONNECTED     = 12,
     SOUND_ANNOUCEMENT      = 13,
     SOUND_UNDO             = 14,
     SOUND_TRAM             = 15
};

constexpr int  LAST_SOUND          = 15;

class Sounds : public QObject
{
    Q_OBJECT

private:
    bool enabled = true;
    QList<QMediaPlayer*> activePlayers;

signals:
    void soundFinished(int soundId);
    void soundFailed(int soundId, const QString &error);

public:
    explicit Sounds(QObject *parent = nullptr);
    ~Sounds();

    void play(int soundID);
    void stop(int soundID);
    void setEnabled(bool activate);
    void stopAllSounds();
};

#endif // SOUNDS_H
