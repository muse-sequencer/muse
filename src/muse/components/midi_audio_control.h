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

#include <QMetaObject>

#include "mpevent.h"
#include "type_defs.h"

class QWidget;
class QComboBox;
class QSpinBox;
class QCheckBox;

namespace MusEGui {

class MidiAudioControl : public QDialog, public Ui::MidiAudioControlBase
{
    Q_OBJECT

private:
    QMetaObject::Connection _configChangedConn;
    QMetaObject::Connection _learnReceivedConn;

    int _port, _chan, _ctrl;
    bool _enableAssignType;
    bool _assignToSong;
    void updateDialog();
    void resetLearn();
    void updateCtrlBoxes();

    void selectPort(QComboBox *cb, int port);
    void selectCtrl(QComboBox *typecb, QSpinBox *hisb, QSpinBox *losb, int ctrl);

    void configChanged();
    void learnChanged(bool);
    void portChanged(int);
    void chanChanged();
    void ctrlTypeChanged(int);
    void ctrlHChanged();
    void ctrlLChanged();
    void assignTrackTriggered();
    void assignSongTriggered();

    void midiLearnReceived(const MusECore::MidiRecordEvent&);
    void assignLearnCC(const MusECore::MidiRecordEvent&,
      QCheckBox *ccEn, QComboBox *ccPort, QComboBox *ccChan, QSpinBox *ccNum);

public:
    MidiAudioControl(bool enableAssignType = false, bool assignToSong = false, int port = -1, int chan = 0, int ctrl = 0, QWidget* parent = 0);
    ~MidiAudioControl();

    int port() const;
    int chan() const;
    int ctrl() const;
    bool enableAssignType() const;
    bool assignToSong() const;
};

}

#endif // MIDI_AUDIO_CONTROL_H
