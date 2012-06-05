//=========================================================
//  MusE
//  Linux Music Editor
//
//  midi_audio_control.h
//  Copyright (C) 2012 by Tim E. Real (terminator356 at users.sourceforge.net)
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
#ifndef MIDI_AUDIO_CONTROL_H
#define MIDI_AUDIO_CONTROL_H

#include "ui_midi_audio_control_base.h"

namespace MusEGui {

class MidiAudioControl : public QDialog, public Ui::MidiAudioControlBase
{
    Q_OBJECT

private:
    int _port, _chan, _ctrl;
    bool _is_learning;
    void update();
    void resetLearn();
    void updateCtrlBoxes();

private slots:
    void heartbeat();
    void learnChanged(bool);
    void portChanged(int);
    void chanChanged();
    void ctrlTypeChanged(int);
    void ctrlHChanged();
    void ctrlLChanged();
    void configChanged();

public:
    MidiAudioControl(int port = -1, int chan = 0, int ctrl = 0, QWidget* parent = 0);
    int port() const { return _port; }
    int chan() const { return _chan; }
    int ctrl() const { return _ctrl; }
};

}

#endif // MIDI_AUDIO_CONTROL_H
