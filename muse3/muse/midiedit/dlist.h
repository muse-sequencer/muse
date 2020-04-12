//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dlist.h,v 1.5.2.3 2009/10/16 21:50:16 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __DLIST_H__
#define __DLIST_H__

#include <QKeyEvent>
#include <QLineEdit>
#include <QSpinBox>

#include "type_defs.h"
#include "pitchedit.h"
#include "view.h"

#define TH  18                // normal Track-hight

// REMOVE Tim. newdrums. Added.
// Adds the ability to override at instrument level.
// But it just makes things too complex for the user.
// And in a way is unnecessary and overkill, since we
//  already allow modifying an instrument.
//#define _USE_INSTRUMENT_OVERRIDES_

class QHeaderView;
class QMouseEvent;
class QPainter;

namespace MusECore {
struct DrumMap;
}

namespace MusEGui {

class ScrollScale;
class DrumCanvas;
class PitchEdit;

enum DrumColumn {
  COL_HIDE = 0,
  COL_MUTE,
  COL_NAME,
  COL_VOLUME,
  COL_QUANT,
  COL_INPUTTRIGGER,
  COL_NOTELENGTH,
  COL_NOTE,
  COL_OUTCHANNEL,
  COL_OUTPORT,
  COL_LEVEL1,
  COL_LEVEL2,
  COL_LEVEL3,
  COL_LEVEL4,
  COL_NONE = -1
};


//---------------------------------------------------------
//   DLineEdit
//---------------------------------------------------------

class DLineEdit: public QLineEdit
{
  Q_OBJECT

  protected:
    virtual bool event(QEvent*);

  signals:
    void returnPressed();
    void escapePressed();

  public:
    DLineEdit(QWidget* parent);
};

//---------------------------------------------------------
//   DrumListSpinBox
//---------------------------------------------------------

class DrumListSpinBox : public QSpinBox {
  Q_OBJECT

  protected:
    virtual bool event(QEvent*);

  signals:
    void returnPressed();
    void escapePressed();

  public:
    DrumListSpinBox(QWidget* parent=0);
};

//---------------------------------------------------------
//   DPitchEdit
//---------------------------------------------------------

class DPitchEdit: public PitchEdit
{
  Q_OBJECT

  protected:
    virtual bool event(QEvent*);

  //signals:
    //void returnPressed();
    //void escapePressed();

  public:
    DPitchEdit(QWidget* parent);
};

//---------------------------------------------------------
//   DList
//---------------------------------------------------------

class DList : public View {
      Q_OBJECT

      MusEGui::DrumCanvas* dcanvas;
      MusECore::DrumMap* ourDrumMap;
      int ourDrumMapSize;
// REMOVE Tim. midnam. Removed. Old drum not used any more.
//       bool old_style_drummap_mode;
      
      QHeaderView* header;
      QLineEdit* editor;
      DrumListSpinBox* val_editor;
      DPitchEdit* pitch_editor;
      MusECore::DrumMap* editEntry;
      MusECore::DrumMap* currentlySelected;
      int selectedColumn;

      
      int startY;
      int curY;
      int sInstrument;
      enum { NORMAL, START_DRAG, DRAG } drag;

      virtual void draw(QPainter&, const QRect&, const QRegion& = QRegion());
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseReleaseEvent(QMouseEvent* event);
      virtual void viewMouseDoubleClickEvent(QMouseEvent*);
      virtual void viewMouseMoveEvent(QMouseEvent*);
      virtual void wheelEvent(QWheelEvent* e);

      int x2col(int x) const;
      // Returns -1 if invalid.
      int col2Field(int col) const;
      // Returns -1 if invalid.
      int field2Col(int field) const;
      bool devicesPopupMenu(MusECore::DrumMap* t, int x, int y);

      void init(QHeaderView*, QWidget*);

   private slots:
      void sizeChange(int, int, int);
      void escapePressed();
      void returnPressed();
      void valEdited();
      void pitchEdited();
      void moved(int, int, int);

   signals:
      void channelChanged();
      void mapChanged(int, int);
      void keyPressed(int, int);
      void keyReleased(int, bool);
      void curDrumInstrumentChanged(int);
      void redirectWheelEvent(QWheelEvent*);

   public slots:
      void tracklistChanged();
      void songChanged(MusECore::SongChangedStruct_t);
      void ourDrumMapChanged(bool);
   
   public:
      void lineEdit(int line, int section);
      void valEdit(int line, int section);
      void pitchEdit(int line, int section);
      void setCurDrumInstrument(int n);
// REMOVE Tim. midnam. Changed. Old drum not used any more.
//       DList(QHeaderView*, QWidget* parent, int ymag, DrumCanvas* dcanvas, bool oldstyle);
      DList(QHeaderView*, QWidget* parent, int ymag, DrumCanvas* dcanvas);
      DList(QHeaderView* h, QWidget* parent, int ymag, MusECore::DrumMap* dm, int dmSize=128);
      virtual ~DList();
      int getSelectedInstrument();

      };

} // namespace MusEGui

#endif // __DLIST_H_

