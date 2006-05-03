#ifndef __BIGTIME_H__
#define __BIGTIME_H__

#include "al/pos.h"

class MusE;
class AL::Pos;

//---------------------------------------------------------
//   BigTime
//---------------------------------------------------------

class BigTime : public QWidget {
      bool tickmode;
      MusE* seq;
      Q_OBJECT

      bool setString(AL::Pos);

      QLabel *barLabel, *beatLabel, *tickLabel,
             *hourLabel, *minLabel, *secLabel, *frameLabel,
             *sep1, *sep2, *sep3, *sep4, *sep5;
      int oldbar, oldbeat, oldhour, oldmin, oldsec, oldframe;
      int oldtick;

      void setFgColor(QColor c);
      void setBgColor(QColor c);

   protected:
      virtual void resizeEvent(QResizeEvent*);
      virtual void closeEvent(QCloseEvent*);

   public slots:
      void setPos(int, AL::Pos, bool);
      void configChanged();

   signals:
      void closed();

   public:
      BigTime(QWidget* parent);
      };

#endif
