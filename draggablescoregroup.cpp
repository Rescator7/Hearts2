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

/*
QVariant DraggableScoreGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange)
        m_mainWindow->setDraggingScore(true);

        QPointF newPos = value.toPointF();

        qDebug() << "ItemPositionChange détecté - old :" << pos() << "new :" << newPos;

        // Seuil de 10 px pour éviter les micro-mouvements
        if (QLineF(pos(), newPos).length() > 10.0) {
            if (m_mainWindow) {
                m_mainWindow->setSavedScorePos(newPos);
                m_mainWindow->setScorePositionSaved(true);
                qDebug() << "VRAI DRAG > 10px ! Position sauvegardée :" << newPos;
            }
        } else {
            qDebug() << "Déplacement trop petit (< 10px) - pas de sauvegarde";
    }

    if (change == ItemPositionHasChanged) {
      m_mainWindow->setDraggingScore(false);
    }
    return QGraphicsItemGroup::itemChange(change, value);
}
*/
/*
QVariant DraggableScoreGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange) {  // Note : ItemPositionChange (pas HasChanged)
        QPointF newPos = value.toPointF();

        // Log pour voir les changements
        qDebug() << "ItemPositionChange détecté - old :" << pos() << "new :" << newPos;

        if (m_mainWindow) {
            m_mainWindow->setSaveScorePos(newPos);
            m_mainWindow->setScorePositionSave(true);
            qDebug() << "Sauvegarde forcée de la position :" << newPos;
        }
    }

    return QGraphicsItemGroup::itemChange(change, value);
}
*/
/*
QVariant DraggableScoreGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        QPointF newPos = value.toPointF();

        // Log pour voir TOUS les changements de position
        qDebug() << "ItemPositionHasChanged détecté - oldPos :" << pos() << "newPos :" << newPos;

        // Abaissé à 5 px pour être plus sensible
        if (QLineF(pos(), newPos).length() > 5.0) {
            if (m_mainWindow) {
                 m_mainWindow->setSaveScorePos(newPos);
                 m_mainWindow->setScorePositionSave(true);
//                m_mainWindow->savedScorePos = newPos;
//                m_mainWindow->scorePositionSaved = true;
                qDebug() << "VRAI DRAG détecté ! Position absolue sauvegardée :" << newPos;
            }
        } else {
            qDebug() << "Changement trop petit (< 5 px) - ignoré pour sauvegarde";
        }
    }

    return QGraphicsItemGroup::itemChange(change, value);
}
*/
/*
QVariant DraggableScoreGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        QPointF newPos = value.toPointF();

        // Si déplacement significatif (> 10 px)
        if (QLineF(pos(), newPos).length() > 10.0) {
            if (m_mainWindow) {
                m_mainWindow->setSaveScorePos(newPos);
                m_mainWindow->setScorePositionSave(true);
       //         m_mainWindow->savedScorePos = newPos;
        //        m_mainWindow->scorePositionSaved = true;
                qDebug() << "Position absolue sauvegardée au drop :" << newPos;
            }
        }
    }

    return QGraphicsItemGroup::itemChange(change, value);
}
*/

/*
QVariant DraggableScoreGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        if (m_mainWindow && scene() && !scene()->views().isEmpty()) {
            QGraphicsView *view = scene()->views().first();
            QSize viewSize = view->size();

            if (viewSize.width() > 0 && viewSize.height() > 0) {
                QPointF center = pos() + boundingRect().center();

                center.setX(qBound(0.0, center.x(), static_cast<qreal>(viewSize.width())));
                center.setY(qBound(0.0, center.y(), static_cast<qreal>(viewSize.height())));

                qreal toLeft = center.x();
                qreal toRight = viewSize.width() - center.x();
                qreal toTop = center.y();
                qreal toBottom = viewSize.height() - center.y();

                ScoreAnchorSide sideX = (toLeft < toRight) ? LeftSide : RightSide;
                ScoreAnchorSide sideY = (toTop < toBottom) ? TopSide : BottomSide;

                qreal distX = (sideX == LeftSide) ? toLeft : toRight;
                qreal distY = (sideY == TopSide) ? toTop : toBottom;

                // Mise à jour explicite
                m_mainWindow->setScoreAnchorSideX(sideX);
                m_mainWindow->setScoreAnchorSideY(sideY);
                m_mainWindow->setScoreAnchorDistX(distX);
                m_mainWindow->setScoreAnchorDistY(distY);

                //m_mainWindow->lastViewSize = QSize(0, 0);  // force un recalcul au prochain vrai resize
                m_mainWindow->setLastViewSize(QSize(0, 0));
                qDebug() << "DROP - Ancrage côté X/Y :" << sideX << sideY << "DistX/Y :" << distX << distY << "Center :" << center;
            }
        }
    }

    return QGraphicsItemGroup::itemChange(change, value);
}
*/

/*
QVariant DraggableScoreGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged) {
        if (m_mainWindow && scene() && !scene()->views().isEmpty()) {
            QGraphicsView *view = scene()->views().first();
            QSize viewSize = view->size();

            if (viewSize.width() > 0 && viewSize.height() > 0) {
                QPointF center = pos() + boundingRect().center();

                // Sécurité sur le centre
                center.setX(qBound(0.0, center.x(), static_cast<qreal>(viewSize.width())));
                center.setY(qBound(0.0, center.y(), static_cast<qreal>(viewSize.height())));

                // Distance au gauche et au droit
                qreal toLeft = center.x();
                qreal toRight = viewSize.width() - center.x();

                // Distance au haut et au bas
                qreal toTop = center.y();
                qreal toBottom = viewSize.height() - center.y();

                // Choisit le côté le plus proche pour chaque axe
                ScoreAnchorSide sideX = (toLeft < toRight) ? LeftSide : RightSide;
                ScoreAnchorSide sideY = (toTop < toBottom) ? TopSide : BottomSide;

                qreal distX = (sideX == LeftSide) ? toLeft : toRight;
                qreal distY = (sideY == TopSide) ? toTop : toBottom;

                // Mise à jour
                m_mainWindow->setScoreAnchorSideX(sideX);
                m_mainWindow->setScoreAnchorSideY(sideY);
                m_mainWindow->setScoreAnchorDistX(distX);
                m_mainWindow->setScoreAnchorDistY(distY);

                qDebug() << "Ancrage côté X/Y :" << sideX << sideY << "DistX/Y :" << distX << distY
                         << "Center :" << center;
            }
        }
    }

    return QGraphicsItemGroup::itemChange(change, value);
}
*/
