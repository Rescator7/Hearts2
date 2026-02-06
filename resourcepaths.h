#ifndef RESOURCEPATHS_H
#define RESOURCEPATHS_H

#include <QString>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>

inline QString getResourcesBasePath()
{
    // 1. Chemins système standards (priorité absolue)
    QStringList systemPaths = {
        "/usr/share/hearts",
        "/usr/local/share/hearts",
        "/usr/share/games/hearts",
        QDir::homePath() + "/.local/share/hearts"
    };

    for (const QString& path : systemPaths) {
        if (QDir(path).exists()) {
            qDebug() << "Ressources système trouvées :" << path;
            return path;
        }
    }

    // 2. Dossier local utilisateur (copie manuelle)
    QString localPath = QDir::homePath() + "/hearts";
    if (QDir(localPath).exists()) {
        qDebug() << "Ressources locales trouvées :" << localPath;
        return localPath;
    }

    // 3. Détection intelligente depuis l'exécutable (dev ou portable)
    QString exeDir = QCoreApplication::applicationDirPath();
    QDir dir(exeDir);

    // Remontée plus généreuse (max 5 niveaux pour couvrir /usr/bin → /usr → /usr/local → racine)
    int maxUp = 5;
    while (maxUp-- > 0 && dir.cdUp()) {
        // Marqueurs fiables de la racine projet
        if (QFile::exists(dir.filePath("CMakeLists.txt")) ||
            QDir(dir.filePath("backgrounds")).exists() ||
            QDir(dir.filePath("sounds")).exists()) {
            QString rootShare = dir.filePath("share/hearts");
            if (QDir(rootShare).exists()) {
                qDebug() << "Ressources via share/hearts :" << rootShare;
                return rootShare;
            }
            // Fallback racine projet
            qDebug() << "Racine projet détectée :" << dir.absolutePath();
            return dir.absolutePath();
        }
    }

    // 4. Dernier fallback : dossier de l'exécutable (dev/portable)
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
