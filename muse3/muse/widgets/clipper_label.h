#ifndef _CLIPPER_LABEL_H_
#define _CLIPPER_LABEL_H_

#include <QFrame>
#include <QImage>

namespace MusEGui
{

class ClipperLabel : public QFrame
{
   Q_OBJECT
private:
   bool _isClipped;
   static QImage _numbersImg [2][13];
   static bool _imagesLoaded;
   double _value;
   QImage _resImg;
public:
   ClipperLabel(QWidget *parent = 0);
   virtual ~ClipperLabel();
   virtual QSize	minimumSizeHint() const;
   void setClipper(bool b);
   void setVal(double v, bool force = false);
protected:
   virtual void paintEvent(QPaintEvent *);
   virtual void resizeEvent(QResizeEvent *e);
   virtual void mousePressEvent(QMouseEvent *);

signals:
   void clicked();

};

}

#endif
