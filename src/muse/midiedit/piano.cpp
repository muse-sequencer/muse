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
#include <QLocale>

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
    selectedPitch = 60;  // Start with middle 'C'
    keyDown = -1;
    button = Qt::NoButton;
    setStatusTip(tr("Piano: Press key to play or enter events in step record mode (SHIFT for chords). RMB: Set cursor for polyphonic control events. CTRL+Mousewheel to zoom view vertically."));
}

//---------------------------------------------------------
//   draw
//---------------------------------------------------------

void Piano::draw(QPainter& p, const QRect&, const QRegion&)
{
  const int selPitchY = pitch2y(selectedPitch);
  const int curPitchY = pitch2y(curPitch);

  const QColor colKeyCur = MusEGlobal::config.pianoCurrentKey;
  const QColor colKeyCurP = MusEGlobal::config.pianoPressedKey;
  const QColor colKeySel = MusEGlobal::config.pianoSelectedKey;
  const qreal rad = 1.0;

  QPen pen(QColor(80,80,80));
  pen.setCosmetic(true);

  const int sclsz = MusEGlobal::config.noteNameList.size();
  const int snote = MusEGlobal::config.noteNameList.startingMidiNote();
  const bool showpiano = MusEGlobal::config.globalShowPiano;
  const bool nopiano = sclsz != 12 || !showpiano;

  if(nopiano)
  {
    const int pianoH = pianoHeight();

    // draw white keys
    {
        const QColor colKeyW("Ivory");

        pen.setWidth(1);
        p.setPen(pen);
        p.setBrush(colKeyW);
        p.setRenderHint(QPainter::Antialiasing);

        if(MusEGlobal::config.pianoShowNoteColors)
        {
          // We need to make sure that the colours here match the colours on the piano canvas.
          // That means first drawing the piano canvas background colour and/or possibly a darker
          //  lane colour, then the transparent note colour. So it looks identical to the canvas.
          p.fillRect(0, 0, pianoWidth, pianoH, MusEGlobal::config.midiCanvasBg);
        }
        else
        {
          // Draw a slight gradient from the top to the bottom of each octave
          //  to help distinguish where each octave starts.
          // (Note that drawing a repeating gradient all the way from piano top to bottom
          //  doesn't work precisely. The border slowy wanders as it's drawn. Same result as
          //  using relative mode (0.0 - 1.0). So we draw by octave instead.)
          const int topnote = (127 + snote) % sclsz;
          const int gradoff = sclsz - topnote - 1;
          QLinearGradient g(0.0, -gradoff * KH_MT, 0.0, (sclsz - gradoff) * KH_MT);
          g.setSpread(QGradient::RepeatSpread);
          g.setColorAt(0.0, colKeyW.darker(118));
          g.setColorAt(1.0, colKeyW);
          p.setBrush(g);
          int starty = 0;
          int endy = (topnote + 1) * KH_MT;

          while(endy < pianoH)
          {
            p.fillRect(0, starty, pianoWidth, endy - starty, g);
            starty = endy;
            endy += sclsz * KH_MT;
          }

          // Fill remainder at bottom.
          if(pianoH - starty > 0)
            p.fillRect(0, starty, pianoWidth, pianoH - starty, g);
        }

        int y = 0;
        // 128 keys.
        for (int i = 0; i < 128; ++i) {
            y = pitch2y(i);

            if (y == selPitchY)
            {
                p.setBrush(colKeySel);
                p.fillRect(0, y, pianoWidth, KH_MT, p.brush());
            }
            else if(MusEGlobal::config.pianoShowNoteColors)
            {
              QColor c = MusECore::noteColorScrambled((i + snote) % sclsz);
              c.setAlpha(MusEGlobal::config.globalAlphaBlend);
              p.fillRect(0, y, pianoWidth, KH_MT, c);
            }

            p.setRenderHint(QPainter::Antialiasing, false);
            // Octave divider? Draw solid line, else draw broken line.
            if(((i + snote + 1) % sclsz) == 0)
              pen.setStyle(Qt::SolidLine);
            else
              pen.setStyle(Qt::DotLine);
            p.setPen(pen);
            p.drawLine(0, y, pianoWidth, y);
            // Reset to solid.
            pen.setStyle(Qt::SolidLine);
            p.setPen(pen);

            p.drawLine(pianoWidth, y, pianoWidth, y + KH_MT);

            if (y == curPitchY) {
                p.save();
                if (curPitch == keyDown)
                    p.setBrush(colKeyCurP);
                else
                    p.setBrush(colKeyCur);
                p.setPen(Qt::NoPen);
                p.setRenderHint(QPainter::Antialiasing);
                p.drawRoundedRect(pianoWidth * 0.65, y + 2, pianoWidth * 0.3, KH_MT - 4, rad, rad);
                p.restore();
            }
        }

        p.setRenderHint(QPainter::Antialiasing, false);
        p.drawLine(0, pianoH, pianoWidth, pianoH);
    }

    // draw note names
    {
        QFont f(MusEGlobal::config.fonts[0].family(), 7);
        QFontMetrics fm(f);
        p.setFont(f);
        p.setRenderHint(QPainter::Antialiasing);

        int y;
        for (int i = 0; i < 128; ++i)
        {
            p.setPen(Qt::black);
            y = pitch2y(i);
            if(y == selPitchY)
            {
              if(!MusECore::isColorBright(colKeySel))
                p.setPen(Qt::white);
            }
            else if(MusEGlobal::config.pianoShowNoteColors)
            {
              if(!MusECore::isColorBright(MusECore::noteColorScrambled((i + snote) % sclsz)))
                p.setPen(Qt::white);
            }

            const int note = (i + snote) % sclsz;
            const MusECore::NoteName nn = MusEGlobal::config.noteNameList.findNoteName(note);
            // -1 if not found.
            if(nn.noteNum() == -1)
              continue;
            const int oct = (i + snote) / sclsz;
            // Octave divider?
            //if(note == 0)
            {
              QString s = nn.firstName();
              // Octave note?
              if(note == 0)
                s += QLocale().toString(oct + MusEGlobal::config.noteNameList.startingMidiOctave() +
                                     MusEGlobal::config.globalOctaveSuffixOffset);

// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
            const int tw = fm.horizontalAdvance(s);
#else
            const int tw = fm.size(s);
#endif
            // Avoid fooling around with Qt alignment. Force right alignment.
            // For text, y is the baseline of the font.
            p.drawText(pianoWidth - 3 - tw, y + KH_MT - 1, s);

            }
        }
    }
  }
  else
  // Normal piano.
  {
    pen.setWidth(2);
    p.setPen(pen);
    p.setRenderHint(QPainter::Antialiasing);

    const int pianoH = pianoHeight();

    const int hn [] { false,true,false,true,false,false,true,false,true,false,false,true };
    const bool ishalfnote = hn[snote];

    // draw white keys
    {

        const QColor colKeyW("Ivory");

        p.setBrush(colKeyW);

        int y = 0;
        if(ishalfnote)
        {
          y = KH / 2;
          // Need to fill in the top half white key area with something.
          p.fillRect(0, 0, pianoWidth, y, p.brush());
          p.drawLine(0, 0, pianoWidth, 0);
          p.drawLine(pianoWidth, 0, pianoWidth, y);
        }

        for (; y < pianoH; y += KH)
        {
            //fprintf(stderr, "Piano::draw white key: y:%d curPitchY:%d selPitchY:%d ishalfnote:%d\n",
            //  y, curPitchY, selPitchY, ishalfnote);

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

        p.drawLine(0, pianoH, pianoWidth, pianoH);
    }

    // draw black keys
    {
        const int wb = pianoWidth * .6;
        const int keyHeightB = 7;
        const int topOffsetB = 10;

        QLinearGradient g(0.0, 0.0, 1.0, 0.0);
        g.setCoordinateMode(QGradient::ObjectBoundingMode);
        g.setColorAt(0.0, QColor(120, 120, 120));
        g.setColorAt(0.79, QColor(70, 70, 70));
        g.setColorAt(0.8, QColor(40, 40, 40));
        g.setColorAt(0.83, QColor(20, 20, 20));
        g.setColorAt(1.0, QColor(20, 20, 20));
        p.setBrush(g);

        // Black keys 5 and 10 are imaginary.
        const int sy  [] { topOffsetB,0,topOffsetB,0,topOffsetB,topOffsetB,0,topOffsetB,0,topOffsetB,topOffsetB,0 };
        const int sbk [] { 3,4,4,5,5,6,0,0,1,1,2,3 };
        int y = sy[snote];
        int sblackkey = sbk[snote];

        if(ishalfnote)
          y += KH / 4;

        // Stop at about half a white key from the bottom,
        //  to avoid drawing half a black key.
        const int yend = pianoH - (KH / 2 + 1);
        for (; y < yend; y += KH) {
            // Skip over where there are no black keys.
            if(sblackkey != 2 && sblackkey != 6)
            {
              //fprintf(stderr, "Piano::draw black key: y:%d curPitchY:%d selPitchY:%d ishalfnote:%d\n",
              //  y, curPitchY, selPitchY, ishalfnote);

              if ((y - 3) == selPitchY) {
                  p.setBrush(colKeySel);
                  p.drawRoundedRect(0, y, wb, keyHeightB, rad, rad);
                  p.setBrush(g);
              }
              else
              {
                  //fprintf(stderr, "Piano::draw: Drawing rounded rect x:0 y:%d w:%d h:%d rad:%f sblackkey:%d\n",
                  //        y, wb, keyHeightB, rad, sblackkey);

                  p.drawRoundedRect(0, y, wb, keyHeightB, rad, rad);
              }

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
            }

            --sblackkey;
            if(sblackkey < 0)
              // Reset the black key down-counter.
              sblackkey = 6;
        }
    }

    // draw shadow
    {
        QLinearGradient g(0.0, 0.0, 1.0, 0.0);
        g.setCoordinateMode(QGradient::ObjectBoundingMode);
        g.setColorAt(0.0, Qt::black);
        g.setColorAt(1.0, QColor(127, 127, 127, 0));
        p.setBrush(g);
        p.fillRect(0, 0, pianoWidth * .1, pianoH + 1, g);
    }

    // draw C notes
    {
        const int octaveHeight = 7 * KH;
        QFont f(MusEGlobal::config.fonts[0].family(), 7);
        QFontMetrics fm(f);
        p.setFont(f);
        p.setPen(Qt::black);

        // First note in the note name list is always an octave note.
        const MusECore::NoteName &nn = MusEGlobal::config.noteNameList.findNoteName(0);
        // -1 if not found.
        const QString octnotename = nn.noteNum() == -1 ? "C" : nn.firstName();
        const int octnote = (12 - MusEGlobal::config.noteNameList.startingMidiNote()) % 12;
        int y = pitch2y(octnote);
        // If the midi starting note is not zero, the displayed octave number starts at 1 higher.
        const int octoff = (snote > 0 ? 1 : 0);

        for (int oct = 0; oct < 11; ++oct) {
            const QString s = octnotename +
                      QLocale().toString(MusEGlobal::config.noteNameList.startingMidiOctave() +
                        MusEGlobal::config.globalOctaveSuffixOffset + oct + octoff);

// Width() is obsolete. Qt >= 5.11 use horizontalAdvance().
#if QT_VERSION >= 0x050b00
            const int tw = fm.horizontalAdvance(s);
#else
            const int tw = fm.size(s);
#endif
            // Avoid fooling around with Qt alignment. Force right alignment.
            // For text, y is the baseline of the font.
            p.drawText(pianoWidth - 3 - tw, y + KH - 4, s);

            y -= octaveHeight;
        }
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
	if (pitch<0)
		return 0;

  int y;
  const int pianoH = pianoHeight();
  const int sclsz = MusEGlobal::config.noteNameList.size();
  const int snote = MusEGlobal::config.noteNameList.startingMidiNote();
  const bool showpiano = MusEGlobal::config.globalShowPiano;
  const bool nopiano = sclsz != 12 || !showpiano;

  if(nopiano)
  {
    y = pianoH - (pitch + 1) * KH_MT;
  }
  else
  {
    const int KH2 = KH / 2;
    const int tt[] = {
      12, 19, 25, 32, 38, 51, 58, 64, 71, 77, 84, 90
    };
    const int toff[] = {
      0, 12 - KH2, 13, 25 - KH2, 26, 39, 51 - KH2, 52, 64 - KH2, 65, 66 + KH2, 78
    };
    const int starty = toff[snote % 12];
    y = pianoH - (tt[(snote + pitch) % 12] + (7 * KH) * ((snote + pitch) / 12) - starty);

    //fprintf(stderr, "Piano::pitch2y: pitch:%d y:%d starty:%d tableidx:%d tablev:%d pianoH:%d\n",
    //  pitch, y, starty, (snote + pitch) % 12, tt[(snote + pitch) % 12], pianoH);
  }
	if (y < 0)
		y = 0;
	return y;
}

//---------------------------------------------------------
//   y2pitch
//---------------------------------------------------------

int Piano::y2pitch(int y) const
{
  const int pianoH = pianoHeight();
  const int sclsz = MusEGlobal::config.noteNameList.size();
  const int snote = MusEGlobal::config.noteNameList.startingMidiNote();
  const bool showpiano = MusEGlobal::config.globalShowPiano;
  const bool nopiano = sclsz != 12 || !showpiano;

  if(nopiano)
  {
    //fprintf(stderr, "Piano::y2pitch: y:%d\n", y);

    if (y < KH_MT)
        return 127;

    // 128 steps.
    y = (pianoH - 1 - y) / KH_MT;

    //fprintf(stderr, "Piano::y2pitch: pitch:%d\n", y);

    if (y < 0)
        return 0;
    if (y >= 128)
    {
        //fprintf(stderr, "Piano::y2pitch: pitch:%d is >= 128\n", y);
        return 127;
    }
    return y;
  }
  else
  {
    int tt[] = {
      0, 6, 13, 19, 26, 39, 45, 52, 58, 65, 71, 78
    };
    const int starty = tt[snote];
    const int yy = pianoH - (y - starty);

    if (yy < 0)
        return 0;
    int oct = (yy / (7 * KH)) * 12;
    char kt[] = {
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*10*/        1, 1, 1, 1, 1, 1, 1,
/*17*/        2, 2, 2, 2, 2, 2,
/*23*/        3, 3, 3, 3, 3, 3, 3,
/*30*/        4, 4, 4, 4, 4, 4, 4, 4, 4,
/*39*/        5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
/*49*/        6, 6, 6, 6, 6, 6, 6,
/*56*/        7, 7, 7, 7, 7, 7,
/*62*/        8, 8, 8, 8, 8, 8, 8,
/*69*/        9, 9, 9, 9, 9, 9,
/*75*/        10, 10, 10, 10, 10, 10, 10,
/*82*/        11, 11, 11, 11, 11, 11, 11, 11, 11, 11
    };

    int pitch = kt[yy % 91] + oct - snote;

    //fprintf(stderr, "Piano::y2pitch: y:%d starty:%d yy:%d tableidx:%d tablev:%d pitch:%d pianoH:%d\n",
    //  y, starty, yy, yy % 91, kt[yy % 91], pitch, pianoH);

    // These will catch any half white key areas at the top and bottom, where pitch would be -1 or 128.
    if(pitch < 0)
      pitch = 0;
    if(pitch > 127)
      pitch = 127;
    return pitch;
  }
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
    //fprintf(stderr, "Piano::viewMouseMoveEvent: pitch:%d event y:%d\n", pitch, event->y());

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
        // avoid stuck notes when LMB is pressed too
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
    if (keyDown != -1 && event->key() == Qt::Key_Shift) {
        emit shiftReleased();
        keyDown = -1;
    }
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

// Static.
int Piano::pianoHeight()
{
  const int sclsz = MusEGlobal::config.noteNameList.size();
  const int snote = MusEGlobal::config.noteNameList.startingMidiNote();
//   const bool showpiano = MusEGlobal::config.noteNameList.showPiano();
  const bool showpiano = MusEGlobal::config.globalShowPiano;
  const bool nopiano = sclsz != 12 || !showpiano;
  // Micro-tonal:
  if(nopiano)
    return 128 * KH_MT;
  // Normal piano:
  const int h[] { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KH / 2, KH / 2 };
  return 75 * KH + h[snote];
}

} // namespace MusEGui
