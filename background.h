#pragma once

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <QPixmap>
#include <QGraphicsObject>
#include <QGraphicsScene>
#include <QLabel>
#include <QColor>
#include <QString>

enum BACKGROUND {
     BACKGROUND_NONE                = 0,
     BACKGROUND_GREEN               = 1,
     BACKGROUND_UNIVERSE            = 2,
     BACKGROUND_OCEAN               = 3,
     BACKGROUND_MT_FUJI             = 4,
     BACKGROUND_EVEREST             = 5,
     BACKGROUND_DESERT              = 6,
     BACKGROUND_WOODEN_1            = 7,
     BACKGROUND_WOODEN_2            = 8,
     BACKGROUND_WOODEN_3            = 9,
     BACKGROUND_WOODEN_4            = 10,
     BACKGROUND_LEAVES              = 11,
     BACKGROUND_MARBLE              = 12
};

constexpr int BACKGROUND_LAST_INDEX          = 12;

constexpr int MAX_FILENAME_LEN = 256;  // Safe maximum

constexpr char BACKGROUND_FILES[13][MAX_FILENAME_LEN] = {
               "",
               "380441109-2ed06248-c87c-4a56-a718-0584d0cdc85d.png",
               "eso1723a.jpg",
               "__Palm___Beach._Port_Douglas._-_panoramio.jpg",
               "Mount._Fuji_early_in_the_morning_早朝の富士山_-_panoramio.jpg",
               "8,848m_Everest_8,516m_Lhotse_Himalaya_Mountain_Flights_Nepal_-_panoramio_(2).jpg",
               "Wadi_rum_desert.jpg",
               "WoodenPlanks.svg",
               "WoodTexture.svg",
               "WoodenFloor2.svg",
               "OverlappingPlanks5.svg",
               "leaves.jpg",
               "marble.jpg"
};

constexpr char FILES_CREDIT[13][20] = {
               "",
               "Attila Tóth",
               "ESO/G. Beccari",
               "grumpylumixuser",
               "Jack Soma",
               "Hiroki Ogawa",
               "Peter Chisholm",
               "Firkin",
               "Firkin",
               "Firkin",
               "Firkin",
               "",
               ""
};

constexpr char CREDITS_COLOR[13][10] = {
               "white",
               "lightgray",
               "lightgray",
               "lightgray",
               "lightgray",
               "lightgray",
               "black",
               "black",
               "lightgray",
               "black",
               "black",
               "lightgray",
               "black"
};

class QListView;

class Background : public QGraphicsObject
{
    Q_OBJECT

private:
    QPixmap m_pixmap;
    QGraphicsScene *m_scene;
    QColor m_average_color;
    QString credit;
    QString creditColor = "white";
    QString fullpath;
    void calcAverageColor();

private slots:
    void onBackgroundSelected(const QModelIndex &index);

signals:
    void backgroundChanged(const QString &path);  // Emit when background changes

public:
    explicit Background(QGraphicsScene *scene, QObject *parent = nullptr);

    // Required overrides
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    void setupBackgroundList(QListView *listView, QLabel *previewLabel);
    void setBackgroundPixmap(const QString path, bool update_credits = true);
    void setBackground(int index);
    void setCredits(const QString &path);
    QString &Credits() { return credit; };
    QString &FullPath() { return fullpath; };
    QString &CreditTextColor() { return creditColor; };
    QColor getAverageColor();

};

#endif // BACKGROUND_H
