#include "define.h"
#include "background.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QListView>
#include <QDir>
#include <QIcon>
#include <QPainter>
#include <QDebug>

Background::Background(QGraphicsScene *scene, QObject *parent)
    : QGraphicsObject()
    , m_scene(scene)
    , m_average_color(100, 100, 100)
{
    setParent(parent);  // ← Critical line
    m_scene->addItem(this);
    setZValue(-1000);
}

QRectF Background::boundingRect() const
{
    if (m_scene && !m_scene->views().isEmpty()) {
        QGraphicsView *view = m_scene->views().first();
        return QRectF(0, 0, view->viewport()->width(), view->viewport()->height());
    }
    // Fallback for safety (never hit in practice)
    return QRectF(0, 0, 800, 600);
}

void Background::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_pixmap.isNull() || !m_scene || m_scene->views().isEmpty()) {
        return;
    }

    QGraphicsView *view = m_scene->views().first();
    QRectF viewRect(0, 0, view->viewport()->width(), view->viewport()->height());

    QPixmap scaled = m_pixmap.scaled(viewRect.size().toSize(),
                                     Qt::KeepAspectRatioByExpanding,
                                     Qt::SmoothTransformation);

    qreal x = (viewRect.width() - scaled.width()) / 2.0;
    qreal y = (viewRect.height() - scaled.height()) / 2.0;

    painter->drawPixmap(QPointF(x, y), scaled);
}

void Background::onBackgroundSelected(const QModelIndex &index)
{
    QString path = index.data(Qt::UserRole).toString();
    if (!path.isEmpty()) {
        setBackgroundPixmap(path);
    }
}

void Background::setCredits(const QString &filename)
{
  QString prefix, suffix;

  int index = 0;

  for (int b = 1; b <= BACKGROUND_LAST_INDEX; b++) {
    if (filename.contains(BACKGROUND_FILES[b])) {
      suffix = FILES_CREDIT[b];

      index = b;
      break;
    }
  }

  creditColor = CREDITS_COLOR[index]; // found or not found (0) is still good.

  if (!index) {
    credit = "";
    return;
  }

  if (suffix.isEmpty())
    prefix = tr("Background created using gimp 2.10.18");
  else
    prefix = tr("Background image by: ");

  credit = prefix + suffix;
}

void Background::setBackgroundPixmap(const QString path, bool update_credits)
{
    QPixmap newPixmap(path);
    if (newPixmap.isNull()) return;

    fullpath = path;
    m_pixmap = newPixmap;
    update();  // Repaint the graphics item

    if (update_credits) {
      setCredits(path);
    }

    calcAverageColor();
    emit backgroundChanged(path);  // Notify MainWindow
}

// This functions is used to load legacy index INT format in configuration file.
void Background::setBackground(int index)
{
  QString filename;

  if (index == BACKGROUND_NONE)
    return;

  filename = BACKGROUND_FILES[index];
  creditColor = CREDITS_COLOR[index];

  QFile aFile;

  fullpath = QString(INSTALL_PATH) + QString("/backgrounds/") + filename;
  aFile.setFileName(fullpath);

  if (!aFile.exists()) {
    fullpath = QDir::homePath() + QString("/DEV/Hearts2/backgrounds/" + filename);
    aFile.setFileName(fullpath);

    if (!aFile.exists())
      return;
  }

  if ((index == BACKGROUND_LEAVES) || (index == BACKGROUND_MARBLE))
    credit = tr("Background created using gimp 2.10.18");
  else {
    credit = tr("Background image by: ");
    credit += FILES_CREDIT[index];
  }

  setBackgroundPixmap(fullpath, false);
}

void Background::calcAverageColor()
{
  QImage image = m_pixmap.toImage();
  if (image.isNull()) return;

    // Resize for speed (150x150 is plenty accurate for backgrounds)
    image = image.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    ulong r = 0, g = 0, b = 0;
    int pixelCount = image.width() * image.height();

    for (int y = 0; y < image.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            r += qRed(line[x]);
            g += qGreen(line[x]);
            b += qBlue(line[x]);
        }
    }

   m_average_color.setRgb( r / pixelCount, g / pixelCount, b / pixelCount );
}

QColor Background::getAverageColor()
{
   return m_average_color;
}
