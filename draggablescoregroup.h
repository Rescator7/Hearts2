#ifndef DRAGGABLESCOREGROUP_H
#define DRAGGABLESCOREGROUP_H

#include <QGraphicsItemGroup>

class MainWindow;  // Forward declaration (pas besoin de tout inclure)

class DraggableScoreGroup : public QGraphicsItemGroup
{
public:
    explicit DraggableScoreGroup(MainWindow *mainWindow, QGraphicsItem *parent = nullptr);

//protected:
//    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    MainWindow *m_mainWindow;  // Pour accéder aux setters publics
};

#endif // DRAGGABLESCOREGROUP_H
