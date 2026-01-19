#pragma once

#ifndef TURNARROW
#define TURNARROW

#include <QGraphicsObject>
#include <QPainter>

class TurnArrow : public QGraphicsObject
{
public:
    TurnArrow(qreal size = 40.0);  // 40-50 px is nice and discreet

    QRectF boundingRect() const override {
        qreal penWidth = 4;
        return QRectF(-m_size/2 - penWidth, -m_size/2 - penWidth,
                      m_size + penWidth*2, m_size + penWidth*2);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override {
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Light green fill
        painter->setBrush(QColor(180, 255, 180));  // softer light green

        // Border: gold
        QPen pen(QColor(255, 200, 0));
        pen.setWidth(2);
        pen.setJoinStyle(Qt::MiterJoin);
        painter->setPen(pen);

        // Downward-pointing equilateral triangle
        QPolygonF triangle;
        triangle << QPointF(0, m_size * 0.5)           // bottom tip
                 << QPointF(-m_size * 0.5, -m_size * 0.5)  // top-left
                 << QPointF( m_size * 0.5, -m_size * 0.5); // top-right

        painter->drawPolygon(triangle);
    }

private:
    qreal m_size;
};

#endif
