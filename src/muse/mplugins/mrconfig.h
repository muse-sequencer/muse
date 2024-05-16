//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mrconfig.h,v 1.1.1.1 2003/10/27 18:52:43 wschweer Exp $
//
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

#ifndef __MRCONFIG_H__
#define __MRCONFIG_H__

#include <QMetaObject>

#include "mpevent.h"
#include "midiremote.h"
#include "type_defs.h"

#include "ui_mrconfigbase.h"

class QCloseEvent;
class QShowEvent;
class QWidget;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;

namespace MusEGui {

class PitchEdit;

//---------------------------------------------------------
//   MRConfig
//---------------------------------------------------------

class MRConfig : public QWidget, public Ui::MRConfigBase {
      Q_OBJECT

      QMetaObject::Connection _songChangedConn;
      QMetaObject::Connection _configChangedConn;
      QMetaObject::Connection _learnReceivedConn;
      MusECore::MidiRemote *_curMidiRemote;

      virtual void closeEvent(QCloseEvent*);
      virtual void showEvent(QShowEvent*);
      void setupPortList(QComboBox *cb, int curPort);
      void setupChannelList(QComboBox *cb, int curChan);
      void setupCCNumList(QSpinBox *sb, int curCCNum);
      void setupValTypeList(QComboBox *cb, int curCCValType);

      void selectPort(QComboBox *cb, int port);
      void selectChannel(QComboBox *cb, int chan);

      void settingChanged();
      void learnChanged(QPushButton *pb, bool newval);
      void switchSettings();

      void resetPressed();
      void copyPressed();

      void songChanged(MusECore::SongChangedStruct_t type);
      void configChanged();
      void midiLearnReceived(const MusECore::MidiRecordEvent&);

      void apply();
      void accept();
      void reject();

      void updateDialog();
      void updateValues();

      void clearLearnSettings() const;
      void assignLearnNote(const MusECore::MidiRecordEvent&,
        QCheckBox *noteEn, QComboBox *notePort, QComboBox *noteChan, PitchEdit *notePitch);
      void assignLearnCC(const MusECore::MidiRecordEvent&,
        QCheckBox *ccEn, QComboBox *ccPort, QComboBox *ccChan, QSpinBox *ccNum);

   signals:
      void hideWindow();

   public:
      MRConfig(QWidget* parent=0, Qt::WindowFlags fl = Qt::Widget);
      ~MRConfig();
      };

} // namespace MusEGui

#endif

