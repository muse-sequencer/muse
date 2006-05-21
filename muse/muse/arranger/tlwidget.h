//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __TLWIDGET_H__
#define __TLWIDGET_H__

class Track;
class TrGroupList;
class TLWidgetLayout;
class SimpleButton;

//---------------------------------------------------------
//   TLWidget
//---------------------------------------------------------

class TLWidget : public QWidget {
      Q_OBJECT

      enum { S_NORMAL, S_DRAGBOTTOM, S_DRAG };
      int state;

      int trackIdx;
      int starty;
      Track* _track;
      QColor bgColor;
      QColor selectBgColor;

      QLineEdit* nameEdit;
      TLWidgetLayout* l;
      TrGroupList* tel;
      QSpinBox* outChannel;
      QComboBox* outPort;
      SimpleButton* off;
      SimpleButton* plus;
      SimpleButton* record;
      std::vector<QWidget*> wlist;
      QComboBox* instrument;
      QLabel* label;

      QTime startDragTime;
      QPoint startDragPos;

      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent*);
      virtual void mouseReleaseEvent(QMouseEvent*);
      virtual void mouseMoveEvent(QMouseEvent*);
      virtual void dragEnterEvent(QDragEnterEvent*);
      virtual void dropEvent(QDropEvent*);

   signals:
      void startDrag(int idx);
      void drag(int idx, int);

   private slots:
      void nameChanged(QString);
      void labelPlusClicked();
      void configChanged();
      void recordToggled(bool);
      void muteToggled(bool);
      void offToggled(bool);
      void soloToggled(bool);
      void monitorToggled(bool);
      void drumMapToggled(bool);
      void selectionChanged();
      void outChannelChanged(int);
      void setOutChannel(int);
      void setOutPort(int);
      void autoReadToggled(bool val);
      void autoWriteToggled(bool val);
      void instrumentSelected(int);
      void instrumentChanged();
      void updateOffState();

   public slots:
      void select();

   signals:
      void plusClicked(TLWidget*);
      void moveTrack(Track* src, Track* dst);

   public:
      TLWidget(Track* t, TrGroupList*);
      Track* track() const { return _track; }
      void setIdx(int n) { trackIdx = n; }
      };

#endif

