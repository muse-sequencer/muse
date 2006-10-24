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

#ifndef __AUDIOTRACK_H__
#define __AUDIOTRACK_H__

#include "fifo.h"
#include "track.h"

class Pipeline;
class SndFile;
class PluginI;
class AuxPluginIF;

//---------------------------------------------------------
//   AudioTrack
//---------------------------------------------------------

class AudioTrack : public Track {
      Q_OBJECT

      bool _prefader;               // prefader metering
      Pipeline* _prePipe;
      Pipeline* _postPipe;
      QList<AuxPluginIF*> _preAux;
      QList<AuxPluginIF*> _postAux;

      void readRecfile(QDomNode);

   protected:
      float* buffer[MAX_CHANNELS];  // this buffer is filled by process()

      bool bufferEmpty;			// set by process() to optimize
      					// data flow

      SndFile* _recFile;
      Fifo fifo;                    // fifo -> _recFile

      virtual bool setMute(bool val);
      virtual bool setOff(bool val);
      virtual bool setSolo(bool val);
      virtual void collectInputData();

   private slots:
      virtual void setAutoRead(bool);
      virtual void setAutoWrite(bool);

   public:
      AudioTrack(TrackType t);
      virtual ~AudioTrack();

      bool readProperties(QDomNode);
      void writeProperties(Xml&) const;

      virtual Part* newPart(Part*p=0, bool clone=false);

      SndFile* recFile() const           { return _recFile; }
      void setRecFile(SndFile* sf)       { _recFile = sf;   }

      virtual void setChannels(int n);

      virtual bool isMute() const;

      void putFifo(int channels, unsigned long n, float** bp);

      void record();
      virtual void startRecording();
      virtual void stopRecording(const AL::Pos&, const AL::Pos&) {}

      bool prefader() const              { return _prefader; }
      void addAuxSend(int n);
      void setPrefader(bool val);
      Pipeline* prePipe()                { return _prePipe;  }
      Pipeline* postPipe()               { return _postPipe;  }

      void addPlugin(PluginI* plugin, int idx, bool pre);
      PluginI* plugin(int idx, bool prefader) const;

      virtual bool hasAuxSend() const { return false; }
      virtual void process();
      void add(int channel,  float**);
      bool copy(int channel, float**);
      bool isBufferEmpty() const { return bufferEmpty; }
      QList<AuxPluginIF*> preAux() const  { return _preAux; }
      QList<AuxPluginIF*> postAux() const { return _postAux; }
      };

#endif
