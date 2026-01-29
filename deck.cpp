#include "deck.h"
#include "define.h"

#include <QDebug>
#include <QDir>
#include <QRectF>
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>
#include <QGraphicsSvgItem>
#include <QCoreApplication>

Deck::Deck(QObject *parent) : QObject(parent)
{
}

Deck::~Deck() {
  if (renderer) {
    delete renderer;
    renderer = nullptr;
  }
}

void Deck::delete_current_deck() {
  current_deck_style = UNSET_DECK;

  // Safe cleanup: qDeleteAll deletes each QGraphicsItem
  // QGraphicsItem destructor automatically calls scene()->removeItem(this)
  // → No dangling pointers, no manual removeItem needed
  // → Scene count drops correctly, permanent items stay
  qDeleteAll(deck);
  deck.clear();
  pixImages.clear();
}

bool Deck::check_file(QString &filename) {
    QFile aFile;

    aFile.setFileName(filename);
    if (!aFile.exists()) {
      qDebug() << filename << ": not found!";
      return false;
    }

    return true;
}

void Deck::createPixmap(int cardId, bool suitFirst, int format)
{
  if (!renderer || !renderer->isValid()) {
    return;
  }

  QSize targetSize = QSize(90, 130);
  QPixmap pixmap(targetSize);
  pixmap.fill(Qt::transparent);           // important for transparency in cards

  QPainter painter(&pixmap);

  // Enable high-quality rendering
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
  // Optional but often nice too:
  painter.setRenderHint(QPainter::TextAntialiasing, true);

  // Render the whole document (if no specific element)
  if (format == SVG_1_FILE_FORMAT) {
    QString elementId = getElementIdForCard(cardId, suitFirst);
    renderer->render(&painter, elementId, QRectF(0, 0, targetSize.width(), targetSize.height()));
  } else {
      renderer->render(&painter, QRectF(0, 0, targetSize.width(), targetSize.height()));
  }
  pixImages.append(pixmap);
}

QString Deck::getElementIdForCard(int cardId, bool suitFirst) const {
   if (cardId < 0 || cardId >= DECK_SIZE) return QString();

   static const QStringList textSuit = QStringList{"club", "spade", "diamond", "heart"};
   static const QStringList textRank = QStringList{"2", "3", "4", "5", "6", "7", "8", "9", "10", "jack", "queen", "king", "1"};

   int suit  = cardId / 13;
   int rank  = cardId % 13;

   if (suitFirst) {
     return textSuit[suit] + "_" + textRank[rank];
   } else {
       return textRank[rank] + "_" + textSuit[suit];
     }
 }

QGraphicsSvgItem *Deck::createSvgCardElement(int cardId, bool suitFirst)
{
    QString elementId;
    if (cardId == BACK_CARD) {
      elementId = "back";
    } else {
        elementId = getElementIdForCard(cardId, suitFirst);
    }

    if (!renderer->elementExists(elementId)) {
      qWarning() << "Element ID not found:" << elementId;
      return nullptr;
    }

    QGraphicsSvgItem *item = new QGraphicsSvgItem();
    item->setSharedRenderer(renderer);
    item->setElementId(elementId);

    QRectF native = item->boundingRect();
    if (native.width() > 0) {
        qreal scale = TARGET_CARD_WIDTH / native.width();
        item->setScale(scale);
    }

    item->setTransformOriginPoint(QPointF(native.width() / 2, 0));

    return item;
}


QGraphicsSvgItem* Deck::loadGraphicsItem(const QString &filePath)
{
    QGraphicsSvgItem *item = nullptr;

  //  const qreal TARGET_CARD_WIDTH = 140.0;  // Logical width you want for all cards
    if (filePath.endsWith(".svg", Qt::CaseInsensitive)) {
        // SVG: Native vector – no rasterization needed
        QGraphicsSvgItem *svgItem = new QGraphicsSvgItem(filePath);

        // Auto-scale to target width (preserves aspect ratio)
        QRectF native = svgItem->boundingRect();
        if (native.width() > 0) {
            qreal scale = TARGET_CARD_WIDTH / native.width();
            svgItem->setScale(scale);
        }

        // Align by top-center instead of full center

        svgItem->setTransformOriginPoint(QPointF(native.width() / 2, 0));  // Top-center anchor
      //  svgItem->setTransformOriginPoint(svgItem->boundingRect().center());
        item = svgItem;
    } else {

    // I dropped PNG support. All cards images are now QGraphicsSvgItems, switching back to
    // QGraphicsItems would break the animation: highlightAIPassedCards() code, because it is using QPropertyAnimation(cardItem, "pos");

/*
        // PNG etc.: Load at a reasonable size or full res
        QPixmap pixmap(filePath);
        if (pixmap.isNull()) return nullptr;

        // Optional: pre-scale large PNGs if needed
        // pixmap = pixmap.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QGraphicsPixmapItem *pixItem = new QGraphicsPixmapItem(pixmap);

           // Auto-scale PNG to same logical width
        if (pixmap.width() > 0) {
            qreal scale = TARGET_CARD_WIDTH / pixmap.width();
            pixItem->setScale(scale);
        }

        // Align by top-center instead of full center
            QRectF native = pixItem->boundingRect();
            pixItem->setTransformOriginPoint(QPointF(native.width() / 2, 0));  // Top-center anchor
        //    pixItem->setTransformOriginPoint(pixmap.rect().center());

        pixItem->setTransformationMode(Qt::SmoothTransformation);
        item = pixItem;
*/
    }

    return item;
}

QGraphicsSvgItem *Deck::get_card_item(int cardId, bool revealed) {
  int size = deck.size() / 2;

  if (cardId < 0 || cardId > size) {
     qCritical() << "FATAL: Invalid card ID requested:" << cardId
                 << "(deck size:" << deck.size() << ")";
     qCritical() << "This indicates a serious bug in game logic.";

    // Force quit — better than continuing with invalid state
    QCoreApplication::quit();
    return nullptr;  // Never reached, but keeps compiler happy
  }

  if (revealed) {
   return deck.at(cardId);
  }
  else {
   return deck.at(cardId + 52);
  }
}

const QPixmap &Deck::get_card_pixmap(int cardId) const {
  static QPixmap empty;

  if ((cardId < 0) || (cardId >= pixImages.size())) {
    return empty;
  }

  return pixImages.at(cardId);
}

void Deck::apply_settings(QGraphicsItem *item, int cardId) {
    item->setData(0, cardId);                              // save the cardId at Data(0)
    item->setData(1, false);                               // the card is not selected

    item->setFlag(QGraphicsItem::ItemIsFocusable, false);  // prevent glitch green trace during animation
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);  // Optional highlight
    item->setFlag(QGraphicsItem::ItemIsMovable, false);    // Prevent drag
    item->setAcceptedMouseButtons(Qt::LeftButton);

    item->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);  // ← important

// Désactive complètement l'effet de sélection visuel
    item->setSelected(false);
    item->setFlag(QGraphicsItem::ItemHasNoContents, false);

    item->hide();
}

void Deck::reset_selections()
{
  for (QGraphicsItem *item : deck)
     item->setData(1, false);
}

bool Deck::set_deck(int style) {
  int format;
  bool suitFirst = true;

  QString path, location, folder_name, fullpath;

  const char cards_name[13][10] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "jack", "queen", "king", "1"};
  const char image_format[3][4] = {"PNG", "SVG", "SVG"};
  const char card_suit_name [4][10] = {"/club_", "/spade_", "/diamond_", "/heart_"};

  if (style == current_deck_style)
    return true;

  if (current_deck_style != UNSET_DECK)
    delete_current_deck();

  current_deck_style = style;

  QGraphicsSvgItem *item;

  switch (style) {
    case KAISER_JUBILAUM:             folder_name = "kaiser-jubilaum";
                                      format = SVG_1_FILE_FORMAT;
                                      suitFirst = true;
                                      break;
    case MITTELALTER_DECK:            folder_name = "mittelalter";
                                      format = SVG_53_FILES_FORMAT;
                                      break;
    case ENGLISH_DECK:                folder_name = "English";
                                      format = SVG_53_FILES_FORMAT;
                                      break;
    case RUSSIAN_DECK:                folder_name = "Russian";
                                      format = SVG_53_FILES_FORMAT;
                                      break;
    case NICU_WHITE_DECK:             folder_name = "white";
                                      format = SVG_53_FILES_FORMAT;
                                      break;
    case NEOCLASSICAL_DECK:           folder_name = "neoclassical";
                                      format = SVG_53_FILES_FORMAT;
                                      break;
    case TIGULLIO_INTERNATIONAL_DECK: folder_name = "tigullio-international";
                                      format = SVG_53_FILES_FORMAT;
                                     // format = SVG_1_FILE_FORMAT;
                                      break;
    case TIGULLIO_MODERN_DECK:        return false; // unsupported

    default:
    case STANDARD_DECK:               folder_name = "Default";
                                      format = SVG_1_FILE_FORMAT;
                                      suitFirst = true;
                                      break;
  }

  if (renderer) {
    delete renderer;
  }
  renderer = new QSvgRenderer(this);

  // built-in deck
  if (style == STANDARD_DECK) {
    fullpath = QString(":/SVG-cards/Default");
  } else {
      path = "/usr/local/Hearts/";
      if (!check_file(path))
        path = QDir::homePath() + FOLDER;

        fullpath = path + QString(image_format[format]) + QString("-cards/") + folder_name;
      }

  if (format == SVG_1_FILE_FORMAT) {
    location = fullpath + QString("/" + folder_name + QString(".svg"));
     if (!check_file(location)) {
      delete_current_deck();
      return false;
    }

    renderer->load(location);
    if (!renderer->isValid()) {
      delete_current_deck();
      qCritical() << "Failed to load deck SVG! [" << location << "]";
      return false;
    } else {
 //     qDebug() << "Deck SVG loaded successfully";
      }
  }

 // renderer = new QSvgRenderer(location);
  int cpt = 0, suit = 0;

  for (int i = 0; i < DECK_SIZE; i++) {
    switch (format) {
       case PNG_FILE_FORMAT:
       case SVG_53_FILES_FORMAT: location = fullpath + QString(card_suit_name[suit]) + QString(cards_name[cpt]) + QString(".") + QString(image_format[format]).toLower();
                                 if (!check_file(location)) {
                                   delete_current_deck();
                                   return false;
                                 }
                                 item = loadGraphicsItem(location);
                                 renderer->load(location);   // This will be needed to create the pixmap
                                 break;
       case SVG_1_FILE_FORMAT: item = createSvgCardElement(i, suitFirst);
                               break;
    }

    if (!item) {
      delete_current_deck(); // delete incomplete deck (safe if empty)
      return false;
    }

    apply_settings(item, i);
    deck.append(item);
    createPixmap(i, suitFirst, format);

    if (++cpt == 13) {
      suit++;
      cpt = 0;
    }
  }

  // clone 52 back cards
  for (int clone = 0; clone < DECK_SIZE; clone++) {
    if (format == SVG_1_FILE_FORMAT) {
      item = createSvgCardElement(BACK_CARD, suitFirst);
    } else {
        location = fullpath + QString("/back.") + QString(image_format[format]).toLower();

        if (!check_file(location)) {
          delete_current_deck();
          return false;
        }
        item = loadGraphicsItem(location);
      }

    if (!item) {
      delete_current_deck(); // delete incomplete deck (safe if empty)
      return false;
    }

    apply_settings(item, BACK_CARD);
    deck.append(item);
  }

  emit deckChange(current_deck_style);

  return true;
}
