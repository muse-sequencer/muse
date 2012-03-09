//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dlist.h,v 1.5.2.3 2009/10/16 21:50:16 terminator356 Exp $
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

#ifndef __DLIST_H__
#define __DLIST_H__

#include <QKeyEvent>
#include <QLineEdit>

#include "awl/pitchedit.h"
#include "view.h"

#define TH  18                // normal Track-hight

class QHeaderView;
class QLineEdit;
class QMouseEvent;
class QPainter;

namespace MusECore {
class DrumMap;
}

namespace MusEGui {

class ScrollScale;

//---------------------------------------------------------
//   DLineEdit
//---------------------------------------------------------
class DLineEdit: public QLineEdit
{
    public:
      DLineEdit(QWidget* parent) : QLineEdit(parent) {}
      virtual ~DLineEdit() {};
      
      virtual void keyPressEvent(QKeyEvent* keyItem) {
            if(keyItem->key() == Qt::Key_Escape) {
                parentWidget()->setFocus();
                hide();
                }
            else
               QLineEdit::keyPressEvent(keyItem);

            }
};

//---------------------------------------------------------
//   DPitchEdit
//---------------------------------------------------------
class DPitchEdit: public Awl::PitchEdit
{
    public:
      DPitchEdit(QWidget* parent) : PitchEdit(parent) {}
      virtual ~DPitchEdit() {};
      
      virtual void keyPressEvent(QKeyEvent* keyItem) {
            if ((keyItem->key() == Qt::Key_Escape) || (keyItem->key() == Qt::Key_Return)) {
                parentWidget()->setFocus();
                hide();
                }
            else
               PitchEdit::keyPressEvent(keyItem);
            }
};

//---------------------------------------------------------
//   DList
//---------------------------------------------------------

class DList : public View {
      Q_OBJECT
    
      QHeaderView* header;
      ScrollScale* scroll;
      QLineEdit* editor;
      DPitchEdit* pitch_editor;
      MusECore::DrumMap* editEntry;
      MusECore::DrumMap* currentlySelected;
      int selectedColumn;

      
      int startY;
      int curY;
      int sPitch;
      enum { NORMAL, START_DRAG, DRAG } drag;

      virtual void draw(QPainter& p, const QRect&);
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseReleaseEvent(QMouseEvent* event);
      virtual void viewMouseDoubleClickEvent(QMouseEvent*);
      virtual void viewMouseMoveEvent(QMouseEvent*);

      int x2col(int x) const;
      void devicesPopupMenu(MusECore::DrumMap* t, int x, int y, bool changeAll);
      
   private slots:
      void sizeChange(int, int, int);
      void returnPressed();
      void pitchEdited();
      void moved(int, int, int);

   signals:
      void channelChanged();
      void mapChanged(int, int);
      void keyPressed(int, int);
      void keyReleased(int, bool);
      void curDrumInstrumentChanged(int);

   public slots:
      void tracklistChanged();
      void songChanged(int);
   public:
      void lineEdit(int line, int section);
      void pitchEdit(int line, int section);
      void setCurDrumInstrument(int n);
      DList(QHeaderView*, QWidget* parent, int ymag);
      ~DList();
      void setScroll(ScrollScale* s) { scroll = s; }
      int getSelectedInstrument();

enum DCols { COL_MUTE=0, COL_NAME, COL_VOL, COL_QNT, COL_ENOTE, COL_LEN,
         COL_ANOTE, COL_CHANNEL, COL_PORT,
         COL_LV1, COL_LV2, COL_LV3, COL_LV4, COL_NONE=-1};
      };

} // namespace MusEGui

#endif // __DLIST_H_

