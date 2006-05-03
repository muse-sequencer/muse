//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: trackinfo.h,v 1.14 2006/01/11 16:14:28 wschweer Exp $
//  (C) Copyright 1999-2005 Werner Schweer (ws@seh.de)
//=========================================================

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

