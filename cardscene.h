#pragma once

#ifndef CARDSCENE_H
#define CARDSCENE_H

// CardScene.h
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>

class CardScene : public QGraphicsScene
{
    Q_OBJECT

private:
    QList<QGraphicsItem*> m_protectedItems;

public:
    explicit CardScene(QObject *parent = nullptr);

    // Register these protected items
    void setBackgroundItem(QGraphicsItem *item);
    void setTurnIndicatorItem(QGraphicsItem *item);
    void setArrowLabel(QGraphicsProxyWidget *proxy);
    void clear();
    void protectItem(QGraphicsItem *item);

signals:
    void cardClicked(QGraphicsItem *card);
    void arrowClicked();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
};

#endif // CARDSCENE_H
