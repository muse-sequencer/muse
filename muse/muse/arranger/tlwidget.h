//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: tlwidget.h,v 1.21 2006/02/03 16:46:38 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

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

