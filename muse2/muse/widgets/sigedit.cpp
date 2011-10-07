//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sigedit.cpp,v 1.1.1.1.2.1 2004/12/28 23:23:51 lunar_shuttle Exp $
//  (C) Copyright 2001 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <stdio.h>
#include <values.h>

#include <QEvent>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QString>
#include <QStyle>
#include <QTimerEvent>

///#include "sig.h"
#include "al/sig.h"
#include "sigedit.h"
#include "spinbox.h"

extern int MusEGlobal::mtcType;

namespace MusEGui {

bool Sig::isValid() const
{
  if((z < 1) || (z > 63))
    return false;
            
  switch(n) 
  {
    case  1:
    case  2:
    case  3:
    case  4:
    case  8:
    case 16:
    case 32:
    case 64:
    case 128:
      return true;
    default:
      return false;
  }                
}


//---------------------------------------------------------
//   NumberSection
//---------------------------------------------------------

class NumberSection
      {
      int selstart;
      int selend;

   public:
      NumberSection(int selStart = 0, int selEnd = 0)
         : selstart(selStart), selend(selEnd )  {}
      int selectionStart() const    { return selstart; }
      void setSelectionStart(int s) { selstart = s; }
      int selectionEnd() const      { return selend; }
      void setSelectionEnd( int s ) { selend = s; }
      int width() const             { return selend - selstart; }
      };

//---------------------------------------------------------
//   SigEditor
//---------------------------------------------------------

class SigEditor : public QLineEdit
      {
      SigEdit* cw;
      bool frm;
      QPixmap *pm;
      int focusSec;
      QList<NumberSection> sections;
      int offset;

      int section(const QPoint&);

   protected:
      void init();
      virtual bool event(QEvent *e);
      virtual void resizeEvent(QResizeEvent*);
      virtual void paintEvent(QPaintEvent*);
      virtual void mousePressEvent(QMouseEvent *e);
      virtual void keyPressEvent(QKeyEvent * event );
      void applyFocusSelection() {}

   public:
      SigEditor(SigEdit* parent, const char* name);
      ~SigEditor();

      void setControlWidget(SigEdit * widget);
      SigEdit* controlWidget() const;

      int focusSection()  const   { return focusSec; }

      bool setFocusSection(int s);
      void appendSection(const NumberSection& sec);
      void clearSections();
      void setSectionSelection(int sec, int selstart, int selend);
      };

//---------------------------------------------------------
//   section
//---------------------------------------------------------

int SigEditor::section(const QPoint& pt)
      {
      if (pm->isNull())
            return -1;
      QPainter p(pm);
      int fw = frm ? style()->pixelMetric(QStyle::PM_DefaultFrameWidth) : 0;
      int x = 2 + fw;
      int y = 0;
      int w = width();
      int h = height();
      for (int i = 0; i < sections.count(); ++i) {
            QString s = cw->sectionFormattedText(i);
            QRect bb = p.boundingRect(x, y, w, h, Qt::AlignVCenter|Qt::AlignLeft, s);
            int nx = bb.x() + bb.width();
            if (pt.x() >= x && pt.x() < nx)
                  return i;
            x = nx;
            if (i < sections.count()-1) {
                  QString s("/");
                  p.drawText(x, y, w, h, Qt::AlignVCenter|Qt::AlignLeft, s, -1, &bb);
                  x = bb.x() + bb.width();
                  }
            }
      return -1;
      }

//---------------------------------------------------------
//   SigEditor
//---------------------------------------------------------

SigEditor::SigEditor(SigEdit* parent, const char* name)
   : QLineEdit(parent)
      {
      setObjectName(name);
      cw       = parent;
      frm      = true;
      focusSec = 0;
      pm       = new QPixmap;
      offset   = 0;
      init();
      }

//---------------------------------------------------------
//   ~SigEditor
//---------------------------------------------------------

SigEditor::~SigEditor()
      {
      delete pm;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void SigEditor::init()
      {
      setBackgroundMode(Qt::PaletteBase);
      setFocusSection(-1);
      setKeyCompression(true);
      setFocusPolicy(Qt::WheelFocus);
      }

//---------------------------------------------------------
//   event
//---------------------------------------------------------

bool SigEditor::event(QEvent *e)
      {
      if (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut) {
            repaint( rect(), false);
            }
      else if (e->type() == QEvent::ShortcutOverride) {
            QKeyEvent* ke = (QKeyEvent*) e;
            switch (ke->key()) {
                  case Qt::Key_Delete:
                  case Qt::Key_Backspace:
                  case Qt::Key_Up:
                  case Qt::Key_Down:
                  case Qt::Key_Left:
                  case Qt::Key_Right:
                        ke->accept();
                  default:
                        break;
                  }
            }
      return QLineEdit::event(e);
      }

void SigEditor::resizeEvent(QResizeEvent *e)
      {
      pm->resize(e->size());
      QLineEdit::resizeEvent(e);
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void SigEditor::paintEvent(QPaintEvent *)
      {
      if (pm->isNull())
            return;

      const QColorGroup & cg = colorGroup();
      QPainter p(pm);
      p.setPen(colorGroup().text());
      QBrush bg = cg.brush(QColorGroup::Base);

      int fw = frm ? style()->pixelMetric(QStyle::PM_DefaultFrameWidth) : 0;
      int x = 2 + fw;
      int y = 0;
      int w = width();
      int h = height();
      p.fillRect(0, 0, w, h, bg);

      for (int i = 0; i < sections.count(); ++i) {
            QRect bb;
            QString s = cw->sectionFormattedText(i);

            if (hasFocus() && (int(i) == focusSec)) {
                  QBrush bg = cg.brush(QColorGroup::Highlight);
                  QRect r = p.boundingRect(x, y, w, h, Qt::AlignVCenter|Qt::AlignLeft, s, -1);
                  p.setPen(colorGroup().highlightedText());
                  p.fillRect(r, bg);
                  }
            else
                  p.setPen(colorGroup().text());
            p.drawText(x, y, w, h, Qt::AlignVCenter|Qt::AlignLeft, s, -1, &bb);
            x = bb.x() + bb.width();
            if (i < sections.count()-1) {
                  QString s("/");
                  p.drawText(x, y, w, h, Qt::AlignVCenter|Qt::AlignLeft, s, -1, &bb);
                  x = bb.x() + bb.width();
                  }
            }
      p.end();
      bitBlt(this, 0, 0, pm);
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void SigEditor::mousePressEvent(QMouseEvent *e)
      {
      QPoint p(e->pos().x(), 0);
      int sec = section(p);
      if (sec != -1) {
            cw->setFocusSection(sec);
            repaint(rect(), false);
            }
      }

//---------------------------------------------------------
//   keyPressEvent
//---------------------------------------------------------

void SigEditor::keyPressEvent(QKeyEvent * e )
      {
      switch (e->key()) {
            case Qt::Key_Right:
                  if (unsigned(focusSec) <= sections.count()) {
                        if (cw->setFocusSection(focusSec+1))
                              repaint(rect(), false);
                        }
            case Qt::Key_Left:
                  if (focusSec > 0 ) {
                        if (cw->setFocusSection(focusSec-1))
                              repaint(rect(), false);
                        }
            case Qt::Key_Up:
                  cw->stepUp();
            case Qt::Key_Down:
                  cw->stepDown();
            case Qt::Key_Backspace:
            case Qt::Key_Delete:
                  cw->removeLastNumber(focusSec);
            case Qt::Key_Enter:
            case Qt::Key_Return:
                  cw->enterPressed();
            default:
                  QString txt = e->text();
                  if (!txt.isEmpty() && txt[0] == '/') {
                        // do the same thing as KEY_RIGHT when the user presses the separator key
                        if (focusSec < (signed)(sections.count())) {
                              if (cw->setFocusSection(focusSec+1))
                                    repaint(rect(), false);
                              }
                        }
                  int num = txt[0].digitValue();
                  
                  //printf("SigEditor::keyPressEvent num:%d\n", num);
                  
                  if (num != -1) {
                        cw->addNumber(focusSec, num);
                        }
            }
      }

void SigEditor::appendSection(const NumberSection& sec)
      {
      sections.append(sec);
      }
void SigEditor::clearSections()
      {
      sections.clear();
      }

//---------------------------------------------------------
//   setSectionSelection
//---------------------------------------------------------

void SigEditor::setSectionSelection(int secNo, int selstart, int selend)
      {
      if (secNo < 0 || secNo > (int)sections.count())
            return;
      sections[secNo].setSelectionStart(selstart);
      sections[secNo].setSelectionEnd(selend);
      }

//---------------------------------------------------------
//   setFocusSection
//---------------------------------------------------------

bool SigEditor::setFocusSection(int idx)
      {
      if (idx > (int)sections.count()-1 || idx < 0)
            return false;
      if (idx != focusSec) {
            focusSec = idx;
            applyFocusSelection();
            return true;
            }
      return false;
      }

//---------------------------------------------------------
//   SigEdit
//---------------------------------------------------------

SigEdit::SigEdit(QWidget* parent, const char* name)
   : QWidget(parent)
      {
      setObjectName(name);
      init();
      updateButtons();
      }

SigEdit::~SigEdit()
      {
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void SigEdit::init()
      {
      ed       = new SigEditor(this, "pos editor");
      controls = new SpinBox(this);
      controls->setEditor(ed);
      setFocusProxy(ed);
      connect(controls, SIGNAL(stepUpPressed()), SLOT(stepUp()));
      connect(controls, SIGNAL(stepDownPressed()), SLOT(stepDown()));
      connect(this, SIGNAL(valueChanged(int,int)),SLOT(updateButtons()));

      overwrite = false;
      timerId   = 0;
      typing    = false;
      changed   = false;
      adv       = false;

      sec[0].offset = 0;
      sec[0].len    = 2;
      sec[0].val    = 4;
      sec[0].voff   = 0;
      sec[1].offset = 3;
      sec[1].len    = 3;
      sec[1].val    = 4;
      sec[1].voff   = 0;
      ed->clearSections();
      ed->appendSection(NumberSection(0,0));
      ed->appendSection(NumberSection(0,0));
      setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void SigEdit::setValue(const Sig& sig)
      {
      sec[0].val = sig.z;
      sec[1].val = sig.n;
      changed = false;
      ed->repaint(ed->rect(), false);
      }

void SigEdit::setValue(const QString& s)
      {
      int z, n;
      sscanf(s.toLatin1(), "%d/%d", &z, &n);
      Sig sig(z, n);
      setValue(sig);
      }

Sig SigEdit::sig() const
      {
      Sig sig(sec[0].val, sec[1].val);
      return sig;
      }

bool SigEdit::event(QEvent *e)
      {
      if (e->type() == QEvent::FocusOut) {
            typing = false;
            if (changed) {
                  emit valueChanged(sig().z, sig().n);
                  changed = false;
                  }
            }
      return QWidget::event(e);
      }

void SigEdit::timerEvent(QTimerEvent *)
      {
      overwrite = true;
      }

//---------------------------------------------------------
//   stepUp
//---------------------------------------------------------

void SigEdit::stepUp()
      {
      bool accepted = false;
      int secNo = ed->focusSection();
      int val = sec[secNo].val;
      if (secNo == 0) {
            if (val < 63) {
                  ++val;
                  accepted = true;
                  }
            }
      else {
            accepted = true;
            switch(val) {
                  case  1:  val = 2;   break;
                  case  2:  val = 3;   break;
                  case  3:  val = 4;   break;
                  case  4:  val = 8;   break;
                  case  8:  val = 16;  break;
                  case 16:  val = 32;  break;
                  case 32:  val = 64;  break;
                  case 64:  val = 128; break;
                  case 128: accepted = false; break;
                  }
            }
      if (accepted) {
            setSec(secNo, val);
            changed = true;
            emit valueChanged(sec[0].val, sec[1].val);
            }
      ed->repaint(ed->rect(), false);
      }

//---------------------------------------------------------
//   stepDown
//---------------------------------------------------------

void SigEdit::stepDown()
      {
      bool accepted = false;
      int secNo = ed->focusSection();
      int val = sec[secNo].val;
      if (secNo == 0) {
            if (val > 1) {
                  --val;
                  accepted = true;
                  }
            }
      else {
            accepted = true;
            switch(val) {
                  case  1:  accepted = false; break;
                  case  2:  val = 1;   break;
                  case  3:  val = 2;   break;
                  case  4:  val = 3;   break;
                  case  8:  val = 4;  break;
                  case 16:  val = 8;  break;
                  case 32:  val = 16;  break;
                  case 64:  val = 32; break;
                  case 128: val = 64; break;
                  }
            }
      if (accepted) {
            setSec(secNo, val);
            changed = true;
            emit valueChanged(sec[0].val, sec[1].val);
            }
      ed->repaint(ed->rect(), false);
      }

//---------------------------------------------------------
//   sectionFormattedText
//    Returns the formatted number for section sec.
//---------------------------------------------------------

QString SigEdit::sectionFormattedText(int secNo)
      {
      QString txt = sectionText(secNo);

      int so  = sec[secNo].offset;
      int len = sec[secNo].len;
      int eo  = so + len;

      if (typing && secNo == ed->focusSection())
            ed->setSectionSelection(secNo, eo - txt.length(), eo);
      else
            ed->setSectionSelection(secNo, so, eo);
      if (secNo == 0)
            txt = txt.rightJustify(len, ' ');
//      else
//            txt = txt.leftJustify(len, ' ');
      return txt;
      }

//---------------------------------------------------------
//   setFocusSection
//---------------------------------------------------------

bool SigEdit::setFocusSection(int s)
      {
      if (s != ed->focusSection()) {
            killTimer(timerId);
            overwrite = true;
            typing    = false;
            int so = sec[s].offset;
            int eo = so + sec[s].len;
            ed->setSectionSelection(s, so, eo);
            if (changed) {
                  emit valueChanged(sig().z, sig().n);
                  changed = false;
                  }
            }
      return ed->setFocusSection(s);
      }

//---------------------------------------------------------
//   setSec
//---------------------------------------------------------

void SigEdit::setSec(int secNo, int val)
      {
      sec[secNo].val = val;
      }

//---------------------------------------------------------
//   sectionText
//    Returns the text of section \a sec.
//---------------------------------------------------------

QString SigEdit::sectionText(int secNo)
      {
      return QString::number(sec[secNo].val + sec[secNo].voff);
      }

//---------------------------------------------------------
//   outOfRange
//    return true if out of range
//---------------------------------------------------------

bool SigEdit::outOfRange(int secNo, int val) const
      {
      if (secNo == 0)
            return ((val < 1) || (val > 63));
      switch (val) {
            case  1:
            case  2:
            case  3:
            case  4:
            case  8:
            case 16:
            case 32:
            case 64:
            case 128:
                  // Changed p3.3.43
                  //return true;
                  return false;
            default:
                  // Changed p3.3.43
                  //return false;
                  return true;
            }
      }

//---------------------------------------------------------
//   addNumber
//---------------------------------------------------------

void SigEdit::addNumber(int secNo, int num)
      {
      if (secNo == -1)
            return;
      killTimer(timerId);
      bool accepted  = false;
      typing         = true;
      int voff       = sec[secNo].voff;

      QString txt = sectionText(secNo);

      //printf("SigEdit::addNumber secNo:%d num:%d voff:%d txt:%s\n", secNo, num, voff, txt.toLatin1());
      
      if ((unsigned) txt.length() == sec[secNo].len) {
            //printf("SigEdit::addNumber txt.length() == sec[secNo].len (%d)\n", sec[secNo].len);
      
            if (!outOfRange(secNo, num - voff)) {
                  //printf("SigEdit::addNumber accepted\n");
                  
                  accepted = true;
                  sec[secNo].val = num - voff;
                  }
            }
      else {
            //printf("SigEdit::addNumber txt.length() != sec[secNo].len (%d)\n", sec[secNo].len);
      
            txt += QString::number(num);
            int temp = txt.toInt() - voff;
            if (outOfRange(secNo, temp))
            {
                  //printf("SigEdit::addNumber not accepted secNo:%d txt:%s temp:%d\n", secNo, txt.toLatin1(), temp);
                  
                  txt = sectionText(secNo);
            }
            else {
                  //printf("SigEdit::addNumber accepted\n");
                  
                  accepted = true;
                  sec[secNo].val = temp;
                  }
            if (adv && ((unsigned) txt.length() == sec[secNo].len)) {
                  setFocusSection(ed->focusSection() + 1);
                  }
            }
      changed = accepted;
      if (accepted)
            emit valueChanged(sig().z, sig().n);
      timerId = startTimer(qApp->doubleClickInterval()*4);
      ed->repaint(ed->rect(), false);
      }

//---------------------------------------------------------
//   removeLastNumber
//---------------------------------------------------------

void SigEdit::removeLastNumber(int secNo)
      {
      if (secNo == -1)
	      return;
      QString txt = QString::number(sec[secNo].val);
      txt = txt.mid(0, txt.length() - 1);
      sec[secNo].val = txt.toInt() - sec[secNo].voff;
      ed->repaint(ed->rect(), false);
      }

//---------------------------------------------------------
//   resizeEvent
//---------------------------------------------------------

void SigEdit::resizeEvent(QResizeEvent* ev)
      {
        QWidget::resizeEvent(ev);
        controls->resize(width(), height());
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize SigEdit::sizeHint() const
      {
      QFontMetrics fm(font());
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this); // ddskrjo
      int h  = fm.height() + fw * 2;

      int w  = 2 + controls->arrowWidth() + fw * 4;
      w     += fm.width('9') * 5 + fm.width('/');
      return QSize(w, h).expandedTo(QApplication::globalStrut());
      }

//---------------------------------------------------------
//   updateButtons
//---------------------------------------------------------

void SigEdit::updateButtons()
      {
      int secNo = ed->focusSection();
      int val   = sec[secNo].val;

      bool upEnabled;
      bool downEnabled;

      if (secNo == 0) {
            upEnabled   = val < 63;
            downEnabled = val > 1;
            }
      else {
            upEnabled   = true;
            downEnabled = true;
            switch (val) {
                  case  1: downEnabled = false; break;
                  case 128: upEnabled = false; break;
                  }
            }
      controls->setStepEnabled(isEnabled() & upEnabled, isEnabled() & downEnabled);
      }

//---------------------------------------------------------
//   enterPressed
//! emit returnPressed
//---------------------------------------------------------
void SigEdit::enterPressed()
      {
      emit returnPressed();
      }

} // namespace MusEGui
