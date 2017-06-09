//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: wavetrack.cpp,v 1.15.2.12 2009/12/20 05:00:35 terminator356 Exp $
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
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

#include <math.h>

#include "track.h"
#include "event.h"
#include "audio.h"
#include "wave.h"
#include "xml.h"
#include "song.h"
#include "globals.h"
#include "gconfig.h"
#include "al/dsp.h"

//#define WAVETRACK_DEBUG

namespace MusECore {

//---------------------------------------------------------
//   WaveTrack
//---------------------------------------------------------

WaveTrack::WaveTrack() : AudioTrack(Track::WAVE)
{
  setChannels(1);
}

WaveTrack::WaveTrack(const WaveTrack& wt, int flags) : AudioTrack(wt, flags)
{
  internal_assign(wt, flags | Track::ASSIGN_PROPERTIES);
}

void WaveTrack::internal_assign(const Track& t, int flags)
{
      if(t.type() != WAVE)
        return;
      //const WaveTrack& wt = (const WaveTrack&)t;

      const bool dup = flags & ASSIGN_DUPLICATE_PARTS;
      const bool cpy = flags & ASSIGN_COPY_PARTS;
      const bool cln = flags & ASSIGN_CLONE_PARTS;
      if(dup || cpy || cln)
      {
        const PartList* pl = t.cparts();
        for (ciPart ip = pl->begin(); ip != pl->end(); ++ip) {
              Part* spart = ip->second;
              Part* dpart = 0;
              if(dup)
                dpart = spart->hasClones() ? spart->createNewClone() : spart->duplicate();
              else if(cpy)
                dpart = spart->duplicate();
              else if(cln)
                dpart = spart->createNewClone();
              if(dpart)
              {
                dpart->setTrack(this);
                parts()->add(dpart);
              }
              }
      }

}

void WaveTrack::assign(const Track& t, int flags)
{
      AudioTrack::assign(t, flags);
      internal_assign(t, flags);
}

//---------------------------------------------------------
//   fetchData
//    called from prefetch thread
//---------------------------------------------------------

void WaveTrack::fetchData(unsigned pos, unsigned samples, float** bp, bool doSeek, bool overwrite)
      {
      #ifdef WAVETRACK_DEBUG
      printf("WaveTrack::fetchData %s samples:%lu pos:%u overwrite:%d\n", name().toLatin1().constData(), samples, pos, overwrite);
      #endif

      // reset buffer to zero
      if(overwrite)
        for (int i = 0; i < channels(); ++i)
            memset(bp[i], 0, samples * sizeof(float));

      // Process only if track is not off.
      if(!off())
      {
        bool do_overwrite = overwrite;
        PartList* pl = parts();
        unsigned n = samples;
        for (iPart ip = pl->begin(); ip != pl->end(); ++ip) {
              WavePart* part = (WavePart*)(ip->second);
              if (part->mute())
                  continue;

              unsigned p_spos = part->frame();
              unsigned p_epos = p_spos + part->lenFrame();
              if (pos + n < p_spos)
                break;
              if (pos >= p_epos)
                continue;

              for (iEvent ie = part->nonconst_events().begin(); ie != part->nonconst_events().end(); ++ie) {
                    Event& event = ie->second;
                    unsigned e_spos  = event.frame() + p_spos;
                    unsigned nn      = event.lenFrame();
                    unsigned e_epos  = e_spos + nn;

                    if (pos + n < e_spos)
                      break;
                    if (pos >= e_epos)
                      continue;

                    int offset = e_spos - pos;

                    unsigned srcOffset, dstOffset;
                    if (offset > 0) {
                          nn = n - offset;
                          srcOffset = 0;
                          dstOffset = offset;
                          }
                    else {
                          srcOffset = -offset;
                          dstOffset = 0;

                          nn += offset;
                          if (nn > n)
                                nn = n;
                          }
                    float* bpp[channels()];
                    for (int i = 0; i < channels(); ++i)
                          bpp[i] = bp[i] + dstOffset;

                    event.readAudio(part, srcOffset, bpp, channels(), nn, doSeek, do_overwrite);
                    do_overwrite = false;
                    }
              }
      }

      if(overwrite && MusEGlobal::config.useDenormalBias) {
            // add denormal bias to outdata
            for (int i = 0; i < channels(); ++i)
                  for (unsigned int j = 0; j < samples; ++j)
                      bp[i][j] +=MusEGlobal::denormalBias;
            }

      _prefetchFifo.add();
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void WaveTrack::write(int level, Xml& xml) const
      {
      xml.tag(level++, "wavetrack");
      AudioTrack::writeProperties(level, xml);
      const PartList* pl = cparts();
      for (ciPart p = pl->begin(); p != pl->end(); ++p)
            p->second->write(level, xml);
      xml.etag(level, "wavetrack");
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void WaveTrack::read(Xml& xml)
      {
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        goto out_of_WaveTrackRead_forloop;
                  case Xml::TagStart:
                        if (tag == "part") {
                              Part* p = 0;
                              p = Part::readFromXml(xml, this);
                              if(p)
                                parts()->add(p);
                              }
                        else if (AudioTrack::readProperties(xml, tag))
                              xml.unknown("WaveTrack");
                        break;
                  case Xml::Attribut:
                        break;
                  case Xml::TagEnd:
                        if (tag == "wavetrack") {
                              mapRackPluginsToControllers();
                              goto out_of_WaveTrackRead_forloop;
                              }
                  default:
                        break;
                  }
            }
out_of_WaveTrackRead_forloop:
      chainTrackParts(this);
      }

//---------------------------------------------------------
//   newPart
//---------------------------------------------------------

Part* WaveTrack::newPart(Part*p, bool clone)
      {
      WavePart* part = clone ? (WavePart*)p->createNewClone() : (WavePart*)p->duplicate();
      part->setTrack(this);
      return part;
      }

bool WaveTrack::openAllParts()
{
  bool opened = false;
  const PartList* pl = parts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    if(ip->second->openAllEvents())
      opened = true;
  }
  return opened;
}
      
bool WaveTrack::closeAllParts()
{
  bool closed = false;
  const PartList* pl = parts();
  for(ciPart ip = pl->begin(); ip != pl->end(); ++ip)
  {
    if(ip->second->closeAllEvents())
      closed = true;
  }
  return closed;
}

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

bool WaveTrack::getData(unsigned framePos, int dstChannels, unsigned nframe, float** bp)
{
  bool have_data = false;
  
  const bool track_rec_flag = recordFlag();
  const bool track_rec_monitor = recMonitor();        // Separate monitor and record functions.

  if((MusEGlobal::song->bounceTrack != this) && !noInRoute())
  {
    have_data = AudioTrack::getData(framePos, dstChannels, nframe, bp);
    if(have_data && track_rec_flag && MusEGlobal::audio->isRecording() && recFile())
    {
      if(MusEGlobal::audio->freewheel())
      {
      }
      else
      {
        if(fifo.put(dstChannels, nframe, bp, MusEGlobal::audio->pos().frame()))
          printf("WaveTrack::getData(%d, %d, %d): fifo overrun\n", framePos, dstChannels, nframe);
      }
    }
  }

  //---------------------------------------------------
  //    metering
  //---------------------------------------------------

  // If we are RECORD-ARMed (but not monitoring), display the incoming levels on the meters.
  // For technical reasons during PLAYBACK it is very much more difficult to pass a separate,
  //  effected, panned and attenuated signal just to the meters - WITHOUT listening - and a
  //  different signal to the actual audio output - while also showing that signal on the meters.
  // So for consistency we do this in PLAYBACK and STOP.
  // A benefit is the user sees the actual record level, not the post-effect/controlled one.
  // TODO: User-selectable metering points, a mixture of user and automatic here...
  if(have_data && track_rec_flag && !track_rec_monitor)
  {
    const int trackChans = channels();
    int chans = dstChannels;
    if(chans > trackChans)
      chans = trackChans;

    double met[chans];

    // FIXME TODO Need multichannel changes here?
    for(int c = 0; c < chans; ++c)
    {
      met[c] = 0.0;
      float* sp = bp[c];
      for(unsigned k = 0; k < nframe; ++k)
      {
        const double f = fabs(*sp++);
        if(f > met[c])
          met[c] = f;
      }
      if(met[c] > _meter[c])
        _meter[c] = met[c];
      if(_meter[c] > _peak[c])
        _peak[c] = _meter[c];

      if(_meter [c] > 1.0)
          _isClipped[c] = true;
    }
  }

  if(!MusEGlobal::audio->isPlaying())
  {
    if(!have_data || (track_rec_monitor && have_data))
      return have_data;
    return false;
  }

  // If there was no data, or we do not want record monitoring, zero the supplied buffers.
  const bool do_overwrite = !have_data || !track_rec_monitor;

  // Set the return type.
  have_data = true;

  if(MusEGlobal::audio->freewheel())
  {
    // when freewheeling, read data direct from file:
    // Indicate do not seek file before each read.
    fetchData(framePos, nframe, bp, false, do_overwrite);
  }
  else
  {
    unsigned pos;
    float* pf_buf[dstChannels];

    if(_prefetchFifo.get(dstChannels, nframe, pf_buf, &pos))
    {
      printf("WaveTrack::getData(%s) (A) fifo underrun\n", name().toLocal8Bit().constData());
      return have_data;
    }
    if(pos != framePos)
    {
      if(MusEGlobal::debugMsg)
        printf("fifo get error expected %d, got %d\n", framePos, pos);
      while (pos < framePos)
      {
        if(_prefetchFifo.get(dstChannels, nframe, pf_buf, &pos))
        {
          printf("WaveTrack::getData(%s) (B) fifo underrun\n",
              name().toLocal8Bit().constData());
          return have_data;
        }
      }
    }

    if(do_overwrite)
    {
      for(int i = 0; i < dstChannels; ++i)
        AL::dsp->cpy(bp[i], pf_buf[i], nframe, MusEGlobal::config.useDenormalBias);
    }
    else
    {
      for(int i = 0; i < dstChannels; ++i)
        AL::dsp->mix(bp[i], pf_buf[i], nframe);
    }
  }

  return have_data;
}

//---------------------------------------------------------
//   setChannels
//---------------------------------------------------------

void WaveTrack::setChannels(int n)
      {
      AudioTrack::setChannels(n);
      SndFileR sf = recFile();
      if (sf) {
            if (sf->samples() == 0) {
                  sf->remove();
                  sf->setFormat(sf->format(), _channels,
                     sf->samplerate());
                  sf->openWrite();
                  }
            }
      }

} // namespace MusECore
