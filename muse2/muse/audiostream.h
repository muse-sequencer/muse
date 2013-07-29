//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audiostream.h,v flo93 Exp $
//
//  (C) Copyright 2013 Florian Jung (florian.a.jung@web.de)
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

#ifndef __AUDIOSTREAM_H__
#define __AUDIOSTREAM_H__

#include "wave.h"
#include "samplerate.h"
#include "tempo.h"
#include "config.h"

#ifdef RUBBERBAND_SUPPORT
  #include <rubberband/RubberBandStretcher.h>
#endif

namespace MusECore {

	class AudioStream // TODO findmich handle and replace Event.spos
	{
		public:
			enum stretch_mode_t
			{
				NO_STRETCHING,
				DO_STRETCHING,
				NAIVE_STRETCHING // not implemented yet, TODO
			};
			

			/* After constructing a new AudioStream, you MUST check isGood(). If not yourStream.isGood(), then the
			   object is in an undefined state and will not work. Delete it, then. */
			AudioStream(QString filename, int sampling_rate, int out_chans, stretch_mode_t stretch_mode, // the sampling rate cannot be changed after creation.
			            XTick startXtick, unsigned startFrame);
			~AudioStream();
			
			void seek(unsigned frame, XTick xtick); // which output-frame / xtick relative to the beginning of the AudioStream to seek to
			unsigned readAudio(float** deinterleaved_dest_buffer, int nFrames, bool overwrite); // returns the number of frames read.
			
			XTick relFrame2XTick(unsigned frame) const;
			XTick relFrameInFile2XTick(unsigned frame) const;
			unsigned relTick2Frame(XTick xtick) const;        // those are relative to the AudioStream's beginning.
			unsigned relTick2FrameInFile(XTick xtick) const;

			int get_n_output_channels() const { return n_output_channels; }
			bool isGood() const { return !initalisation_failed; }
			
			stretch_mode_t getStretchMode() const { return doStretch ? DO_STRETCHING : NO_STRETCHING; }
			
			void readPeakRms(SampleV* s, int mag, unsigned pos, bool overwrite = true) const;
			
		private:
			void maybe_update_stretch_ratio();
			void update_stretch_ratio();

#ifdef RUBBERBAND_SUPPORT			
			void set_stretch_ratio(double ratio); // 2.0 makes it slower
			void set_pitch_ratio(double ratio); // 2.0 shifts up an octave
#endif
			
			
			
			bool initalisation_failed;
			unsigned currentPositionInInput, currentPositionInOutput;
			int input_sampling_rate, output_sampling_rate;
			int n_input_channels, n_output_channels;
			bool doStretch;

			SndFile* sndfile;
			SRC_STATE* srcState; // sampling rate converter state
#ifdef RUBBERBAND_SUPPORT
			RubberBand::RubberBandStretcher* stretcher;
			double currentStretchRate;       // these are pretty useless when RUBBERBAND_SUPPORT
			double effective_pitch_ratio;    // is disabled. but for the sake of code-simplicity
			double effective_stretch_ratio;  // i'll just let here, doesn't waste too much.
#endif
			
			
			unsigned frameStartInSong;  // TODO, remove this, or externalTempoMap!
			XTick xtickStartInSong;     // TODO, remove this, or externalTempoMap!
			//TempoList externalTempoMap; // this is a copy of the MusEGlobal::tempomap, however stored with an xtick-offset of -PositionInSong
			TempoList fileTempoMap;     // this maps which xtick corresponds to which frame in the audio file
	};
	

} // namespace MusECore

#endif

