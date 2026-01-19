#include "turnarrow.h"

// In constructor
TurnArrow::TurnArrow(qreal size)
    : m_size(size)
{
    setFlag(QGraphicsItem::ItemIgnoresParentOpacity, true);  // optional
    setAcceptedMouseButtons(Qt::NoButton);  // ← Critical: ignores all clicks
    setFlag(QGraphicsItem::ItemIsSelectable, false); // not selectable
    setFlag(QGraphicsItem::ItemIsMovable, false);
}
