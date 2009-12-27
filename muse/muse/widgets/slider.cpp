#include <cmath>
#include "mmath.h"
#include <qdrawutil.h>
#include <qpainter.h>
#include <qcursor.h>

#include "slider.h"

//-------------------------------------------------------------
//  Slider - The Slider Widget
//
//  Slider is a slider widget which operates on an interval
//  of type double. Slider supports different layouts as
//  well as a scale.
//------------------------------------------------------------

//------------------------------------------------------------
//.F  Slider::Slider
//
//    Constructor
//
//.u  Syntax:
//.f  Slider::Slider(QWidget *parent, const char *name, Orientation orient = Horizontal, ScalePos scalePos = None, int bgStyle = BgTrough)
//
//.u  Parameters
//.p
//  QWidget *parent --  parent widget
//  const char *name -- The Widget's name. Default = 0.
//  Orientation Orient -- Orientation of the slider. Can be Slider::Horizontal
//        or Slider::Vertical.
//                    Defaults to Horizontal.
//  ScalePos scalePos --  Position of the scale.  Can be Slider::None,
//        Slider::Left, Slider::Right, Slider::Top,
//        or Slider::Bottom. Defaults to Slider::None.
//  int bgStyle --  Background style. Slider::BgTrough draws the
//        slider button in a trough, Slider::BgSlot draws
//        a slot underneath the button. An or-combination of both
//        may also be used. The default is Slider::BgTrough.
//------------------------------------------------------------

Slider::Slider(QWidget *parent, const char *name,
   Orientation orient, ScalePos scalePos, int bgStyle)
      : SliderBase(parent,name)
      {
      if (bgStyle == BgSlot) {
            d_thumbLength = 16;
            d_thumbHalf = 8;
            d_thumbWidth = 30;
            }
      else {
            d_thumbLength = 30;
            d_thumbHalf = 15;
            d_thumbWidth = 16;
            }

      d_borderWidth = 2;
      d_scaleDist   = 4;
      d_scaleStep   = 0.0;
      d_scalePos    = scalePos;
      d_xMargin     = 0;
      d_yMargin     = 0;
      d_bgStyle     = bgStyle;

      if (bgStyle & BgTrough)
            d_bwTrough = d_borderWidth;
      else
            d_bwTrough = 0;

      d_sliderRect.setRect(0, 0, 8, 8);
      setOrientation(orient);
      }

//------------------------------------------------------------
//.F  Slider::~Slider
//    Destructor
//.u  Syntax
//.f  Slider::~Slider()
//------------------------------------------------------------

Slider::~Slider()
      {
      }

//------------------------------------------------------------
//
//.F  Slider::setBorderWidth
//  Change the slider's border width
//
//.u  Syntax
//.f  void Slider::setBorderWidth(int bd)
//
//.u  Parameters
//.p  int bd -- border width
//
//------------------------------------------------------------

void Slider::setBorderWidth(int bd)
{
    d_borderWidth = qwtMin(qwtMax(bd,0),10);
    if (d_bgStyle & BgTrough)
       d_bwTrough = d_borderWidth;
    else
       d_bwTrough = 0;
}

//----------------------------------------------------
//
//.F  Slider::setThumbLength
//
//    Set the slider's thumb length
//
//.u  Syntax
//  void Slider::setThumbLength(int l)
//
//.u  Parameters
//.p  int l   --    new length
//
//-----------------------------------------------------
void Slider::setThumbLength(int l)
{
    d_thumbLength = qwtMax(l,8);
    d_thumbHalf = d_thumbLength / 2;
    resize(size());
}

//------------------------------------------------------------
//
//.F  Slider::setThumbWidth
//  Change the width of the thumb
//
//.u  Syntax
//.p  void Slider::setThumbWidth(int w)
//
//.u  Parameters
//.p  int w -- new width
//
//------------------------------------------------------------
void Slider::setThumbWidth(int w)
{
    d_thumbWidth = qwtMax(w,4);
    resize(size());
}


//------------------------------------------------------------
//.-  
//.F  Slider::scaleChange
//  Notify changed scale
//
//.u  Syntax
//.f  void Slider::scaleChange()
//
//.u  Description
//  Called by QwtScaledWidget
//
//------------------------------------------------------------
void Slider::scaleChange()
{
    if (!hasUserScale())
       d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor);
    update();
}


//------------------------------------------------------------
//.-
//.F  Slider::fontChange
//  Notify change in font
//  
//.u  Syntax
//.f   Slider::fontChange(const QFont &oldFont)
//
//------------------------------------------------------------
void Slider::fontChange(const QFont & /*oldFont*/)
{
    repaint(FALSE);
}

//------------------------------------------------------------
//    drawSlider
//     Draw the slider into the specified rectangle.  
//------------------------------------------------------------

void Slider::drawSlider(QPainter *p, const QRect &r)
      {
      QColorGroup g = colorGroup();
      QBrush brBack(g.background());
      QBrush brMid;
      QBrush brDark(g.dark());

      QRect cr;

      int ipos,dist1;
      double rpos;
      int lineDist;

      if (d_bwTrough > 0) {
            qDrawShadePanel(p, r.x(), r.y(),
      r.width(), r.height(),
      g, TRUE, d_bwTrough,0);
        cr.setRect(r.x() + d_bwTrough,
       r.y() + d_bwTrough,
       r.width() - 2*d_bwTrough,
       r.height() - 2*d_bwTrough);
        brMid = QBrush(g.mid());
            }
    else {
            cr = r;
            brMid = brBack;
            }

    rpos = (value()  - minValue()) / (maxValue() - minValue());

    lineDist = d_borderWidth - 1;
    if (lineDist < 1) lineDist = 1;

    if (d_orient == Horizontal)
    {
  
  dist1 = int(double(cr.width() - d_thumbLength) * rpos);
  ipos =  cr.x() + dist1;
  markerPos = ipos + d_thumbHalf;

  //
  // draw background
  //
  if (d_bgStyle & BgSlot)
  {
      drawHsBgSlot(p, cr, QRect(ipos, cr.y(), d_thumbLength, cr.height()), brMid);
  }
  else
  {
      p->fillRect(cr.x(),cr.y(),dist1,cr.height(),brMid);
      p->fillRect(ipos + d_thumbLength, cr.y(),
           cr.width() - d_thumbLength - dist1, cr.height(),brMid);
  }
  
  //
  //  Draw thumb
  //
  qDrawShadePanel(p,ipos, cr.y(), d_thumbLength, cr.height(),
      colorGroup(), FALSE, d_borderWidth, &brBack);
  
  if (lineDist > 1)
     qDrawShadeLine(p,markerPos, cr.y() + lineDist , markerPos,
        cr.y() + cr.height() - lineDist,
        colorGroup(), TRUE, 1);
  else
  {
      p->setPen(g.dark());
      p->drawLine(markerPos -1 , cr.y() + lineDist, markerPos -1,
      cr.y() + cr.height() - lineDist - 1);
      p->setPen(g.light());
      p->drawLine(markerPos, cr.y() + lineDist, markerPos,
      cr.y() + cr.height() - lineDist - 1);
  }
  
      
    }
    else
    {
  dist1 = int(double(cr.height() - d_thumbLength) * (1.0 - rpos));
  ipos = cr.y() + dist1;
  markerPos = ipos + d_thumbHalf;

  if ( d_bgStyle & BgSlot)
  {
      drawVsBgSlot(p, cr, QRect(cr.left(), ipos, cr.width(),
              d_thumbLength), brMid);
  }
  else
  {
      p->fillRect(cr.x(),cr.y(),cr.width(),ipos,brMid);
      p->fillRect(cr.x(), ipos + d_thumbLength, cr.width(),
      cr.height() - d_thumbLength - dist1, brMid);
  }
  
  qDrawShadePanel(p,cr.x(),ipos , cr.width(), d_thumbLength,
      colorGroup(),FALSE,d_borderWidth, &brBack);
  if (lineDist > 1)
     qDrawShadeLine(p, cr.x() + lineDist , markerPos,
        cr.x() + cr.width() - lineDist, markerPos,
        colorGroup(), TRUE, 1);
  else {
    
    p->setPen(g.dark());
    p->drawLine(cr.x() + lineDist, markerPos - 1 ,
          cr.x() + cr.width() -  lineDist - 1, markerPos - 1);
    p->setPen(g.light());
    p->drawLine(cr.x() + lineDist, markerPos,
          cr.x() + cr.width() -  lineDist - 1 , markerPos);
      }
    }

}

//------------------------------------------------------------
//.-
//.F  Slider::drawSlotBg
//
//
//.u  Syntax
//.f  void Slider::drawSlotBg(QPainter *p, const QRect &rBound, const QRect &rThumb, const QRect &rSlot, const QBrush &brBack)
//
//.u  Parameters
//.p  QPainter *p, const QRect &rBound, const QRect &rThumb, const QRect &rSlot, const QBrush &brBack
//
//------------------------------------------------------------
void Slider::drawHsBgSlot(QPainter *p, const QRect &rBound, const QRect &rThumb, const QBrush &brBack)
{
    int ws, ds, dLeft;
    int lPos, rPos;
    QRect rSlot;
    const QColorGroup &g = colorGroup();

    ws = rBound.height();
    if ((ws / 2) * 2 != ws)
       ws = 5;
    else
       ws = 4;

    ds = qwtMax(1, d_thumbLength/2 - 4);
    dLeft = rThumb.left() - rBound.left();

    rSlot = QRect(rBound.x() + ds, rBound.y() + (rBound.height() - ws) / 2,
      rBound.width() - 2 * ds, ws);

    rPos = qwtMin(rSlot.x(), rThumb.left());

    if (rThumb.left() > rBound.x())
    {
  p->fillRect(rBound.x(),rBound.y(),dLeft, rSlot.top() - rBound.top(), brBack);
  p->fillRect(rBound.x(),rSlot.bottom() + 1,dLeft,
        rBound.bottom() - rSlot.bottom(),brBack);
  if (rPos > rBound.left())
     p->fillRect(rBound.x(),rSlot.y(),
           rPos - rBound.left(),ws,brBack);

  p->setPen(g.dark());
  if (rSlot.x() < rThumb.left())
     p->drawLine(rSlot.x(), rSlot.bottom(), rSlot.x(), rSlot.top());
  if (rSlot.x() < rThumb.left() - 1)
  {
      p->drawLine(rSlot.x(), rSlot.top(), rThumb.left() - 1, rSlot.top());
      p->setPen(g.light());
      p->drawLine(rSlot.x() + 1, rSlot.bottom(),
      rThumb.left() - 1, rSlot.bottom());
  
      p->fillRect(rSlot.x() + 1, rSlot.y() + 1, dLeft - ds -1,
      rSlot.height() -2, QBrush(black));
  
  }
    }

    lPos = qwtMax(rSlot.right(), rThumb.right()) + 1;
    if (rThumb.right() < rBound.right())
    {
  p->fillRect(rThumb.right() + 1,rBound.y(),rBound.right() - rThumb.right(),
        rSlot.top() - rBound.top(), brBack);
  p->fillRect(rThumb.right() + 1,rSlot.bottom() + 1,
        rBound.right() - rThumb.right(),
        rBound.bottom() - rSlot.bottom(),brBack);
  if (lPos <= rBound.right())
     p->fillRect(lPos, rSlot.y() , rBound.right() - lPos + 1, ws ,brBack);

  p->setPen(g.dark());
  if (rSlot.right() > rThumb.right())
  {
      p->drawLine(rThumb.right() + 1, rSlot.top(), rSlot.right(), rSlot.top());
      p->setPen(g.light());
      p->drawLine(rSlot.right(), rSlot.bottom(), rSlot.right(), rSlot.top() + 1);
  }

  if (rSlot.right() > rThumb.right() + 1)
  {
      p->setPen(g.light());
      p->drawLine(rThumb.right() + 1, rSlot.bottom(),
      rSlot.right() -1, rSlot.bottom());
      p->fillRect(rThumb.right() + 1, rSlot.y() + 1,
      rSlot.right() - rThumb.right() - 1,
      rSlot.height() -2, QBrush(black));
  }
    }

}

//------------------------------------------------------------
//.-
//.F  Slider::drawVsBgSlot
//
//
//.u  Syntax
//.f  void Slider::drawVsBgSlot(QPainter *p, const QRect &rBound, const QRect &rThumb, const QBrush &brBack)
//
//.u  Parameters
//.p  QPainter *p, const QRect &rBound, const QRect &rThumb, const QBrush &brBack
//
//.u  Return Value
//
//.u  Description
//
//------------------------------------------------------------
void Slider::drawVsBgSlot(QPainter *p, const QRect &rBound, const QRect &rThumb, const QBrush &brBack)
{

    int ws, ds, dTop;
    int lPos, hPos;
    QRect rSlot;
    const QColorGroup &g = colorGroup();

    ws = rBound.width();
    if ((ws / 2) * 2 != ws)
       ws = 5;
    else
       ws = 4;

    ds = qwtMax(1, d_thumbLength/2 - 4);
    dTop = rThumb.top() - rBound.top();

    rSlot = QRect(rBound.x() + (rBound.width() - ws) / 2, rBound.y() + ds,
      ws, rBound.height() - 2 * ds);

    hPos = qwtMin(rSlot.y(), rThumb.top());

    if (rThumb.top() > rBound.top())
    {
  p->fillRect(rBound.x(),rBound.y(), rSlot.left() - rBound.left(),dTop, brBack);
  p->fillRect(rSlot.right() + 1, rBound.y(),
        rBound.right() - rSlot.right(), dTop,brBack);
  if (hPos > rBound.top())
     p->fillRect(rSlot.x(),rBound.y(), ws,
           hPos - rBound.top(),brBack);

  p->setPen(g.dark());
  if (rSlot.top() < rThumb.top())
     p->drawLine(rSlot.left(), rSlot.top(), rSlot.right(), rSlot.top());

  
  if (rSlot.top() < rThumb.top() - 1)
  {
      p->drawLine(rSlot.left(), rThumb.top() - 1, rSlot.left(), rSlot.top());
      p->setPen(g.light());
      p->drawLine(rSlot.right(), rSlot.top() + 1, rSlot.right(),
      rThumb.top() - 1);
  
      p->fillRect(rSlot.x() + 1, rSlot.y() + 1, rSlot.width() - 2,
      dTop - ds -1, QBrush(black));
  
  }
    }

    lPos = qwtMax(rSlot.bottom(), rThumb.bottom()) + 1;
    if (rThumb.bottom() < rBound.bottom())
    {
  p->fillRect(rBound.left(), rThumb.bottom() + 1,
        rSlot.left() - rBound.left(),
        rBound.bottom() - rThumb.bottom(), brBack);
  p->fillRect(rSlot.right() + 1, rThumb.bottom() + 1,
        rBound.right() - rSlot.right(),
        rBound.bottom() - rThumb.bottom(), brBack);
  if (lPos <= rBound.bottom())
     p->fillRect(rSlot.left(), lPos, ws, rBound.bottom() - lPos + 1, brBack);

  p->setPen(g.dark());
  if (rSlot.bottom() > rThumb.bottom())
  {
      p->drawLine(rSlot.left(), rThumb.bottom() + 1, rSlot.left(), rSlot.bottom());
      p->setPen(g.light());
      p->drawLine(rSlot.left() * 1, rSlot.bottom(), rSlot.right(), rSlot.bottom());
  }

  if (rSlot.bottom() > rThumb.bottom() + 1)
  {
      p->setPen(g.light());
      p->drawLine(rSlot.right(), rThumb.bottom() + 1, rSlot.right(),
      rSlot.bottom());
      p->fillRect(rSlot.left() + 1, rThumb.bottom() + 1,
      rSlot.width() - 2, rSlot.bottom() - rThumb.bottom() - 1,
      QBrush(black));
  }
    }

}

//------------------------------------------------------------
//.-
//.F  Slider::getValue
//  Determine the value corresponding to a specified
//  mouse location.
//
//.u  Syntax
//.f     double Slider::getValue(const QPoint &p)
//
//.u  Parameters
//.p  const QPoint &p --
//
//.u  Description
//  Called by SliderBase
//------------------------------------------------------------
double Slider::getValue( const QPoint &p)
{
    double rv;
    int pos;
    QRect r = d_sliderRect;

    r.setLeft(r.left() + d_bwTrough);
    r.setRight(r.right() - d_bwTrough);
    r.setTop(r.top() - d_bwTrough);
    r.setBottom(r.bottom() - d_bwTrough);

    if (d_orient == Horizontal)
    {
  
  if (r.width() <= d_thumbLength)
  {
      rv = 0.5 * (minValue() + maxValue());
  }
  else
  {
      pos = p.x() - r.x() - d_thumbHalf;
      rv  =  minValue() +
         rint( (maxValue() - minValue()) * double(pos)
        / double(r.width() - d_thumbLength)
        / step() ) * step();
  }
  
    }
    else
    {
  if (r.height() <= d_thumbLength)
  {
      rv = 0.5 * (minValue() + maxValue());
  }
  else
  {
      pos = p.y() - r.y() - d_thumbHalf;
      rv =  minValue() +
         rint( (maxValue() - minValue()) *
        (1.0 - double(pos)
         / double(r.height() - d_thumbLength))
        / step() ) * step();
  }
  
    }

    return(rv);
}


//------------------------------------------------------------
//.-
//.F  Slider::getScrollMode
//  Determine scrolling mode and direction
//
//.u  Syntax
//.f   void Slider::getScrollMode( const QPoint &p, int &scrollMode, int &direction )
//
//.u  Parameters
//.p  const QPoint &p -- point
//
//.u  Description
//  Called by SliderBase
//
//------------------------------------------------------------
void Slider::getScrollMode( QPoint &p, const Qt::ButtonState &button, int &scrollMode, int &direction )
{
    if(cursorHoming() && button == QMouseEvent::LeftButton)
    {
      if(d_sliderRect.contains(p))
      {
        scrollMode = ScrMouse;
        direction = 0;

        int mp = 0;
        QRect cr;
        QPoint cp;
        int ipos,dist1;
        double rpos;
        int lineDist;
  
        if(d_bwTrough > 0) 
          cr.setRect(d_sliderRect.x() + d_bwTrough,
                    d_sliderRect.y() + d_bwTrough,
                    d_sliderRect.width() - 2*d_bwTrough,
                    d_sliderRect.height() - 2*d_bwTrough);
        else 
          cr = d_sliderRect;
  
        rpos = (value()  - minValue()) / (maxValue() - minValue());
  
        lineDist = d_borderWidth - 1;
        if(lineDist < 1) lineDist = 1;
  
        if(d_orient == Horizontal)
        {
          dist1 = int(double(cr.width() - d_thumbLength) * rpos);
          ipos =  cr.x() + dist1;
          mp = ipos + d_thumbHalf;
        
          p.setX(mp);
          cp = mapToGlobal( QPoint(mp, p.y()) );
        }  
        else
        {
          dist1 = int(double(cr.height() - d_thumbLength) * (1.0 - rpos));
          ipos = cr.y() + dist1;
          mp = ipos + d_thumbHalf;
          p.setY(mp);
          cp = mapToGlobal( QPoint(p.x(), mp) );
        }  
        cursor().setPos(cp.x(), cp.y());
      }
    }
    else
    {
      int currentPos;
      if (d_orient == Horizontal)
       currentPos = p.x();
      else
       currentPos = p.y();
      
      if (d_sliderRect.contains(p))
      {
        if ((currentPos > markerPos - d_thumbHalf)  
            && (currentPos < markerPos + d_thumbHalf))
        {
          scrollMode = ScrMouse;
          direction = 0;
        }
        else
        {
          scrollMode = ScrPage;
          if (((currentPos > markerPos) && (d_orient == Horizontal))
              || ((currentPos <= markerPos) && (d_orient != Horizontal)))
            direction = 1;
          else
            direction = -1;
        }
      }
      else
      {
        scrollMode = ScrNone;
        direction = 0;
      }
    
    }
}

//------------------------------------------------------------
//.F  Slider::paintEvent
//  Qt paint event
//
//.u  Syntax
//.f  void Slider::paintEvent(QPaintEvent *e)
//------------------------------------------------------------

void Slider::paintEvent(QPaintEvent* /*e*/)
      {
      QPainter p;

      if (p.begin(this)) {
            if (d_scalePos != None) {
                  p.fillRect(this->rect(), this->backgroundColor());
                  d_scale.draw(&p);
                  }
            drawSlider(&p, d_sliderRect);
            }
      p.end();
      }

//------------------------------------------------------------
//.F  Slider::resizeEvent
//  Qt resize event
//
//.u  Parameters
//.p  QResizeEvent *e
//
//.u  Syntax
//.f  void Slider::resizeEvent(QResizeEvent *e)
//------------------------------------------------------------

void Slider::resizeEvent(QResizeEvent *e)
{

    d_resized = TRUE;
    QSize s = e->size();
    int sliderWidth = d_thumbWidth + 2 * d_bwTrough;

    // reposition slider
    if(d_orient == Horizontal)
    {
  switch(d_scalePos)
  {
  case Top:
  
      d_sliderRect.setRect(this->rect().x() + d_xMargin,
         this->rect().y() + s.height() - 1
         - d_yMargin - sliderWidth,
         s.width() - 2 * d_xMargin,
         sliderWidth);
      d_scale.setGeometry(d_sliderRect.x() + d_bwTrough + d_thumbHalf,
        d_sliderRect.y() - d_scaleDist,
        d_sliderRect.width() - d_thumbLength - 2*d_bwTrough,
        ScaleDraw::Top);
  
      break;
  
  case Bottom:
  
      d_sliderRect.setRect(this->rect().x() + d_xMargin,
         this->rect().y() + d_yMargin,
         s.width() - 2*d_xMargin,
         sliderWidth);
      d_scale.setGeometry(d_sliderRect.x() + d_bwTrough + d_thumbHalf,
        d_sliderRect.y() + d_sliderRect.height() +  d_scaleDist,
        d_sliderRect.width() - d_thumbLength - 2*d_bwTrough,
        ScaleDraw::Bottom);
  
      break;
  
  default:
      d_sliderRect.setRect(this->rect().x(), this->rect().x(),
         s.width(), s.height());
      break;
  }
    }
    else
    {
  switch(d_scalePos)
  {
  case Left:
      d_sliderRect.setRect(this->rect().x() + s.width()
         - sliderWidth - 1 - d_xMargin,
         this->rect().y() + d_yMargin,
         sliderWidth,
         s.height() - 2 * d_yMargin);
      d_scale.setGeometry(d_sliderRect.x() - d_scaleDist,
        d_sliderRect.y() + d_thumbHalf + d_bwTrough,
        s.height() - d_thumbLength - 2*d_bwTrough,
        ScaleDraw::Left);
  
      break;
  case Right:
      d_sliderRect.setRect(this->rect().x() + d_xMargin,
         this->rect().y() + d_yMargin,
         sliderWidth,
         s.height() - 2* d_yMargin);
      d_scale.setGeometry(this->rect().x() + d_sliderRect.width()
        + d_scaleDist,
        d_sliderRect.y() + d_thumbHalf + d_bwTrough,
        s.height() - d_thumbLength - 2*d_bwTrough,
        ScaleDraw::Right);
      break;
  default:
      d_sliderRect.setRect(this->rect().x(), this->rect().x(),
         s.width(), s.height());
      break;
  }
    }

}

//------------------------------------------------------------
//.-
//.F  Slider::valueChange
//  Notify change of value
//
//.u  Syntax
//.f  void Slider::valueChange()
//
//------------------------------------------------------------

void Slider::valueChange()
      {
      QPainter p;

      p.begin(this);
      drawSlider(&p, d_sliderRect);
      p.end();

      SliderBase::valueChange();
      }

//------------------------------------------------------------
//.-  
//.F  Slider::rangeChange
//  Notify change of range
//
//.u  Description
//
//.u  Syntax
//.f  void Slider::rangeChange()
//
//------------------------------------------------------------
void Slider::rangeChange()
{
    if (!hasUserScale())
       d_scale.setScale(minValue(), maxValue(), d_maxMajor, d_maxMinor);
    SliderBase::rangeChange();
    repaint(FALSE);
}

//------------------------------------------------------------
//
//.F  Slider::setMargins
//  Set distances between the widget's border and
//  internals.
//
//.u  Syntax
//.f  void Slider::setMargins(int hor, int vert)
//
//.u  Parameters
//.p  int hor, int vert -- Margins
//
//------------------------------------------------------------
void Slider::setMargins(int hor, int vert)
{
    d_xMargin = qwtMax(0, hor);
    d_yMargin = qwtMin(0, vert);
    resize(this->size());
}

//------------------------------------------------------------
//
//.F  Slider::sizeHint
//  Return a recommended size
//
//.u  Syntax
//.f  QSize Slider::sizeHint() const
//
//.u  Note
//  The return value of sizeHint() depends on the font and the
//  scale.
//------------------------------------------------------------

QSize Slider::sizeHint() const
      {
      QPainter p;
      int msWidth = 0, msHeight = 0;

      int w = 40;
      int h = 40;
      if (d_scalePos != None) {
            if (p.begin(this)) {
                  msWidth = d_scale.maxWidth(&p, FALSE);
                  msHeight = d_scale.maxHeight(&p);
                  }
            p.end();

            switch(d_orient) {
                  case Vertical:
                        w = 2*d_xMargin + d_thumbWidth + 2*d_bwTrough + msWidth + d_scaleDist + 2;
                        break;
                  case Horizontal:
                        h = 2*d_yMargin + d_thumbWidth + 2*d_bwTrough + msHeight + d_scaleDist;
                        break;
                  }
            }
      else {      // no scale
            switch(d_orient) {
                  case Vertical:
                        w = 16 + 2 * d_bwTrough;
                        break;
                  case Horizontal:
                        h = 16 + 2 * d_bwTrough;
                        break;
                  }
            }
      return QSize(w, h);
      }

//---------------------------------------------------------
//   setOrientation
//---------------------------------------------------------

void Slider::setOrientation(Orientation o)
      {
      d_orient = o;
      ScaleDraw::OrientationX so = ScaleDraw::Bottom;
      switch(d_orient) {
            case Vertical:
                  if (d_scalePos == Right)
                        so = ScaleDraw::Right;
                  else
                        so = ScaleDraw::Left;
                  break;
            case Horizontal:
                  if (d_scalePos == Bottom)
                        so = ScaleDraw::Bottom;
                  else
                        so = ScaleDraw::Top;
                  break;
            }

      d_scale.setGeometry(0, 0, 40, so);
      if (d_orient == Vertical)
            setMinimumSize(10,20);
      else
            setMinimumSize(20,10);
      QRect r = geometry();
      setGeometry(r.x(), r.y(), r.height(), r.width());
      update();
      }

Qt::Orientation Slider::orientation() const
      {
      return d_orient;
      }

double Slider::lineStep() const
      {
      return 1.0;
      }

double Slider::pageStep() const
      {
      return 1.0;
      }

void Slider::setLineStep(double)
      {
      }

void Slider::setPageStep(double)
      {
      }

