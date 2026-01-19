#include "define.h"
#include "cardscene.h"
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>

CardScene::CardScene(QObject *parent) : QGraphicsScene(parent) {}

void CardScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
qDebug() << "****";
    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    // Get all items at click position, sorted by Z (highest first by default)
    QList<QGraphicsItem*> items = this->items(event->scenePos());

    QGraphicsItem *clickedCard = nullptr;

    for (QGraphicsItem *item : items) {
        // 1. Skip items that ignore mouse events
        if (item->acceptedMouseButtons() == Qt::NoButton) {
            continue;
        }

        // 2. Skip non-selectable items (extra safety)
        if (!item->flags().testFlag(QGraphicsItem::ItemIsSelectable)) {
            continue;
        }

        // Block click if card is disabled
        if (item->data(10).toBool()) {
          return;  // Ignore click completely
        }

        // 3. Optional: skip your turn arrow explicitly by type (very safe)
        // if (dynamic_cast<TurnArrow*>(item)) continue;

        // 4. Skip background or other non-card items (by negative Z or data key)
        if (item->zValue() < 0) {
            continue;
        }

        // for now
        if (item->zValue() == ZLayer::Z_ARROW) {
          emit arrowClicked();
          continue;
        }

        // 5. Only accept actual cards — you can add a data key or type check
        // Example: if your cards have item->setData(0, "card");
        // if (item->data(0).toString() != "card") continue;

        // If we reach here: it's a clickable, selectable card
        clickedCard = item;
        break;  // take the topmost valid card
    }

    if (clickedCard) {
        emit cardClicked(clickedCard);
    }

    // Always call base class so selection rectangle, etc. still work if needed
    QGraphicsScene::mousePressEvent(event);
}

void CardScene::clear()
{
qDebug() << "CLEARING SCENE";
    QList<QGraphicsItem*> allItems = items();

    for (QGraphicsItem *item : allItems) {
       if (m_protectedItems.contains(item)) {
         continue;  // Skip protected (background, arrow, credits, etc.)
       }
       item->hide();
       item->setPos(-1000, -1000);  // Off-screen, out of the way
    }

    // QGraphicsScene::clear() does more than just remove items
    // So call base only if you want full reset (but it would delete protected items)
    // → Don't call QGraphicsScene::clear() here

     // Force un vrai nettoyage des caches de rendu
    invalidate(sceneRect(), QGraphicsScene::AllLayers);
    update();

    // Petit coup de pouce à l'event loop pour libérer les buffers OpenGL
    QApplication::processEvents(QEventLoop::AllEvents, 20);
}

void CardScene::protectItem(QGraphicsItem *item)
{
   if (item && !m_protectedItems.contains(item)) {
     m_protectedItems.append(item);
   }
}
