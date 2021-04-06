//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tlist.h,v 1.8.2.5 2008/01/19 13:33:46 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on sourceforge)
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

#include "type_defs.h"
#include "track.h"

#include <map>
#include <QWidget>


// Forward declarations:
class QKeyEvent;
class QLineEdit;
class QSpinBox;
class QMouseEvent;
class QPaintEvent;
class QScrollBar;
class QWheelEvent;
class QMenu;

namespace MusECore {
class Xml;
class Undo;
}

namespace MusEGui {
class Header;
class PopupMenu;

//---------------------------------------------------------
//   TList
//---------------------------------------------------------

class TList : public QWidget {
      Q_OBJECT

    Q_PROPERTY(bool sel3d READ sel3d WRITE setSel3d)
    Q_PROPERTY(bool curSelBorder READ curSelBorder WRITE setCurSelBorder)
    Q_PROPERTY(QColor curSelBorderColor READ curSelBorderColor WRITE setCurSelBorderColor)

  public:
    enum TrackColumn {
          COL_TRACK_IDX = 0,
          COL_INPUT_MONITOR,
          COL_RECORD,
          COL_MUTE,
          COL_SOLO,
          COL_CLASS,
          COL_NAME,
          COL_OPORT,
          COL_OCHANNEL,
//          COL_TIMELOCK,
          COL_AUTOMATION,
          COL_CLEF,
          COL_CUSTOM_MIDICTRL_OFFSET,
          COL_NONE = -1
          };

  private:
    bool _sel3d;
    bool _curSelBorder;
    QColor _curSelBorderColor;

      int ypos;
      bool editMode;
      bool editJustFinished;
      
      std::map<MusECore::Track*, std::map<int, int> > old_ctrl_hw_states;

      QPixmap bgPixmap;       // background Pixmap
      bool resizeFlag;        // true if resize cursor is shown

      QMenu *addTrackMenu, *insertTrackMenu;
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

      virtual void paintEvent(QPaintEvent*) override;
      virtual void mousePressEvent(QMouseEvent* event) override;
      virtual void mouseDoubleClickEvent(QMouseEvent*) override;
      virtual void mouseMoveEvent(QMouseEvent*) override;
      virtual void mouseReleaseEvent(QMouseEvent*) override;
      virtual void keyPressEvent(QKeyEvent* e) override;
      virtual void wheelEvent(QWheelEvent* e) override;
      virtual QSize sizeHint() const override;
      virtual QSize minimumSizeHint() const override;

      void showMidiClassPopupMenu(MusECore::Track*, int x, int y);
      void showAudioOutPopupMenu(MusECore::Track*, int x, int y);
      void moveSelection(int n);
      void adjustScrollbar();
      void paint(const QRect& r);
      void redraw(const QRect& r);
      MusECore::Track* y2Track(int) const;
      MusECore::TrackList getRecEnabledTracks();
      void setHeaderToolTips();
      PopupMenu* colorMenu(QColor c, int id, QWidget* parent);
      void setMute(MusECore::Undo& operations, MusECore::Track *t, bool turnOff, bool state);
      void changeTrackToType(MusECore::Track *t, MusECore::Track::TrackType trackType);
      void editTrackName(MusECore::Track *t);
      void setTrackChannel(MusECore::Track *t, bool isDelta, int channel, int delta, bool doAllTracks = false);
      void incrementController(MusECore::Track* t, int controllerType, int incrementValue);
      void addAutoMenuAction(PopupMenu* p, const MusECore::CtrlList *cl);
      void outputAutoMenuSorted(PopupMenu* p, QList<const MusECore::CtrlList *> &);


   protected:
      bool event(QEvent *) override;

   private slots:
      void maybeUpdateVolatileCustomColumns(); // updates AFFECT_CPOS-columns when and only when the hwState has changed
      void returnPressed();
      void chanValueFinished();
      void ctrlValueFinished();
      void instrPopupActivated(QAction*);
      void songChanged(MusECore::SongChangedStruct_t flags);
      void changeAutomation(QAction*);
      void changeAutomationColor(QAction*);
      void loadTrackDrummap(MusECore::MidiTrack*, const char* filename=NULL);
      void loadTrackDrummapFromXML(MusECore::MidiTrack*t, MusECore::Xml &xml);
      void saveTrackDrummap(MusECore::MidiTrack*, bool full, const char* filename=NULL);
      void copyTrackDrummap(MusECore::MidiTrack*, bool full);

   signals:
      void keyPressExt(QKeyEvent*);
      void redirectWheelEvent(QWheelEvent*);
      void verticalScrollSetYpos(int ypos);

   public slots:
      void tracklistChanged();
      void setYPos(int);
      void redraw();
      void selectTrack(MusECore::Track*, bool deselect=true);
      void selectTrackAbove();
      void selectTrackBelow();
      void editTrackNameSlot();
      void muteSelectedTracksSlot();
      void soloSelectedTracksSlot();
      void volumeSelectedTracksSlot(int);
      void panSelectedTracksSlot(int);

      void setHeader(Header*);

   public:
      TList(Header*, QWidget* parent, const char* name);
      void setScroll(QScrollBar* s) { _scroll = s; }
      MusECore::Track* track() const { return editTrack; }
      void populateAddTrack();
      bool sel3d() const { return _sel3d; }
      void setSel3d(bool sel3d) { _sel3d = sel3d; }
      bool curSelBorder() const { return _curSelBorder; }
      void setCurSelBorder(bool curSelBorder) { _curSelBorder = curSelBorder; }
      QColor curSelBorderColor() const { return _curSelBorderColor; }
      void setCurSelBorderColor(const QColor c) { _curSelBorderColor = c; }
      void moveSelectedTracks(bool up, bool full);
      };

} // namespace MusEGui

#endif

