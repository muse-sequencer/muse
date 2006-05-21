//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __TRACKINFO_H__
#define __TRACKINFO_H__

class Track;
class MidiTrack;
class AudioOutput;
class AudioInput;
class AudioGroup;
class AudioAux;
class SynthI;
class MidiInPort;
class MidiOutPort;
class MidiChannel;
class WaveTrack;
class TLLineEdit;

//---------------------------------------------------------
//   TrackInfo
//---------------------------------------------------------

class TrackInfo : public QWidget {
      Q_OBJECT

      QLabel* label;
      TLLineEdit* name;

   protected:
      QGridLayout* grid;
      Track* track;

   private slots:
      void nameChanged(QString s);
      void songChanged(int);

   public:
      TrackInfo();
      virtual void init(Track*);
      };

//---------------------------------------------------------
//   MidiTrackInfo
//---------------------------------------------------------

class MidiTrackInfo : public TrackInfo {
      Q_OBJECT

      QSpinBox* transposition;
      QSpinBox* delay;
      QSpinBox* length;
      QSpinBox* velocity;
      QSpinBox* compression;
      QComboBox* channel;
      QPushButton* patch;
      QComboBox* port;
      QComboBox* instrument;
      QMenu* pop;

   private slots:
      void transpositionChanged(int val);
      void velocityChanged(int val);
      void delayChanged(int val);
      void lenChanged(int val);
      void iKomprChanged(int val);
      void patchClicked();
      void instrumentSelected(int);
      void instrumentChanged();
      void autoChanged(Track*,bool);
      void controllerChanged(int);
      void portSelected(int);
      void channelSelected(int);

   public:
      MidiTrackInfo();
      virtual void init(Track*);
      };

//---------------------------------------------------------
//   AudioOutputInfo
//---------------------------------------------------------

class AudioOutputInfo : public TrackInfo {
      Q_OBJECT

   public:
      AudioOutputInfo();
      };

//---------------------------------------------------------
//   AudioInputInfo
//---------------------------------------------------------

class AudioInputInfo : public TrackInfo {
      Q_OBJECT

   public:
      AudioInputInfo();
      };

//---------------------------------------------------------
//   AudioGroupInfo
//---------------------------------------------------------

class AudioGroupInfo : public TrackInfo {
      Q_OBJECT

   public:
      AudioGroupInfo();
      };

//---------------------------------------------------------
//   AudioAuxInfo
//---------------------------------------------------------

class AudioAuxInfo : public TrackInfo {
      Q_OBJECT

   public:
      AudioAuxInfo();
      };

//---------------------------------------------------------
//   WaveTrackInfo
//---------------------------------------------------------

class WaveTrackInfo : public TrackInfo {
      Q_OBJECT

   public:
      WaveTrackInfo();
      };

//---------------------------------------------------------
//   SynthIInfo
//---------------------------------------------------------

class SynthIInfo : public TrackInfo {
      Q_OBJECT

   public:
      SynthIInfo();
      };

//---------------------------------------------------------
//   MidiSynthIInfo
//---------------------------------------------------------

class MidiSynthIInfo : public TrackInfo {
      Q_OBJECT

   public:
      MidiSynthIInfo();
      };

//---------------------------------------------------------
//   MidiOutPortInfo
//---------------------------------------------------------

class MidiOutPortInfo : public TrackInfo {
      Q_OBJECT

      QComboBox* instrument;

   private slots:
      void instrumentSelected(int);
      void instrumentChanged();

   public:
      MidiOutPortInfo();
      virtual void init(Track*);
      };

//---------------------------------------------------------
//   MidiInPortInfo
//---------------------------------------------------------

class MidiInPortInfo : public TrackInfo {
      Q_OBJECT

   public:
      MidiInPortInfo();
      };

//---------------------------------------------------------
//   MidiChannelInfo
//---------------------------------------------------------

class MidiChannelInfo : public TrackInfo {
      Q_OBJECT

      TLLineEdit* portName;
      QComboBox* instrument;
      QPushButton* patch;
      QMenu* pop;

   private slots:
      void instrumentSelected(int);
      void instrumentChanged();
      void controllerChanged(int);
      void patchClicked();

   public:
      MidiChannelInfo();
      virtual void init(Track*);
      };

#endif

