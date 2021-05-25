//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: piano.cpp,v 1.3 2004/05/31 11:48:55 lunar_shuttle Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#include <QToolTip>
//#include <QDebug>

//#include <stdio.h>

#include "piano.h"
#include "globals.h"
#include "song.h"
#include "midiport.h"
#include "track.h"
#include "midictrl.h"
//#include "icons.h"
#include "utils.h"
#include "gconfig.h"

// Forwards from header:
#include <QEvent>
#include <QMouseEvent>
//#include <QWheelEvent>
#include <QPainter>
//#include <QPixmap>
#include "midieditor.h"

namespace MusEGui {

/*
static const char *oct_xpm[] = {
    // w h colors
    "40 91 2 1",
    ". c #c0c0c0",
    "# c #000000",
    //           x
    "########################################",
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#", // 10
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#", //------------------------
    "########################...............#",
    "########################...............#",
    "########################...............#",
    "########################################",     // 7
    "########################...............#",
    "########################...............#",
    "########################...............#", //------------------------
    ".......................................#",
    ".......................................#",
    ".......................................#",     // 6
    ".......................................#",
    ".......................................#",
    ".......................................#", //------------------------
    "########################...............#",
    "########################...............#",
    "########################...............#",     // 7
    "########################################",
    "########################...............#",
    "########################...............#",
    "########################...............#", //------------------------
    ".......................................#",
    ".......................................#",
    ".......................................#",    // 6
    ".......................................#",
    ".......................................#",
    ".......................................#", //------------------------
    "########################...............#",
    "########################...............#",
    "########################...............#",    // 7
    "########################################",
    "########################...............#",
    "########################...............#",
    "########################...............#", //------------------------
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#",    // 10
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#",
    "########################################", //----------------------
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#",    // 9
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#", //------------------------
    "########################...............#",
    "########################...............#",
    "########################...............#",
    "########################################",   // 7
    "########################...............#",
    "########################...............#",
    "########################...............#", //------------------------
    ".......................................#",
    ".......................................#",
    ".......................................#",     // 6
    ".......................................#",
    ".......................................#",
    ".......................................#", //--------------------------
    "########################...............#",
    "########################...............#",
    "########################...............#",     // 7
    "########################################",
    "########################...............#",
    "########################...............#",
    "########################...............#", //------------------------
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#",
    ".......................................#",     // 9
    ".......................................#",
    ".......................................#",
    ".......................................#",
};
*/

/*
      0   1   2  3  4  5  6  7  8  9  10
      c-2 c-1 C0 C1 C2 C3 C4 C5 C6 C7 C8 - G8

      Grid über Oktave:

           +------------+ ------------------------------
       11  |            |
           |         h  |         7
           +------+     |
       10  |  a#  +-----+ ..............................
           +------+  a  |
        9  |            |         6
           +------+     |
        8  |  g#  +-----+ ..............................
           +------+  g  |
        7  |            |         5
           +------+     |
        6  |  f#  +-----+ ..............................
           +------+  f  |
        5  |            |         4
           |            |
           +------------+ ------------------------------
        4  |            |
           |         e  |         3
           +------+     |
        3  |  d#  +-----+ ..............................
           +------+  d  |
        2  |            |         2
           +------+     |
        1  |  c#  +-----+ ..............................
           +------+  c  |
           |            |         1
        0  |            |
           +------------+ ------------------------------
 */

//---------------------------------------------------------
//   Piano
//---------------------------------------------------------

Piano::Piano(QWidget* parent, int ymag, int width, MidiEditor* editor)
    : View(parent, 1, ymag),
      pianoWidth(width)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    _midiEditor = editor;
    curPitch = -1;
    selectedPitch = 60;  // Start with 'C3"
    keyDown = -1;
    button = Qt::NoButton;
    setStatusTip(tr("Piano: Press key to play or enter events in step record mode (SHIFT for chords). RMB: Set cursor for polyphonic control events. CTRL+Mousewheel to zoom view vertically."));
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Piano::draw(QPainter& p, const QRect&, const QRegion&)
{
    //      QRect ur = mapDev(mr);
    //      if (ur.height() > pianoHeight)
    //          ur.setHeight(pianoHeight);
    //      // FIXME: For some reason need the expansion otherwise drawing
    //      //        artifacts (incomplete drawing). Can't figure out why.
    //      ur.adjust(0, -4, 0, 4);

    const int selPitchY = pitch2y(selectedPitch);
    const int curPitchY = pitch2y(curPitch);

    const QColor colKeyCur = MusEGlobal::config.pianoCurrentKey;
    const QColor colKeyCurP = MusEGlobal::config.pianoPressedKey;
    const QColor colKeySel = MusEGlobal::config.pianoSelectedKey;
    const qreal rad = 1.0;

    QPen pen(QColor(80,80,80));
    pen.setCosmetic(true);
    pen.setWidth(2);
    p.setPen(pen);
    p.setRenderHint(QPainter::Antialiasing);

    // draw white keys
    {
        const QColor colKeyW("Ivory");

        p.setBrush(colKeyW);

        int y = 0;
        for (int i = 0; i < 75; i++) {
            y = i * KH;
            if (y + 1 == selPitchY)
                p.setBrush(colKeySel);

            p.fillRect(0, y, pianoWidth, KH, p.brush());
            p.drawLine(0, y, pianoWidth, y);
            p.drawLine(pianoWidth, y, pianoWidth, y + KH);

            if (y + 1 == selPitchY)
                p.setBrush(colKeyW);

            if (y + 1 == curPitchY) {
                p.save();
                if (curPitch == keyDown)
                    p.setBrush(colKeyCurP);
                else
                    p.setBrush(colKeyCur);
                p.setPen(Qt::NoPen);
                p.drawRoundedRect(pianoWidth * 0.65, y + 2, pianoWidth * 0.3, 9, rad, rad);
                p.restore();
            }
        }
        y = 75 * KH;
        p.drawLine(0, y, pianoWidth, y);
    }

    // draw black keys
    {
        const int keyHeightB = 7;
        const int distSmallB = 6;
        const int distLargeB = 19;
        const int topOffsetB = 10;

        QLinearGradient g(0.0, 0.0, 1.0, 0.0);
        g.setCoordinateMode(QGradient::ObjectBoundingMode);
        g.setColorAt(0.0, QColor(120, 120, 120));
        g.setColorAt(0.79, QColor(70, 70, 70));
        g.setColorAt(0.8, QColor(40, 40, 40));
        g.setColorAt(0.83, QColor(20, 20, 20));
        g.setColorAt(1.0, QColor(20, 20, 20));
        p.setBrush(g);

        int cnt = 2;
        bool flag = true;
        int y = topOffsetB;
        int wb = pianoWidth * .6;
        for (int i = 0; i < 53; i++) {
            if ((y - 3) == selPitchY) {
                p.setBrush(colKeySel);
                p.drawRoundedRect(0, y, wb, keyHeightB, rad, rad);
                p.setBrush(g);
            }
            else
                p.drawRoundedRect(0, y, wb, keyHeightB, rad, rad);

            if ((y - 3) == curPitchY) {
                p.save();
                if (curPitch == keyDown)
                    p.setBrush(colKeyCurP);
                else
                    p.setBrush(colKeyCur);
                p.setPen(Qt::NoPen);
                p.drawRoundedRect(pianoWidth * 0.2, y + 1, pianoWidth * 0.3, 5, rad, rad);
                p.restore();
            }

            if ((flag && ++cnt == 3) || (!flag && ++cnt == 2)) {
                y = y + distLargeB + keyHeightB;
                flag = !flag;
                cnt = 0;
            }
            else
                y = y + distSmallB + keyHeightB;
        }
    }

    // draw shadow
    {
        const int pianoHeight = (7 * KH * 10) + (KH * 5) + 1;
        QLinearGradient g(0.0, 0.0, 1.0, 0.0);
        g.setCoordinateMode(QGradient::ObjectBoundingMode);
        g.setColorAt(0.0, Qt::black);
        g.setColorAt(1.0, QColor(127, 127, 127, 0));
        p.setBrush(g);
        p.fillRect(0, 0, pianoWidth * .1, pianoHeight, g);
    }

    // draw C notes
    {
        const int octaveHeight = 7 * KH;
        QFont f(MusEGlobal::config.fonts[0].family(), 7);
        QFontMetrics fm(f);
        p.setFont(f);
        p.setPen(Qt::black);
        int y = 5 * KH;
        for (int i = 0; i < 11; i++) {
            QString s("C" + QString::number(8 - i));
            p.drawText(pianoWidth - fm.size(0, s).width() - (pianoWidth / 10 - 3), y - 3, s);
            y += octaveHeight;
        }
    }


    if(!_midiEditor)
        return;

    MusECore::PartList* part_list = _midiEditor->parts();
    MusECore::Part* cur_part = _midiEditor->curCanvasPart();
    if(!part_list || !cur_part || !cur_part->track()->isMidiTrack())
        return;

    MusECore::MidiTrack* track = (MusECore::MidiTrack*)(cur_part->track());
    int channel      = track->outChannel();
    MusECore::MidiPort* port   = &MusEGlobal::midiPorts[track->outPort()];
    MusECore::MidiCtrlValListList* cll = port->controller();
    const int min = channel << 24;
    const int max = min + 0x1000000;

    for(MusECore::ciMidiCtrlValList it = cll->lower_bound(min); it != cll->lower_bound(max); ++it)
    {
        MusECore::MidiCtrlValList* cl = it->second;
        MusECore::MidiController* c   = port->midiController(cl->num(), channel);
        if(!c->isPerNoteController())
            continue;
        int cnum = c->num();
        int num = cl->num();
        int pitch = num & 0x7f;
        bool used = false;
        for (MusECore::ciEvent ie = cur_part->events().begin(); ie != cur_part->events().end(); ++ie)
        {
            MusECore::Event e = ie->second;
            if(e.type() != MusECore::Controller)
                continue;
            int ctl_num = e.dataA();
            if((ctl_num | 0xff) == cnum && (ctl_num & 0x7f) == pitch)
            {
                used = true;
                break;
            }
        }

        bool off = cl->hwVal() == MusECore::CTRL_VAL_UNKNOWN;  // Does it have a value or is it 'off'?

        if (used)
        {
            if (off)
                p.setBrush(QColor("MediumSeaGreen"));
            else
                p.setBrush(QColor("OrangeRed"));
        }
        else
        {
            if (off)
                p.setBrush(QColor(179, 179,179)); // Gray
            else
                p.setBrush(QColor("DodgerBlue"));
        }

        qreal y = pitch2y(pitch) + 4.;
        qreal w = pianoWidth;
        QPainterPath path;
        path.moveTo(w * 0.1, y);
        path.lineTo(w * 0.1, y + 5.0);
        path.lineTo(w * 0.2, y + 3.0);
        path.lineTo(w * 0.1, y);
        p.fillPath(path, p.brush());
    }

}

//---------------------------------------------------------
//   pitch2y
//---------------------------------------------------------

int Piano::pitch2y(int pitch) const
      {
      int tt[] = {
            12, 19, 25, 32, 38, 51, 58, 64, 71, 77, 84, 90
            };
      int y = (75 * KH) - (tt[pitch%12] + (7 * KH) * (pitch/12));
      if (y < 0)
            y = 0;
      return y;
      }

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int Piano::y2pitch(int y) const
{
    if (y < KH)
        return 127;
    const int total = (10 * 7 + 5) * KH;       // 75 full tone steps
    y = total - y;
    if (y < 0)
        return 0;
    int oct = (y / (7 * KH)) * 12;
    char kt[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7,
        8, 8, 8, 8, 8, 8, 8,
        9, 9, 9, 9, 9, 9,
        10, 10, 10, 10, 10, 10, 10,
        11, 11, 11, 11, 11, 11, 11, 11, 11, 11
    };
    return kt[y % 91] + oct;
}

//---------------------------------------------------------
//   leaveEvent
//---------------------------------------------------------

void Piano::leaveEvent(QEvent*)
      {
      if (keyDown != -1) {
            emit keyReleased(keyDown, shift);
            keyDown = -1;
            }
      emit pitchChanged(-1);
      setPitch(-1);
      }

//---------------------------------------------------------
//   setPitch
//---------------------------------------------------------

void Piano::setPitch(int pitch)
      {
      if (curPitch == pitch)
            return;
      curPitch = pitch;
      redraw();
      }

//---------------------------------------------------------
//   viewMouseMoveEvent
//---------------------------------------------------------

void Piano::viewMouseMoveEvent(QMouseEvent* event)
{
    int pitch = y2pitch(event->y());
    emit pitchChanged(pitch);
    setPitch(pitch);

    if (button == Qt::LeftButton) {
        int nk = y2pitch(event->y());
        if (nk < 0 || nk > 127)
            nk = -1;
        if (nk != keyDown) {
            if (keyDown != -1 && !shift) {
                emit keyReleased(keyDown, shift);
            }
            keyDown = nk;
            if (keyDown != -1) {
                int velocity = (event->x() + 1) * 127 / pianoWidth;
                if(velocity > 127)
                    velocity = 127;
                else if(velocity <= 0)
                    velocity = 1;
                emit keyPressed(keyDown, velocity, shift);
            }
            redraw();
        }
    }

    if (!MusEGlobal::config.showNoteTooltips)
        return;

    int v = qMax(1, qMin(127, (event->x() + 1) * 127 / pianoWidth));
    QString str = tr("Velocity: ") + QString::number(v);
    QToolTip::showText(event->globalPos(), str);
}

//---------------------------------------------------------
//   viewMousePressEvent
//---------------------------------------------------------

void Piano::viewMousePressEvent(QMouseEvent* event)
{
    button = event->button();
    shift  = event->modifiers() & Qt::ShiftModifier;

    if (button == Qt::LeftButton) {
        if (keyDown != -1 && !shift) {
            emit keyReleased(keyDown, shift);
            keyDown = -1;
        }
        keyDown = y2pitch(event->y());
        if (keyDown < 0 || keyDown > 127) {
            keyDown = -1;
        }
        else {
            int velocity = (event->x() + 1) * 127 / pianoWidth;
            if(velocity > 127)
                velocity = 127;
            else if(velocity <= 0)
                velocity = 1;
            emit keyPressed(keyDown, velocity, shift);
        }
    }

    if (button == Qt::RightButton) {
        if (keyDown != -1 && !shift) {
            emit keyReleased(keyDown, shift);
            keyDown = -1;
        }
        selectedPitch = y2pitch(event->y());
        emit curSelectedPitchChanged(selectedPitch);
        redraw();
        MusEGlobal::song->update(SC_DRUMMAP);
    }

    redraw();
}

//---------------------------------------------------------
//   viewMouseReleaseEvent
//---------------------------------------------------------

void Piano::viewMouseReleaseEvent(QMouseEvent* event)
{
    if (button == Qt::LeftButton) {
        shift = event->modifiers() & Qt::ShiftModifier;
        if (keyDown != -1 && !shift) {
            emit keyReleased(keyDown, shift);
            keyDown = -1;
        }
        redraw();
    }

    button = Qt::NoButton;
}

void Piano::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Shift)
        emit keyReleased(keyDown, true);
    else
        View::keyReleaseEvent(event);
}

//---------------------------------------------------------
//   setCurSelectedPitch
//---------------------------------------------------------

void Piano::setCurSelectedPitch(int pitch)
      {
      if (pitch < 0 || pitch >= 128)
        return;
      if (pitch != selectedPitch) {
            selectedPitch = pitch;
            emit curSelectedPitchChanged(selectedPitch);
            redraw();
            }
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void Piano::wheelEvent(QWheelEvent* ev)
{
    if (ev->modifiers() & Qt::ControlModifier) {
        const QPoint pixelDelta = ev->pixelDelta();
        const QPoint angleDegrees = ev->angleDelta() / 8;
        int delta = 0;
        if(!pixelDelta.isNull())
            delta = pixelDelta.y();
        else if(!angleDegrees.isNull())
            delta = angleDegrees.y() / 15;
        else
          return;

        emit wheelStep(delta > 0 ? true : false);
        return;
    }

    emit redirectWheelEvent(ev);
}

} // namespace MusEGui
