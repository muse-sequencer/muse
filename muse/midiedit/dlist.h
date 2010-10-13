//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: dlist.h,v 1.5.2.3 2009/10/16 21:50:16 terminator356 Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __DLIST_H__
#define __DLIST_H__

#include <qlineedit.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QKeyEvent>
#include "view.h"

#define TH  18                // normal Track-hight

class QPainter;
class Q3Header;
class ScrollScale;
class Device;
class QLineEdit;
class DrumMap;


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
//   DList
//---------------------------------------------------------

class DList : public View {
      Q3Header* header;
      ScrollScale* scroll;
      QLineEdit* editor;
      DrumMap* editEntry;
      DrumMap* currentlySelected;
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
      void devicesPopupMenu(DrumMap* t, int x, int y, bool changeAll);
      Q_OBJECT
      //void setCurDrumInstrument(int n);

   private slots:
      void sizeChange(int, int, int);
      void returnPressed();
      void moved(int, int);

   signals:
      void channelChanged();
      void mapChanged(int, int);
      void keyPressed(int, bool);
      void keyReleased(int, bool);
      void curDrumInstrumentChanged(int);

   public slots:
      void tracklistChanged();
      void songChanged(int);
   public:
      void lineEdit(int line, int section);
      void setCurDrumInstrument(int n);
      DList(Q3Header*, QWidget* parent, int ymag);
      ~DList();
      void setScroll(ScrollScale* s) { scroll = s; }
      int getSelectedInstrument();

enum DCols { COL_MUTE=0, COL_NAME, COL_VOL, COL_QNT, COL_ENOTE, COL_LEN,
         COL_ANOTE, COL_CHANNEL, COL_PORT,
         COL_LV1, COL_LV2, COL_LV3, COL_LV4, COL_NONE=-1};
      };

#endif // __DLIST_H_

