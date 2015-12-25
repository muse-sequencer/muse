//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: strip.cpp,v 1.6.2.5 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

#include <QToolButton>
#include <QLabel>
#include <QLayout>
#include <QPalette>
#include <QColor>
#include <QVBoxLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QMenu>

#include "globals.h"
#include "gconfig.h"
#include "app.h"
#include "audio.h"
#include "song.h"
#include "track.h"
#include "strip.h"
#include "meter.h"
#include "utils.h"
#include "icons.h"
#include "undo.h"

using MusECore::UndoOp;

namespace MusEGui {

//---------------------------------------------------------
//   setRecordFlag
//---------------------------------------------------------

void Strip::setRecordFlag(bool flag)
      {
      if (record) {
            record->blockSignals(true);
            record->setChecked(flag);
            record->blockSignals(false);
//             record->setIcon(flag ? QIcon(*record_on_Icon) : QIcon(*record_off_Icon));  // REMOVE Tim. Trackinfo. Removed.
            //record->setIconSize(record_on_Icon->size());  
            }
      }

//---------------------------------------------------------
//   resetPeaks
//---------------------------------------------------------

void Strip::resetPeaks()
      {
      track->resetPeaks();
      }

//---------------------------------------------------------
//   recordToggled
//---------------------------------------------------------

void Strip::recordToggled(bool val)
      {
      if (track->type() == MusECore::Track::AUDIO_OUTPUT) {
            if (val && track->recordFlag() == false) {
                  MusEGlobal::muse->bounceToFile((MusECore::AudioOutput*)track);
                  }
            MusEGlobal::audio->msgSetRecord((MusECore::AudioOutput*)track, val);
            if (!((MusECore::AudioOutput*)track)->recFile())
            {  
                  record->setChecked(false);
//                   record->setIcon(QIcon(*record_off_Icon));  // REMOVE Tim. Trackinfo. Removed.
                  //record->setIconSize(record_on_Icon->size());  
            }      
            return;
            }
      MusEGlobal::song->setRecordFlag(track, val);
      }
//---------------------------------------------------------
//   heartBeat
//---------------------------------------------------------

void Strip::heartBeat()
      {
      }

//---------------------------------------------------------
//   setLabelFont
//---------------------------------------------------------

void Strip::setLabelFont()
{
  // Use the new font #6 I created just for these labels (so far).
  // Set the label's font.
  label->setFont(MusEGlobal::config.fonts[6]);
  // Dealing with a horizontally constrained label. Ignore vertical. Use a minimum readable point size.
  MusECore::autoAdjustFontSize(label, label->text(), false, true, MusEGlobal::config.fonts[6].pointSize(), 5); 
}

//---------------------------------------------------------
//   setLabelText
//---------------------------------------------------------

void Strip::setLabelText()
{
      QColor c;
      switch(track->type()) {
            case MusECore::Track::AUDIO_OUTPUT:
                  //c = Qt::green;
                  c = MusEGlobal::config.outputTrackLabelBg;
                  break;
            case MusECore::Track::AUDIO_GROUP:
                  //c = Qt::yellow;
                  c = MusEGlobal::config.groupTrackLabelBg;
                  break;
            case MusECore::Track::AUDIO_AUX:
                  //c = QColor(120, 255, 255);   // Light blue
                  c = MusEGlobal::config.auxTrackLabelBg;
                  break;
            case MusECore::Track::WAVE:
                  //c = Qt::magenta;
                  c = MusEGlobal::config.waveTrackLabelBg;
                  break;
            case MusECore::Track::AUDIO_INPUT:
                  //c = Qt::red;
                  c = MusEGlobal::config.inputTrackLabelBg;
                  break;
            case MusECore::Track::AUDIO_SOFTSYNTH:
                  //c = QColor(255, 130, 0);  // Med orange
                  c = MusEGlobal::config.synthTrackLabelBg;
                  break;
            case MusECore::Track::MIDI:
                  //c = QColor(0, 160, 255); // Med blue
                  c = MusEGlobal::config.midiTrackLabelBg;
                  break;
            case MusECore::Track::DRUM:
                  //c = QColor(0, 160, 255); // Med blue
                  c = MusEGlobal::config.drumTrackLabelBg;
                  break;
            case MusECore::Track::NEW_DRUM:
                  //c = QColor(0, 160, 255); // Med blue
                  c = MusEGlobal::config.newDrumTrackLabelBg;
                  break;
            default:
                  return;      
            }
      
      if (track->type() == MusECore::Track::AUDIO_AUX) {
          label->setText(((MusECore::AudioAux*)track)->auxName());
      } else {
          label->setText(track->name());
      }
      QPalette palette;
      //palette.setColor(label->backgroundRole(), c);
      QLinearGradient gradient(label->geometry().topLeft(), label->geometry().bottomLeft());
      //gradient.setColorAt(0, c.darker());
      //gradient.setColorAt(0, c);
      //gradient.setColorAt(1, c.darker());
      gradient.setColorAt(0, c);
      gradient.setColorAt(0.5, c.lighter());
      gradient.setColorAt(1, c);
      //palette.setBrush(QPalette::Button, gradient);
      //palette.setBrush(QPalette::Window, gradient);
      palette.setBrush(label->backgroundRole(), gradient);
      label->setPalette(palette);
      
      //label->setStyleSheet(QString("background-color: ") + c.name());
}

//---------------------------------------------------------
//   muteToggled
//---------------------------------------------------------

void Strip::muteToggled(bool val)
      {
      track->setMute(val);
      MusEGlobal::song->update(SC_MUTE);
      }

//---------------------------------------------------------
//   soloToggled
//---------------------------------------------------------

void Strip::soloToggled(bool val)
      {
      MusEGlobal::audio->msgSetSolo(track, val);
      MusEGlobal::song->update(SC_SOLO);
      }

//---------------------------------------------------------
//   Strip
//    create mixer strip
//---------------------------------------------------------

Strip::Strip(QWidget* parent, MusECore::Track* t)
   : QFrame(parent)
      {
      setMouseTracking(true);
      
      _curGridRow = 0;
      _userWidth = 0;
      autoType = 0;
      //_resizeMode = ResizeModeNone;
      setAttribute(Qt::WA_DeleteOnClose);
      iR            = 0;
      oR            = 0;
      
      ///setBackgroundRole(QPalette::Mid);
      setFrameStyle(Panel | Raised);
      setLineWidth(2);
      
      // NOTE: Workaround for freakin' improper disabled button text colour (at least with Oxygen colours). 
      // Just set the parent palette.
      //QPalette pal(palette());
      //pal.setColor(QPalette::Disabled, QPalette::ButtonText, 
      //             pal.color(QPalette::Disabled, QPalette::WindowText));
      //setPalette(pal);
      
      track    = t;
      meter[0] = 0;
      meter[1] = 0;
      //setFixedWidth(STRIP_WIDTH);
      //setMinimumWidth(STRIP_WIDTH);     // TESTING Tim.
      //setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding)); // TESTING Tim.
//       setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding)); // TESTING Tim.
      setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
      
      grid = new QGridLayout();
      grid->setContentsMargins(0, 0, 0, 0);
      grid->setSpacing(0);
      setLayout(grid);

      //---------------------------------------------
      //    label
      //---------------------------------------------

      //label = new QLabel(this);
      // NOTE: This was required, otherwise the strip labels have no colour in the mixer only - track info OK !
      // Not sure why...
      label = new QLabel(this);
      label->setObjectName(track->cname());
      
      // Moved by Tim. p3.3.9
      //setLabelText();
      //label->setFont(MusEGlobal::config.fonts[1]);
      
      //printf("Strip::Strip w:%d frw:%d layoutmarg:%d lx:%d ly:%d lw:%d lh:%d\n", STRIP_WIDTH, frameWidth(), layout->margin(), label->x(), label->y(), label->width(), label->height());
      
      // Tested: The label's width is 100. It does not become STRIP_WIDTH - 2*layout->margin
      //  until the mixer is shown in MusE::showMixer.
      // Therefore 'fake' set the size of the label now.
      // Added by Tim. p3.3.9
      //label->setGeometry(label->x(), label->y(), STRIP_WIDTH - 2*frameWidth() - 2*layout->margin(), label->height());
      ///label->setGeometry(label->x(), label->y(), STRIP_WIDTH - 2*grid->margin(), label->height());
      
      label->setTextFormat(Qt::PlainText);
      
      // Unfortunately for the mixer labels, QLabel doesn't support the BreakAnywhere flag.
      // Changed by Tim. p3.3.9
      //label->setAlignment(AlignCenter);
      //label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
      // MusE-2 Tested: TextWrapAnywhere actually works, but in fact it takes precedence 
      //  over word wrap, so I found it is not really desirable. Maybe with a user setting...
      //label->setAlignment(Qt::AlignCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere);
      // changed by Orcan: We can't use Qt::TextWordWrap in alignment in Qt4.
      label->setAlignment(Qt::AlignCenter);
      label->setWordWrap(true);
      label->setAutoFillBackground(true);
      label->setLineWidth(2);
      label->setFrameStyle(Sunken | StyledPanel);
      
      //label->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum));
      label->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum));
      
      // Added by Tim. p3.3.9
      setLabelText();
      setLabelFont();
      
      //layout->addWidget(label);
//       grid->addWidget(label, _curGridRow++, 0, 1, 2);
      grid->addWidget(label, _curGridRow++, 0, 1, 3); // REMOVE Tim. Trackinfo. Changed. TEST
      }

//---------------------------------------------------------
//   Strip
//---------------------------------------------------------

Strip::~Strip()
      {
      }

void Strip::addGridWidget(QWidget* w, const GridPosStruct& pos, Qt::Alignment alignment)
{
  grid->addWidget(w, pos._row, pos._col, pos._rowSpan, pos._colSpan, alignment);
}

void Strip::addGridLayout(QLayout* l, const GridPosStruct& pos, Qt::Alignment alignment)
{
  grid->addLayout(l, pos._row, pos._col, pos._rowSpan, pos._colSpan, alignment);
}
      
//---------------------------------------------------------
//   setAutomationType
//---------------------------------------------------------

void Strip::setAutomationType(int t)
{
  // If going to OFF mode, need to update current 'manual' values from the automation values at this time...   
  if(t == AUTO_OFF && track->automationType() != AUTO_OFF) // && track->automationType() != AUTO_WRITE)
  {
    // May have a lot to do in updateCurValues, so try using idle.
    MusEGlobal::audio->msgIdle(true);
    track->setAutomationType(AutomationType(t));
    if(!track->isMidiTrack())
      (static_cast<MusECore::AudioTrack*>(track))->controller()->updateCurValues(MusEGlobal::audio->curFramePos());
    MusEGlobal::audio->msgIdle(false);
  }
  else
    // Try it within one message.
    MusEGlobal::audio->msgSetTrackAutomationType(track, t);   
  
  MusEGlobal::song->update(SC_AUTOMATION);
}
      
void Strip::resizeEvent(QResizeEvent* ev)
{
  //printf("Strip::resizeEvent\n");  
  QFrame::resizeEvent(ev);
  setLabelText();  
  setLabelFont();
}  

void Strip::mousePressEvent(QMouseEvent* ev)
{
  // Only one button at a time.
  if(ev->buttons() ^ ev->button())
  {
    //_resizeMode = ResizeModeNone;
    //unsetCursor();
    ev->accept();
    return;
  }
  
  //if(_resizeMode == ResizeModeHovering)
  //{
    // Switch to dragging mode.
    //_resizeMode = ResizeModeDragging;
    //ev->ignore();
    //return;
  //}
  
  if (ev->button() == Qt::RightButton) {
    QMenu* menu = new QMenu;
    menu->addAction(tr("Remove track?"));
    QPoint pt = QCursor::pos();
    QAction* act = menu->exec(pt, 0);
    if (!act)
    {
      delete menu;
      QFrame::mousePressEvent(ev);
      return;
    }
    MusEGlobal::song->applyOperation(UndoOp(UndoOp::DeleteTrack, MusEGlobal::song->tracks()->index(track), track));
    ev->accept();
    return;
  }
  QFrame::mousePressEvent(ev);
}

// REMOVE Tim. Trackinfo. Added.
// void Strip::mouseMoveEvent(QMouseEvent* ev)
// {
//   QFrame::mouseMoveEvent(ev);
//   const QPoint p = ev->pos();
//   switch(_resizeMode)
//   {
//     case ResizeModeNone:
//     {
//       if(p.x() >= (width() - frameWidth()) && p.x() < width() && p.y() >= 0 && p.y() < height())
//       {
//         fprintf(stderr, "Strip::mouseMoveEvent ResizeModeNone resize area hit\n"); // REMOVE Tim. Trackinfo.
//         _resizeMode = ResizeModeHovering;
//         setCursor(Qt::SizeHorCursor);
//       }
//       ev->ignore();
//       return;
//     }
//     break;
// 
//     case ResizeModeHovering:
//     {
//       if(p.x() < (width() - frameWidth()) || p.x() >= width() || p.y() < 0 || p.y() >= height())
//       {
//         fprintf(stderr, "Strip::mouseMoveEvent ResizeModeHovering resize area not hit\n"); // REMOVE Tim. Trackinfo.
//         _resizeMode = ResizeModeNone;
//         unsetCursor();
//       }
//       ev->ignore();
//       return;
//     }
//     break;
//     
//     case ResizeModeDragging:
//     {
//       //setUserWidth(p.x() - width());
//       setUserWidth(p.x() + 1);
//       ev->ignore();
//       return;
//     }
//     break;
//   }
// }
// 
// void Strip::mouseReleaseEvent(QMouseEvent* ev)
// {
//   QFrame::mouseReleaseEvent(ev);
//   switch(_resizeMode)
//   {
//     case ResizeModeNone:
//     case ResizeModeHovering:
//       ev->ignore();
//       return;
//     break;
//     
//     case ResizeModeDragging:
//     {
//       const QPoint p = ev->pos();
//       if(p.x() >= (width() - frameWidth()) && p.x() < width() && p.y() >= 0 && p.y() < height())
//       {
//         fprintf(stderr, "Strip::mouseReleaseEvent ResizeModeDragging resize area hit\n"); // REMOVE Tim. Trackinfo.
//         _resizeMode = ResizeModeHovering;
//         setCursor(Qt::SizeHorCursor);
//       }
//       else
//       {
//         fprintf(stderr, "Strip::mouseReleaseEvent ResizeModeDragging resize area not hit\n"); // REMOVE Tim. Trackinfo.
//         _resizeMode = ResizeModeNone;
//         unsetCursor();
//       }
//       ev->ignore();
//       return;
//     }
//     break;
//   }
// }
// 
// void Strip::leaveEvent(QEvent* ev)
// {
//   ev->ignore();
//   QFrame::leaveEvent(ev);
//   switch(_resizeMode)
//   {
//     case ResizeModeDragging:
//       return;
//     break;
//     
//     case ResizeModeHovering:
//       _resizeMode = ResizeModeNone;
//       // Fall through...
//     case ResizeModeNone:
//       unsetCursor();
//       return;
//     break;
//   }
// }
// 
 
QSize Strip::sizeHint() const
{
  const QSize sz = QFrame::sizeHint();
  return QSize(sz.width() + _userWidth, sz.height());
//   return QSize(_userWidth, sz.height());
}

void Strip::setUserWidth(int w)
{
  _userWidth = w;
  if(_userWidth < 0)
    _userWidth = 0;
  
//   grid->invalidate();
//   grid->activate();
//   grid->update();
//   adjustSize();
  updateGeometry();
}

void Strip::changeUserWidth(int delta)
{
  _userWidth += delta;
  if(_userWidth < 0)
    _userWidth = 0;
  updateGeometry();
}

//---------------------------------------------------------
//   ExpanderHandle
//---------------------------------------------------------

ExpanderHandle::ExpanderHandle(QWidget* parent, int handleWidth, Qt::WindowFlags f) 
              : QFrame(parent, f), _handleWidth(handleWidth)
{
  setObjectName("ExpanderHandle");
  setCursor(Qt::SizeHorCursor);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
  setFixedWidth(_handleWidth);
  setContentsMargins(0, 0, 0, 0);
 _resizeMode = ResizeModeNone;
}
 
void ExpanderHandle::mousePressEvent(QMouseEvent* e)
{
  // Only one button at a time.
//   if(e->buttons() ^ e->button())
//   {
//     //_resizeMode = ResizeModeNone;
//     //unsetCursor();
//     e->accept();
//     return;
//   }
  
  //if(_resizeMode == ResizeModeHovering)
  //{
    // Switch to dragging mode.
    //_resizeMode = ResizeModeDragging;
    //ev->ignore();
    //return;
  //}

  switch(_resizeMode)
  {
    case ResizeModeNone:
    case ResizeModeHovering:
      _dragLastGlobPos = e->globalPos();
      _resizeMode = ResizeModeDragging;
      e->accept();
      return;
    break;
    
    case ResizeModeDragging:
      e->accept();
      return;
    break;
  }
  
  e->ignore();
  QFrame::mousePressEvent(e);
}

void ExpanderHandle::mouseMoveEvent(QMouseEvent* e)
{
//   const QPoint p = e->pos();
  switch(_resizeMode)
  {
    case ResizeModeNone:
    {
//       if(p.x() >= (width() - frameWidth()) && p.x() < width() && p.y() >= 0 && p.y() < height())
//       {
//         fprintf(stderr, "Strip::mouseMoveEvent ResizeModeNone resize area hit\n"); // REMOVE Tim. Trackinfo.
//         _resizeMode = ResizeModeHovering;
//         setCursor(Qt::SizeHorCursor);
//       }
//       e->accept();
//       return;
    }
    break;

    case ResizeModeHovering:
    {
//       if(p.x() < (width() - frameWidth()) || p.x() >= width() || p.y() < 0 || p.y() >= height())
//       {
//         fprintf(stderr, "Strip::mouseMoveEvent ResizeModeHovering resize area not hit\n"); // REMOVE Tim. Trackinfo.
//         _resizeMode = ResizeModeNone;
//         unsetCursor();
//       }
//       e->accept();
//       return;
    }
    break;
    
    case ResizeModeDragging:
    {
      const QPoint gp = e->globalPos();
      const QPoint delta = gp -_dragLastGlobPos;
      _dragLastGlobPos = gp;
      emit moved(delta.x());
      e->accept();
      return;
    }
    break;
  }

  e->ignore();
  QFrame::mouseMoveEvent(e);
}

void ExpanderHandle::mouseReleaseEvent(QMouseEvent* e)
{
//   switch(_resizeMode)
//   {
//     case ResizeModeNone:
//     case ResizeModeHovering:
//     break;
//     
//     case ResizeModeDragging:
//     {
//       const QPoint p = ev->pos();
//       if(p.x() >= (width() - frameWidth()) && p.x() < width() && p.y() >= 0 && p.y() < height())
//       {
//         fprintf(stderr, "Strip::mouseReleaseEvent ResizeModeDragging resize area hit\n"); // REMOVE Tim. Trackinfo.
//         _resizeMode = ResizeModeHovering;
//         setCursor(Qt::SizeHorCursor);
//       }
//       else
//       {
//         fprintf(stderr, "Strip::mouseReleaseEvent ResizeModeDragging resize area not hit\n"); // REMOVE Tim. Trackinfo.
//         _resizeMode = ResizeModeNone;
//         unsetCursor();
//       }
//       ev->ignore();
//       return;
//     }
//     break;
//   }
  _resizeMode = ResizeModeNone;
  e->ignore();
  QFrame::mouseReleaseEvent(e);
}

// void ExpanderHandle::leaveEvent(QEvent* e)
// {
//   e->ignore();
//   QFrame::leaveEvent(e);
//   switch(_resizeMode)
//   {
//     case ResizeModeDragging:
//       return;
//     break;
//     
//     case ResizeModeHovering:
//       _resizeMode = ResizeModeNone;
//       Fall through...
//     case ResizeModeNone:
//       unsetCursor();
//       return;
//     break;
//   }
// }

QSize ExpanderHandle::sizeHint() const
{
  QSize sz = QFrame::sizeHint();
  sz.setWidth(_handleWidth);
  return sz;
}


} // namespace MusEGui
