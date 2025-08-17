//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: poslabel.cpp,v 1.2.2.2 2009/04/06 01:24:55 terminator356 Exp $
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

//#include <stdlib.h>
#include "muse_math.h"

#include <QApplication>
#include <QStyle>

#include <limits.h>
#include "poslabel.h"
//#include "sync.h"
#include "sig.h"
#include "tempo.h"
#include "globals.h"

namespace MusEGlobal {
extern int mtcType;
}

namespace MusEGui {

//---------------------------------------------------------
//   PosLabel
//---------------------------------------------------------

PosLabel::PosLabel(QWidget* parent, const char* name)
  : QLabel(parent)
      {
      setObjectName(name);
      _tickValue = 0;
      _sampleValue = 0;
      _smpte = false;
      setFrameStyle(WinPanel | Sunken);
      setLineWidth(2);
      setMidLineWidth(3);
      //int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this); // ddskrjo 0
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
      setIndent(fw);
      //setContentsMargins(0,0,0,0);  
      updateValue();
      }

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize PosLabel::sizeHint() const
      {
      const QFontMetrics fm = fontMetrics();
      //int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this); // ddskrjo 0
      int fw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
      int h  = fm.height() + fw * 2;
      int w;
      if (_smpte)
            w  = 2 + fm.horizontalAdvance("999:99:99:99") + fw * 4;
      else
            w  = 2 + fm.horizontalAdvance("9999.99.99999") + fw * 4;
      return QSize(w, h)/*.expandedTo(QApplication::globalStrut())*/;
      }

//---------------------------------------------------------
//   updateValue
//---------------------------------------------------------

void PosLabel::updateValue()
      {
      QString s;
      if (_smpte) {
            double time = double(_sampleValue) / double(MusEGlobal::sampleRate);
            int min  = int(time) / 60;
            int sec  = int(time) % 60;
            double rest = time - (min * 60 + sec);
            switch(MusEGlobal::mtcType) {
                  case 0:     // 24 frames sec
                        rest *= 24;
                        break;
                  case 1:     // 25
                        rest *= 25;
                        break;
                  case 2:     // 30 drop frame
                        rest *= 30;
                        break;
                  case 3:     // 30 non drop frame
                        rest *= 30;
                        break;
                  }
            int frame = int(rest);
            int subframe = int((rest-frame)*100);
            s = QString("%1:%2:%3:%4")
                .arg(min,      3, 10, QLatin1Char('0'))
                .arg(sec,      2, 10, QLatin1Char('0'))
                .arg(frame,    2, 10, QLatin1Char('0'))
                .arg(subframe, 2, 10, QLatin1Char('0'));
            }
      else {
            int bar, beat;
            unsigned tick;
            MusEGlobal::sigmap.tickValues(_tickValue, &bar, &beat, &tick);
            s = QString("%1.%2.%3")
                .arg(bar + 1,      4, 10, QLatin1Char('0'))
                .arg(beat + 1,     2, 10, QLatin1Char('0'))
                .arg(tick,         3, 10, QLatin1Char('0'));
            }
      setText(s);
      }

//---------------------------------------------------------
//   setSampleValue
//---------------------------------------------------------

void PosLabel::setSampleValue(unsigned val)
      {
      if (val == INT_MAX)
      {
        setEnabled(false);
        return;
      }
      if(!isEnabled())
        setEnabled(true);
            
      if (val == _sampleValue)
            return;
      _sampleValue = val;
      updateValue();
      }

//---------------------------------------------------------
//   setTickValue
//---------------------------------------------------------

void PosLabel::setTickValue(unsigned val)
      {
      if (val == INT_MAX)
      {
        setEnabled(false);
        return;
      }
      if(!isEnabled())
        setEnabled(true);
            
      if (val == _tickValue)
            return;
      if (val >= MAX_TICK)
      {
            printf("THIS SHOULD NEVER HAPPEN: val=%u > MAX_TICK=%u in PosLabel::setTickValue()!\n",val, MAX_TICK);
            val=MAX_TICK-1;
      }
      
      _tickValue = val;
      updateValue();
      }

//---------------------------------------------------------
//   setValue
//---------------------------------------------------------

void PosLabel::setValue(unsigned val)
      {
      if (val == INT_MAX)
      {
        setEnabled(false);
        return;
      }
      if(!isEnabled())
        setEnabled(true);
            
      unsigned oval = _smpte ? _sampleValue : _tickValue;
      if (val == oval)
            return;
      if (_smpte)
            _sampleValue = val;
      else
            _tickValue = val;
      updateValue();
      }

//---------------------------------------------------------
//   setSmpte
//---------------------------------------------------------

void PosLabel::setSmpte(bool val)
      {
      _smpte = val;
      if (val)
            _sampleValue = MusEGlobal::tempomap.tick2frame(_tickValue);
      else
            _tickValue = MusEGlobal::tempomap.frame2tick(_sampleValue);
      updateValue();
      }

} // namespace MusEGui
