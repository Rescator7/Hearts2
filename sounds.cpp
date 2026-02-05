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


  if (!al_install_audio()) {
    qWarning() << "Échec initialisation Allegro audio";
    return;
  }

  al_reserve_samples(LAST_SOUND + 1);

  if (!al_init_acodec_addon()) {
    qWarning() << "Échec initialisation Allegro acodec";
    return;
  }

  if (!al_restore_default_mixer()) {
    qWarning() << "Échec restauration mixer Allegro";
    return;
  }

  audioAvailable = true;

  for (int i = 0; i <= LAST_SOUND; ++i) {
     QString path = getResourceFile("sounds/" + QString(sounds_files[i]));
     if (path.isEmpty()) {
       qWarning() << "Missing sound :" << sounds_files[i];
       continue;
     }

    ALLEGRO_SAMPLE *sample = al_load_sample(path.toStdString().c_str());
    if (sample) {
      samples[i] = sample;
      qDebug() << "Sound loaded :" << sounds_files[i];
    } else {
        qWarning() << "Sound loading failed :" << path;
      }
  }

  if (samples.isEmpty()) {
    qWarning() << "Aucun son chargé → audio désactivé";
    audioAvailable = false;
  }
}

Sounds::~Sounds() {
  for (auto sample : samples) {
    if (sample) al_destroy_sample(sample);
  }
  al_uninstall_audio();
}

void Sounds::play(int soundId)
{
  if (!enabled || !audioAvailable || !samples.contains(soundId)) {
    return;
  }

  al_play_sample(samples[soundId], 0.5f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, nullptr);
}

void Sounds::stopAllSounds()
{
  if (audioAvailable)
    al_stop_samples();
}

void Sounds::setEnabled(bool activate)
{
  enabled = activate;

  if (!enabled) {
    stopAllSounds();
  }
}
