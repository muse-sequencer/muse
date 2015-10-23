//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: posedit.cpp,v 1.3.2.2 2008/05/21 00:28:54 terminator356 Exp $
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

#include <values.h>

#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QString>
#include <QStyle>
#include <QTimerEvent>

#include "posedit.h"
#include "sig.h"
#include "spinbox.h"

extern int MusEGlobal::mtcType;

namespace MusEGui {

//---------------------------------------------------------
//   QNumberSection
//---------------------------------------------------------

class QNumberSection
      {
      int selstart;
      int selend;

   public:
      QNumberSection(int selStart = 0, int selEnd = 0)
         : selstart(selStart), selend(selEnd )  {}
      int selectionStart() const    { return selstart; }
      void setSelectionStart(int s) { selstart = s; }
      int selectionEnd() const      { return selend; }
      void setSelectionEnd( int s ) { selend = s; }
      int width() const             { return selend - selstart; }
      };

//---------------------------------------------------------
//   PosEditor
//---------------------------------------------------------

class PosEditor : public QLineEdit
      {
      PosEdit* cw;
      bool frm;
      QPixmap *pm;
      int focusSec;
      QList<QNumberSection> sections;
      QString sep;
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
      PosEditor(PosEdit* Q_PARENT, const char * Q_NAME );
      ~PosEditor();

      void setControlWidget(PosEdit * widget);
      PosEdit* controlWidget() const;

      void setSeparator(const QString& s) { sep = s; }
      QString separator() const           { return sep; }
      int focusSection()  const           { return focusSec; }

      bool setFocusSection(int s);
      void appendSection(const QNumberSection& sec);
      void clearSections();
      void setSectionSelection(int sec, int selstart, int selend);
      };

//---------------------------------------------------------
//   section
//---------------------------------------------------------

int PosEditor::section(const QPoint& pt)
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
                  QString s = sep;
                  p.drawText(x, y, w, h, Qt::AlignVCenter|Qt::AlignLeft, s, -1, &bb);
                  x = bb.x() + bb.width();
                  }
            }
      return -1;
      }

//---------------------------------------------------------
//   PosEditor
//---------------------------------------------------------

PosEditor::PosEditor(PosEdit* parent, const char* name)
   : QLineEdit(parent), sep(".")
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
//   ~PosEditor
//---------------------------------------------------------

PosEditor::~PosEditor()
      {
      delete pm;
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void PosEditor::init()
      {
      setBackgroundMode(Qt::PaletteBase);
      setFocusSection(-1);
      setKeyCompression(true);
      setFocusPolicy(Qt::WheelFocus);
      }

//---------------------------------------------------------
//   event
//---------------------------------------------------------

bool PosEditor::event(QEvent *e)
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
      return QWidget::event(e);
      }

void PosEditor::resizeEvent(QResizeEvent *e)
      {
      pm->resize(e->size());
      QWidget::resizeEvent(e);
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void PosEditor::paintEvent(QPaintEvent *)
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
                  QString s = sep;
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

void PosEditor::mousePressEvent(QMouseEvent *e)
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

void PosEditor::keyPressEvent(QKeyEvent *e)
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
            case Qt::Key_Return:
                  cw->enterPressed();
            default:
                  QString txt = e->text();
                  if (!txt.isEmpty() && !sep.isEmpty() && txt[0] == sep[0]) {
                        // do the same thing as KEY_RIGHT when the user presses the separator key
                        if (unsigned(focusSec) < sections.count()) {
                              if (cw->setFocusSection(focusSec+1))
                                    repaint(rect(), false);
                              }
                        }
                  int num = txt[0].digitValue();
                  if (num != -1) {
                        cw->addNumber(focusSec, num);
                        }
            }
      }

void PosEditor::appendSection(const QNumberSection& sec)
      {
      sections.append(sec);
      }
void PosEditor::clearSections()
      {
      sections.clear();
      }

//---------------------------------------------------------
//   setSectionSelection
//---------------------------------------------------------

void PosEditor::setSectionSelection(int secNo, int selstart, int selend)
      {
      if (secNo < 0 || secNo > (int)sections.count())
            return;
      sections[secNo].setSelectionStart(selstart);
      sections[secNo].setSelectionEnd(selend);
      }

//---------------------------------------------------------
//   setFocusSection
//---------------------------------------------------------

bool PosEditor::setFocusSection(int idx)
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
//   PosEdit
//---------------------------------------------------------

PosEdit::PosEdit(QWidget* parent, const char* name)
   : QWidget(parent)
      {
      setObjectName(name);
      init();
      updateButtons();
      }

PosEdit::PosEdit(const Pos& time, QWidget* parent, const char* name)
    : QWidget(parent, name)
      {
      init();
      setValue(time);
      updateButtons();
      }

PosEdit::~PosEdit()
      {
      }

//---------------------------------------------------------
//   init
//---------------------------------------------------------

void PosEdit::init()
      {
      ed       = new PosEditor(this, "pos editor");
      controls = new SpinBox(this);
      controls->setEditor(ed);
      setFocusProxy(ed);
      connect(controls, SIGNAL(stepUpPressed()), SLOT(stepUp()));
      connect(controls, SIGNAL(stepDownPressed()), SLOT(stepDown()));
      connect(this, SIGNAL(valueChanged(const Pos&)),SLOT(updateButtons()));

      overwrite = false;
      timerId   = 0;
      typing    = false;
      min       = Pos(0);
      max       = Pos(MAX_TICK);
      changed   = false;
      adv       = false;


      static Section s_midiSections[3] = {  // measure, beat, tick
            { 0, 4, 1, 0 },
            { 5, 2, 1, 0 },
            { 8, 3, 0, 0 }
            };
      static Section s_smpteSections[4] = {  // minute second frame subframe
            {  0, 3, 0, 0 },
            {  4, 2, 0, 0 },
            {  7, 2, 0, 0 },
            { 10, 2, 0, 0 }
            };
      memcpy(midiSections, s_midiSections, sizeof(s_midiSections));
      memcpy(smpteSections, s_smpteSections, sizeof(s_smpteSections));

      _smpte     = false;  // show position in smpte format
      sec       = midiSections;
      setSections();
      setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
      }

//---------------------------------------------------------
//   setSetions
//---------------------------------------------------------

void PosEdit::setSections()
      {
      ed->clearSections();
      ed->appendSection(QNumberSection(0,0));
      ed->appendSection(QNumberSection(0,0));
      ed->appendSection(QNumberSection(0,0));
      if (_smpte) {
            ed->appendSection(QNumberSection(0,0));
            ed->setSeparator(QString(":"));
            }
      else {
            ed->setSeparator(QString("."));
            }
      }

//---------------------------------------------------------
//   smpte
//---------------------------------------------------------

bool PosEdit::smpte() const
      {
      return _smpte;
      }

//---------------------------------------------------------
//   setSmpte
//---------------------------------------------------------

void PosEdit::setSmpte(bool f)
      {
      _smpte = f;
      sec   = f ? smpteSections : midiSections;
      setSections();
      ed->repaint(ed->rect(), false);
      }

//---------------------------------------------------------
//   minValue
//---------------------------------------------------------

Pos PosEdit::minValue() const
      {
      return min;
      }

//---------------------------------------------------------
//   maxValue
//---------------------------------------------------------

Pos PosEdit::maxValue() const
      {
      return max;
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void PosEdit::setRange(const Pos& _min, const Pos& _max)
      {
      if (min.isValid())
            min = _min;
      if (max.isValid())
            max = _max;
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void PosEdit::setValue(const Pos& time)
      {
      if (time > maxValue() || time < minValue())
            return;
      if (_smpte)
            time.msf(&(sec[0].val), &(sec[1].val), &(sec[2].val),
               &(sec[3].val));
      else
            time.mbt(&(sec[0].val), &(sec[1].val), &(sec[2].val));
      changed = false;
      
      updateButtons();
      ed->repaint(ed->rect(), false);
      }

void PosEdit::setValue(const QString& s)
      {
      Pos time(s);
      setValue(time);
      }

void PosEdit::setValue(int t)
      {
      Pos time(t);
      setValue(time);
      }

Pos PosEdit::pos() const
      {
      if (_smpte) {
            if (Pos::isValid(sec[0].val, sec[1].val, sec[2].val, sec[3].val))
                  return Pos(sec[0].val, sec[1].val, sec[2].val, sec[3].val);
            }
      else {
            if (Pos::isValid(sec[0].val, sec[1].val, sec[2].val))
                  return Pos(sec[0].val, sec[1].val, sec[2].val);
            }
      return Pos();
      }

void PosEdit::setSeparator(const QString& s)
      {
      ed->setSeparator(s);
      }

QString PosEdit::separator() const
      {
      return ed->separator();
      }

bool PosEdit::event(QEvent *e)
      {
      if (e->type() == QEvent::FocusOut) {
            typing = false;
            if (changed) {
                  emit valueChanged(pos() );
                  changed = false;
                  }
            }
      return QWidget::event(e);
      }

void PosEdit::timerEvent(QTimerEvent *)
      {
      overwrite = true;
      }

//---------------------------------------------------------
//   stepUp
//---------------------------------------------------------

void PosEdit::stepUp()
      {
      int secNo = ed->focusSection();
      bool accepted = false;

      if (!outOfRange(secNo, sec[secNo].val+1)) {
            accepted = true;
            setSec(secNo, sec[secNo].val+1);
            }
      if (accepted) {
            changed = true;
            Pos p = pos();
            emit valueChanged(p);
            }
      ed->repaint(ed->rect(), false);
      }

//---------------------------------------------------------
//   stepDown
//---------------------------------------------------------

void PosEdit::stepDown()
      {
      int secNo = ed->focusSection();
      bool accepted = false;
      if (!outOfRange(secNo, sec[secNo].val-1)) {
            accepted = true;
            setSec(secNo, sec[secNo].val-1);
            }
      if (accepted) {
            changed = true;
            emit valueChanged(pos());
            }
      ed->repaint(ed->rect(), false);
      }

//---------------------------------------------------------
//   sectionFormattedText
//    Returns the formatted number for section sec.
//---------------------------------------------------------

QString PosEdit::sectionFormattedText(int secNo)
      {
      QString txt = sectionText(secNo);
      int so      = sec[secNo].offset;
      int len     = sec[secNo].len;
      int eo      = so + len;

      if (typing && secNo == ed->focusSection())
            ed->setSectionSelection(secNo, eo - txt.length(), eo);
      else
            ed->setSectionSelection(secNo, so, eo);
      txt = txt.rightJustify(len, '0');
      return txt;
      }

//---------------------------------------------------------
//   setFocusSection
//---------------------------------------------------------

bool PosEdit::setFocusSection(int s)
      {
      if (s != ed->focusSection()) {
            killTimer(timerId);
            overwrite = true;
            typing    = false;
            int so = sec[s].offset;
            int eo = so + sec[s].len;
            ed->setSectionSelection(s, so, eo);
            if (changed) {
                  emit valueChanged(pos());
                  changed = false;
                  }
            }
      return ed->setFocusSection(s);
      }

//---------------------------------------------------------
//   setSec
//---------------------------------------------------------

void PosEdit::setSec(int secNo, int val)
      {
      if (val < 0)
            val = 0;
      if (_smpte) {
            switch(secNo) {
                  case 0:
                        break;
                  case 1:
                        if (val > 59)
                              val = 59;
                        break;
                  case 2:
                        switch(MusEGlobal::mtcType) {
                              case 0:     // 24 frames sec
                                    if (val > 23)
                                          val = 23;
                                    break;
                              case 1:
                                    if (val > 24)
                                          val = 24;
                                    break;
                              case 2:     // 30 drop frame
                              case 3:     // 30 non drop frame
                                    if (val > 29)
                                          val = 29;
                                    break;
                              }
                        break;
                  case 3:
                        if (val > 99)
                              val = 99;
                  }
            }
      else {
            switch(secNo) {
                  case 0:
                        break;
                  case 1:
                        {
                        int z, n;
                        int tick = sigmap.bar2tick(sec[0].val, val, sec[2].val);
                        sigmap.timesig(tick, z, n);
                        if (val >= n)
                              val = n-1;
                        }
                        break;
                  case 2:
                        {
                        int tick = sigmap.bar2tick(sec[0].val, sec[1].val, val);
                        int tb = sigmap.ticksBeat(tick);
                        if (val >= tb)
                              val = tb-1;
                        }
                        break;
                  }
            }
      sec[secNo].val = val;
      }

//---------------------------------------------------------
//   sectionText
//    Returns the text of section \a sec.
//---------------------------------------------------------

QString PosEdit::sectionText(int secNo)
      {
      return QString::number(sec[secNo].val + sec[secNo].voff);
      }

//---------------------------------------------------------
//   outOfRange
//    return true if out of range
//---------------------------------------------------------

bool PosEdit::outOfRange(int secNo, int val) const
      {
      if (val < 0)
            return true;
      int limit = MAXINT;
      if (_smpte) {
            switch(secNo) {
                  case 0:
                        break;
                  case 1:
                        limit = 59;
                        break;
                  case 2:
                        switch(MusEGlobal::mtcType) {
                              case 0:     // 24 frames sec
                                    limit = 23;
                                    break;
                              case 1:
                                    limit = 24;
                                    break;
                              case 2:     // 30 drop frame
                              case 3:     // 30 non drop frame
                                    limit = 29;
                                    break;
                              }
                        break;
                  case 3:
                        limit = 99;
                        break;
                  }
            }
      else {
            switch(secNo) {
                  case 0:
                        break;
                  case 1:
                        {
                        int z;
                        int tick = sigmap.bar2tick(sec[0].val, val, sec[2].val);
                        sigmap.timesig(tick, z, limit);
                        limit -= 1;
                        }
                        break;
                  case 2:
                        int tick = sigmap.bar2tick(sec[0].val, sec[1].val, val);
                        limit = sigmap.ticksBeat(tick) - 1;
                        break;
                  }
            }
      return val > limit;
      }

//---------------------------------------------------------
//   addNumber
//---------------------------------------------------------

void PosEdit::addNumber(int secNo, int num)
      {
      if (secNo == -1)
            return;
      killTimer(timerId);
      bool accepted  = false;
      typing         = true;
      int voff       = sec[secNo].voff;

      QString txt = sectionText(secNo);

      if ((unsigned) txt.length() == sec[secNo].len) {
            if (!outOfRange(secNo, num - voff)) {
                  accepted = true;
                  sec[secNo].val = num - voff;
                  }
            }
      else {
            txt += QString::number(num);
            int temp = txt.toInt() - voff;
            if (outOfRange(secNo, temp))
                  txt = sectionText(secNo);
            else {
                  accepted = true;
                  sec[secNo].val = temp;
                  }
            if (adv && ((unsigned) txt.length() == sec[secNo].len)) {
                  setFocusSection(ed->focusSection() + 1);
                  }
            }
      changed = accepted;
      if (accepted)
            emit valueChanged(pos());
      timerId = startTimer(qApp->doubleClickInterval()*4);
      ed->repaint(ed->rect(), false);
      }

//---------------------------------------------------------
//   removeLastNumber
//---------------------------------------------------------

void PosEdit::removeLastNumber(int secNo)
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

void PosEdit::resizeEvent(QResizeEvent* ev)
      {
      QWidget::resizeEvent(ev);
      controls->resize(width(), height());
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize PosEdit::sizeHint() const
      {
      QFontMetrics fm(font());
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth,0, this); // ddskrjo 0
      int h  = fm.height() + fw * 2;
      int w  = 4 + controls->arrowWidth() + fw * 4;
      if (_smpte)
            w += fm.width('9') * 10 + fm.width(ed->separator()) * 3;
      else
            w += fm.width('9') * 10 + fm.width(ed->separator()) * 2;
      return QSize(w, h).expandedTo(QApplication::globalStrut());
      }

//---------------------------------------------------------
//   updateButtons
//---------------------------------------------------------

void PosEdit::updateButtons()
      {
      bool upEnabled   = isEnabled() && (pos() < maxValue());
      bool downEnabled = isEnabled() && (pos() > minValue());

      //printf("PosEdit::updateButtons smpte:%d upEnabled:%d downEnabled:%d\n", smpte(), upEnabled, downEnabled);

      controls->setStepEnabled(upEnabled, downEnabled);
      }

//---------------------------------------------------------
//   enterPressed
//---------------------------------------------------------
void PosEdit::enterPressed()
      {
      emit returnPressed();
      }

//---------------------------------------------------------
//   setEnabled
//---------------------------------------------------------
void PosEdit::setEnabled(bool v) 
{
  QWidget::setEnabled(v);
  updateButtons();
}

} // namespace MusEGui
