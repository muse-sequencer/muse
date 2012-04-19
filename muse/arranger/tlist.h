//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tlist.h,v 1.8.2.5 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __TLIST_H__
#define __TLIST_H__

#include "track.h"
#include <map>
#include <QWidget>

class QWidget;
class QKeyEvent;
class QLineEdit;
class QSpinBox;
class QMouseEvent;
class QPaintEvent;
class QScrollBar;
class QWheelEvent;

namespace MusECore {
class Track;
class Xml;
}

namespace MusEGui {
class Header;
class PopupMenu;
class ScrollScale;

enum TrackColumn {
      COL_RECORD = 0,
      COL_MUTE,
      COL_SOLO,
      COL_CLASS,
      COL_NAME,
      COL_OPORT,
      COL_OCHANNEL,
      COL_TIMELOCK,
      COL_AUTOMATION,
      COL_CLEF,
      COL_CUSTOM_MIDICTRL_OFFSET,
      COL_NONE = -1
      };

//---------------------------------------------------------
//   TList
//---------------------------------------------------------

class TList : public QWidget {
      Q_OBJECT

      int ypos;
      bool editMode;
      bool editJustFinished;
      
      std::map<MusECore::Track*, std::map<int, int> > old_ctrl_hw_states;

      QPixmap bgPixmap;       // background Pixmap
      bool resizeFlag;        // true if resize cursor is shown

      Header* header;
      QScrollBar* _scroll;
      QLineEdit* editor;
      QSpinBox* chan_edit;
      QSpinBox* ctrl_edit;
      int ctrl_num;
      unsigned ctrl_at_tick;
      MusECore::Track* editTrack;
      MusECore::Track* editAutomation;

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

      void portsPopupMenu(MusECore::Track*, int, int);
      void oportPropertyPopupMenu(MusECore::Track*, int x, int y);
      void moveSelection(int n);
      void adjustScrollbar();
      void paint(const QRect& r);
      void redraw(const QRect& r);
      MusECore::Track* y2Track(int) const;
      void classesPopupMenu(MusECore::Track*, int x, int y);
      MusECore::TrackList getRecEnabledTracks();
      void setHeaderToolTips();
      PopupMenu* colorMenu(QColor c, int id, QWidget* parent);

   protected:
      bool event(QEvent *);
   private slots:
      void maybeUpdateVolatileCustomColumns(); // updates AFFECT_CPOS-columns when and only when the hwState has changed
      void returnPressed();
      void chanValueFinished();
      void ctrlValueFinished();
      void instrPopupActivated(QAction*);
      void songChanged(int flags);
      void changeAutomation(QAction*);
      void changeAutomationColor(QAction*);

   signals:
      void selectionChanged(MusECore::Track*);
      void keyPressExt(QKeyEvent*);
      void redirectWheelEvent(QWheelEvent*);

   public slots:
      void tracklistChanged();
      void setYPos(int);
      void redraw();
      void selectTrack(MusECore::Track*);
      void selectTrackAbove();
      void selectTrackBelow();
      void setHeader(Header*);

   public:
      TList(Header*, QWidget* parent, const char* name);
      void setScroll(QScrollBar* s) { _scroll = s; }
      MusECore::Track* track() const { return editTrack; }
      };

} // namespace MusEGui

#endif

