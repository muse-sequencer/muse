#include <QPaintEvent>
#include <QPainter>
#include <QStyle>
#include <QLocale>

#include "fastlog.h"
#include "clipper_label.h"

namespace MusEGui
{

ClipperLabel::ClipperLabel(QWidget *parent):
   QFrame(parent),
   _isClipped(false),
   _value(0.0)
{   
      // Background is drawn by us.
      setBackgroundRole(QPalette::NoRole);
      setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      setAttribute(Qt::WA_OpaquePaintEvent);    
  
      //setFrameStyle(QFrame::Box | QFrame::Sunken);
      setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
      
      //setLineWidth(1); // Not really required - StyledPanel always has 1 width.
      //setMidLineWidth(0);
      
      const int fw = frameWidth();
      setContentsMargins(fw, fw, fw, fw);
//    setProperty("clipped", "false");

      const QColor fc(255, 75, 75);
      const QColor fcd = fc.darker(150);
      _onGradient.setColorAt(0.0, fcd);
      _onGradient.setColorAt(0.5, fc);
      _onGradient.setColorAt(1.0, fcd);
      
   setVal(_value, true);
}
  
QSize ClipperLabel::sizeHint() const
{
  const int fw = frameWidth();
  const QSize sz = fontMetrics().boundingRect("-88.8.").size();
  return QSize(sz.width() + 2 * fw, sz.height() + 2 * fw);
}

void ClipperLabel::paintEvent(QPaintEvent *e)
{   
  const QRect& r = frameRect();
  QPainter p;
  p.begin(this);
  if(_isClipped)
    p.fillRect(r, _onGradient);
  else
    p.fillRect(r, palette().window());
  p.end();

  QFrame::paintEvent(e);
  
  p.begin(this);
  if(_isClipped)
    p.setPen(Qt::white);
  //p.drawText(e->rect(), Qt::AlignCenter, _text);  // Drawing artifacts?
  p.drawText(contentsRect(), Qt::AlignCenter, _text);
  p.end();
}

void ClipperLabel::resizeEvent(QResizeEvent *)
{
  _onGradient.setStart(0, frameRect().y());
  _onGradient.setFinalStop(0, frameRect().y() + frameRect().height() - 1);
}

void ClipperLabel::mousePressEvent(QMouseEvent *ev)
{
  ev->accept();
   emit clicked();
}

void ClipperLabel::mouseReleaseEvent(QMouseEvent *ev)
{
  ev->accept();
}

void ClipperLabel::mouseMoveEvent(QMouseEvent *ev)
{
  ev->accept();
}

void ClipperLabel::contextMenuEvent(QContextMenuEvent * ev)
{
  ev->accept();
}

void ClipperLabel::setClipped(bool b)
{
   if(b != _isClipped)
   {
      _isClipped = b;
      setVal(_value, true);
      update();
   }
}

void ClipperLabel::setVal(double v, bool force)
{
   if((v == _value) && !force)
   {
      return;
   }

   _value = v;

      v = MusECore::fast_log10(v) * 20.0;
      
      if(v >= -60.0f)
      {
        _text = locale().toString(v, 'f', 1);
      }
      else
      {
        _text = QString("-");
        _text += QChar(0x221e); // The infinty character
        
      }

   update();

}

}

