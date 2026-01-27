#include <QSize>
#include <QGraphicsView>
#include <QTimer>

#include "draggablescoregroup.h"
#include "mainwindow.h"

DraggableScoreGroup::DraggableScoreGroup(MainWindow *mainWindow, QGraphicsItem *parent)
    : QGraphicsItemGroup(parent),
      m_mainWindow(mainWindow)
{
    setFlag(ItemIsMovable, true);
    setFlag(ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
//    m_mainWindow->setDraggingScore(true);
}

void DraggableScoreGroup::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    QGraphicsItemGroup::hoverEnterEvent(event);
}

void DraggableScoreGroup::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    setCursor(Qt::ArrowCursor);
    QGraphicsItemGroup::hoverLeaveEvent(event);
}

QVariant DraggableScoreGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
/*
    if (change == ItemPositionChange) {
        QPointF newPos = value.toPointF();

    //    qDebug() << "ItemPositionChange détecté - old :" << pos() << "new :" << newPos;

        m_mainWindow->setDraggingScore(true);  // ← Flag ON pendant le drag

        if (QLineF(pos(), newPos).length() > 10.0) {
            if (m_mainWindow) {
                m_mainWindow->setSavedScorePos(newPos);
                m_mainWindow->setScorePositionSaved(true);
                qDebug() << "VRAI DRAG > 10px ! Position sauvegardée :" << newPos;
            }
        }
    }

    if (change == ItemPositionHasChanged) {
        m_mainWindow->setDraggingScore(false);  // ← Flag OFF au drop
   //     qDebug() << "Drag terminé - flag isDragging = false";
    }
*/
    return QGraphicsItemGroup::itemChange(change, value);
}



