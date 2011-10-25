//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midifilterimpl.h,v 1.1.1.1 2003/10/27 18:52:40 wschweer Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
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

#ifndef __MIDIFILTERIMP_H__
#define __MIDIFILTERIMP_H__

#include "globals.h"
#include "ui_midifilter.h"

class QCloseEvent;

namespace MusEGui {

//---------------------------------------------------------
//   MidiFilterConfig
//---------------------------------------------------------

class MidiFilterConfig : public QDialog, public Ui::MidiFilterConfigBase {
      Q_OBJECT

      void rChanged(bool f, int val) {
            if (f)
                  MusEGlobal::midiRecordType |= val;
            else
                  MusEGlobal::midiRecordType &= ~val;
            }
      void tChanged(bool f, int val) {
            if (f)
                  MusEGlobal::midiThruType |= val;
            else
                  MusEGlobal::midiThruType &= ~val;
            }
      void chChanged(bool f, int val) {
            if (f)
                  MusEGlobal::midiInputChannel |= val;
            else
                  MusEGlobal::midiInputChannel &= ~val;
            }
      virtual void closeEvent(QCloseEvent*);

   signals:
      void hideWindow();

   private slots:
      void channelChanged1(bool f)  { chChanged(f, 0x01);   }
      void channelChanged2(bool f)  { chChanged(f, 0x02);   }
      void channelChanged3(bool f)  { chChanged(f, 0x04);   }
      void channelChanged4(bool f)  { chChanged(f, 0x08);   }
      void channelChanged5(bool f)  { chChanged(f, 0x10);   }
      void channelChanged6(bool f)  { chChanged(f, 0x20);   }
      void channelChanged7(bool f)  { chChanged(f, 0x40);   }
      void channelChanged8(bool f)  { chChanged(f, 0x80);   }
      void channelChanged9(bool f)  { chChanged(f, 0x100);  }
      void channelChanged10(bool f) { chChanged(f, 0x200);  }
      void channelChanged11(bool f) { chChanged(f, 0x400);  }
      void channelChanged12(bool f) { chChanged(f, 0x800);  }
      void channelChanged13(bool f) { chChanged(f, 0x1000); }
      void channelChanged14(bool f) { chChanged(f, 0x2000); }
      void channelChanged15(bool f) { chChanged(f, 0x4000); }
      void channelChanged16(bool f) { chChanged(f, 0x8000); }

      void recordChanged1(bool f)   { rChanged(f, 1);  }
      void recordChanged2(bool f)   { rChanged(f, 2);  }
      void recordChanged3(bool f)   { rChanged(f, 4);  }
      void recordChanged4(bool f)   { rChanged(f, 8);  }
      void recordChanged5(bool f)   { rChanged(f, 16); }
      void recordChanged6(bool f)   { rChanged(f, 32); }
      void recordChanged7(bool f)   { rChanged(f, 64); }

      void thruChanged1(bool f)     { tChanged(f, 1);  }
      void thruChanged2(bool f)     { tChanged(f, 2);  }
      void thruChanged3(bool f)     { tChanged(f, 4);  }
      void thruChanged4(bool f)     { tChanged(f, 8);  }
      void thruChanged5(bool f)     { tChanged(f, 16); }
      void thruChanged6(bool f)     { tChanged(f, 32); }
      void thruChanged7(bool f)     { tChanged(f, 64); }

      void setCtrl1(int);
      void setCtrl2(int);
      void setCtrl3(int);
      void setCtrl4(int);

   public:
      MidiFilterConfig(QDialog* parent=0);
      };

} // namespace MusEGui

#endif


