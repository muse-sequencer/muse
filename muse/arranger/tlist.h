//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tlist.h,v 1.8.2.5 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __TLIST_H__
#define __TLIST_H__

#include <qtooltip.h>
#include <q3whatsthis.h>
#include <q3header.h>
//Added by qt3to4:
#include <QPixmap>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include "track.h"

class QPainter;
class ScrollScale;
class QScrollBar;
class QLineEdit;
class Track;
class Xml;
class Header;

enum TrackColumn {
      COL_RECORD = 0,
      COL_MUTE,
      COL_SOLO,
      COL_CLASS,
      COL_NAME,
      COL_OPORT,
      COL_OCHANNEL,
      COL_TIMELOCK,
      COL_NONE = -1
      };
//      COL_AUTOMATION, -- not enabled

//----------------------------------------------------------
//   THeaderTip
//----------------------------------------------------------

class THeaderTip { //: public QToolTip { ddskrjo

   public:
    THeaderTip(QWidget * parent) {} //: QToolTip(parent) {} ddskrjo
      virtual ~THeaderTip() {}
   protected:
      void maybeTip(const QPoint &);
      };

//---------------------------------------------------------
//   TList
//---------------------------------------------------------

class TList : public QWidget {
      Q_OBJECT

      int ypos;
      bool editMode;

      QPixmap pm;             // for double buffering
      bool pmValid;
      QPixmap bgPixmap;       // background Pixmap

      bool resizeFlag;        // true if resize cursor is shown

      Header* header;
      QScrollBar* scroll;
      QLineEdit* editor;
      Track* editTrack;

      int startY;
      int curY;
      int sTrack;
      int dragHeight;
      int dragYoff;

      enum { NORMAL, START_DRAG, DRAG, RESIZE} mode;

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent* event);
      virtual void mouseDoubleClickEvent(QMouseEvent*);
      virtual void mouseMoveEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent*);
      virtual void keyPressEvent(QKeyEvent* e);
      virtual void wheelEvent(QWheelEvent* e);

      void portsPopupMenu(Track*, int, int);
      void oportPropertyPopupMenu(Track*, int x, int y);
      void moveSelection(int n);
      void adjustScrollbar();
      void paint(const QRect& r);
      virtual void resizeEvent(QResizeEvent*);
      void redraw(const QRect& r);
      Track* y2Track(int) const;
      void classesPopupMenu(Track*, int x, int y);
      TrackList getRecEnabledTracks();

   private slots:
      void returnPressed();
      void songChanged(int flags);

   signals:
      void selectionChanged();
      void keyPressExt(QKeyEvent*);
      void redirectWheelEvent(QWheelEvent*);

   public slots:
      void tracklistChanged();
      void setYPos(int);
      void redraw();
      void selectTrack(Track*);
      void selectTrackAbove();
      void selectTrackBelow();

   public:
      TList(Header*, QWidget* parent, const char* name);
      void setScroll(QScrollBar* s) { scroll = s; }
      Track* track() const { return editTrack; }
      void writeStatus(int level, Xml&, const char* name) const;
      void readStatus(Xml&, const char* name);
      };

#endif

