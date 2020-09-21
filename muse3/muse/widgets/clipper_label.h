#ifndef _CLIPPER_LABEL_H_
#define _CLIPPER_LABEL_H_

#include <QFrame>
#include <QLinearGradient>
#include <QSize>
#include <QString>

class QPaintEvent;
class QResizeEvent;
class QMouseEvent;
class QContextMenuEvent;

namespace MusEGui
{
class ClipperLabel : public QFrame
{
   Q_OBJECT
   Q_PROPERTY( bool clipped READ clipped WRITE setClipped )

private:
   bool _isClipped;
   double _value;
   QString _text;
   QLinearGradient _onGradient;
   
public:
   ClipperLabel(QWidget *parent = 0);
   virtual QSize sizeHint() const;
   bool clipped() const { return _isClipped; }
   void setClipped(bool b);
   void setVal(double v, bool force = false);
   
protected:
   virtual void paintEvent(QPaintEvent *);
   virtual void resizeEvent(QResizeEvent *e);
   virtual void mousePressEvent(QMouseEvent *);
   virtual void mouseReleaseEvent(QMouseEvent *);
   virtual void mouseMoveEvent(QMouseEvent *);
   virtual void contextMenuEvent(QContextMenuEvent*);

signals:
   void clicked();

};

}

#endif
