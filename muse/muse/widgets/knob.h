#ifndef QWT_KNOB_H
#define QWT_KNOB_H

#include <sliderbase.h>
#include <sclif.h>
#include <qcolor.h>


//---------------------------------------------------------
//   Knob
//---------------------------------------------------------

class Knob : public SliderBase, public ScaleIf
      {
  Q_OBJECT

   public:
      enum Symbol { Line, Dot };

   private:
      bool hasScale;

      int d_borderWidth;
      int d_borderDist;
      int d_scaleDist;
      int d_maxScaleTicks;
      int d_newVal;
      int d_knobWidth;
      int d_dotWidth;

      Symbol d_symbol;
      double d_angle;
      double d_oldAngle;
      double d_totalAngle;
      double d_nTurns;

      QRect  kRect;
      bool _faceColSel;
      QColor d_faceColor;
      QColor d_curFaceColor;
      QColor d_altFaceColor;
      QColor d_markerColor;

      void recalcAngle();
      void valueChange();
      void rangeChange();
      void drawKnob(QPainter *p, const QRect &r);
      void drawMarker(QPainter *p, double arc, const QColor &c);

      void paintEvent(QPaintEvent *e);
      void resizeEvent(QResizeEvent *e);
      double getValue(const QPoint &p);
      void getScrollMode( QPoint &p, const Qt::ButtonState &button, int &scrollMode, int &direction );
      void scaleChange()             { repaint(false); }
      void fontChange(const QFont &) { repaint(false); }

   public:
      Knob(QWidget* parent = 0, const char *name = 0);
      ~Knob() {}

      void setKnobWidth(int w);
      void setTotalAngle (double angle);
      void setBorderWidth(int bw);
      void selectFaceColor(bool alt);
      bool selectedFaceColor() { return _faceColSel; }
      QColor faceColor() { return d_faceColor; }
      void setFaceColor(const QColor c);
      QColor altFaceColor() { return d_altFaceColor; }
      void setAltFaceColor(const QColor c);
      QColor markerColor() { return d_markerColor; }
      void setMarkerColor(const QColor c);
      };


#endif
