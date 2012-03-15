//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2010 Werner Schweer and others (ws@seh.de)
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

#ifndef __MTRACKINFO_H__
#define __MTRACKINFO_H__

#include "ui_mtrackinfobase.h"

namespace MusECore {
class Track;
}

namespace MusEGui {

//---------------------------------------------------------
//   MidiTrackInfo
//---------------------------------------------------------

class MidiTrackInfo : public QWidget, public Ui::MidiTrackInfoBase 
{
      Q_OBJECT
      MusECore::Track* selected;
      bool _midiDetect;
      int program, pan, volume;
      int heartBeatCounter;
      
    protected:
      virtual void resizeEvent(QResizeEvent*);
  
    private slots:
      void iOutputChannelChanged(int);
      void iOutputPortChanged(int);
      void iProgHBankChanged();
      void iProgLBankChanged();
      void iProgramChanged();
      void iProgramDoubleClicked();
      void iLautstChanged(int);
      void iLautstDoubleClicked();
      void iTranspChanged(int);
      void iAnschlChanged(int);
      void iVerzChanged(int);
      void iLenChanged(int);
      void iKomprChanged(int);
      void iPanChanged(int);
      void iPanDoubleClicked();
      void instrPopup();
      void recordClicked();
      void progRecClicked();
      void volRecClicked();
      void panRecClicked();
      void recEchoToggled(bool);
      void inRoutesPressed();
      void outRoutesPressed();
      void instrPopupActivated(QAction*);
      
   protected slots:
      virtual void heartBeat();

   public slots:
      void setTrack(MusECore::Track*); 
      void configChanged();
      void songChanged(int);
   
   signals:
      void returnPressed();
      void escapePressed();
      
   public:
      MidiTrackInfo(QWidget*, MusECore::Track* = 0);
      MusECore::Track* track() const { return selected; }
      void setLabelText();
      void setLabelFont();
      void updateTrackInfo(int);
};

} // namespace MusEGui

#endif

