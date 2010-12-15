#ifndef __BIGTIME_H__
#define __BIGTIME_H__

#include <QWidget>

class QCheckBox;
class QLabel;

class MusE;

//---------------------------------------------------------
//   BigTime
//---------------------------------------------------------

class BigTime : public QWidget {
      bool tickmode;
      MusE* seq;
      Q_OBJECT

      bool setString(unsigned);

      QWidget *dwin;
      QCheckBox *fmtButton;
      QLabel *absTickLabel;
      QLabel *absFrameLabel;
      QLabel *barLabel, *beatLabel, *tickLabel,
             //*hourLabel, *minLabel, *secLabel, *frameLabel,
             *minLabel, *secLabel, *frameLabel, *subFrameLabel, 
             *sep1, *sep2, *sep3, *sep4, *sep5;
      
      //int oldbar, oldbeat, oldhour, oldmin, oldsec, oldframe;
      int oldbar, oldbeat, oldmin, oldsec, oldframe, oldsubframe;
      unsigned oldtick;
      unsigned oldAbsTick, oldAbsFrame;
      void setFgColor(QColor c);
      void setBgColor(QColor c);

   protected:
      virtual void resizeEvent(QResizeEvent*);
      virtual void closeEvent(QCloseEvent*);

   public slots:
      void setPos(int, unsigned, bool);
      void configChanged();
      void fmtButtonToggled(bool);
   signals:
      void closed();

   public:
      BigTime(QWidget* parent);
      };

#endif
