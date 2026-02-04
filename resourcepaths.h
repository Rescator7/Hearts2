#ifndef RESOURCEPATHS_H
#define RESOURCEPATHS_H

#include <QString>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

inline QString getResourcesBasePath()
{
    // 1. Installation système (priorité absolue)
    QString systemPath = "/usr/local/share/hearts";
    if (QDir(systemPath).exists()) {
        qDebug() << "Ressources système trouvées :" << systemPath;
        return systemPath;
    }

    // 2. Dossier local utilisateur (copie manuelle ou portable)
    QString localPath = QDir::homePath() + "/hearts";
    if (QDir(localPath).exists()) {
        qDebug() << "Ressources locales trouvées :" << localPath;
        return localPath;
    }

    // 3. Détection intelligente depuis le dossier de l'exécutable
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);

    // On remonte jusqu'à la racine du projet (gère build, build/Desktop-*, etc.)
    int maxUp = 3;  // sécurité
    while (maxUp-- > 0 && dir.cdUp()) {
        // On cherche un marqueur fiable de la racine du projet
        // (ex: CMakeLists.txt, ou un dossier connu comme backgrounds)
        if (QFile::exists(dir.filePath("CMakeLists.txt")) ||
            QDir(dir.filePath("backgrounds")).exists()) {
            QString rootShare = dir.filePath("share/hearts");
            if (QDir(rootShare).exists()) {
                qDebug() << "Ressources trouvées à la racine projet :" << rootShare;
                return rootShare;
            }
            // Sinon on retourne la racine projet directement
            qDebug() << "Racine projet détectée (fallback share absent) :" << dir.absolutePath();
            return dir.absolutePath();
        }
    }

    // Dernier fallback absolu : dossier de l'exécutable
    qDebug() << "Dernier fallback : dossier de l'exécutable :" << exeDir;
    return exeDir;
}

// Retourne le chemin complet d'une ressource (ou vide si non trouvée)
inline QString getResourceFile(const QString& subPath)
{
    QString base = getResourcesBasePath();
    QString fullPath = QDir(base).filePath(subPath);

    if (QFile::exists(fullPath)) {
        qDebug() << "Ressource trouvée :" << fullPath;
        return fullPath;
    }

    qWarning() << "Ressource non trouvée :" << fullPath;
    return QString();
}

#endif // RESOURCEPATHS_H
