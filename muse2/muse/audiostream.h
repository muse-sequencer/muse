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
#include <rubberband/RubberBandStretcher.h>

namespace MusECore {

	class AudioStream
	{
		public:
			AudioStream(SndFileR sndfile, int sampling_rate, int out_chans, bool stretching); // the returned sampling rate cannot be changed after creation.
			
			void seek(unsigned frame, fromXTick);
			unsigned readAudio(float** deinterleaved_dest_buffer, int channel, int nFrames, bool overwrite); // returns the number of frames read.
			
			void set_stretch_ratio(double ratio); // 2.0 makes it slower
			void set_pitch_ratio(double ratio); // 2.0 shifts up an octave
		
		private:
			const int update_ratio_every_n_frames = 10000;
			
			void update_stretch_ratio();
			
			unsigned currentPositionInInput, currentPositionInOutput;
			int input_sampling_rate, output_sampling_rate;
			int n_input_channels, n_output_channels;
			bool doStretch;

			SndFileR sndfile;
			SRC_STATE* srcState; // sampling rate converter state
			RubberBand::RubberBandStretcher* stretcher;

			double currentStretchRate;
			double effective_pitch_ratio;
			double effective_stretch_ratio;
			
	};
	
	class StretcherStream
	{
		public:
			StretcherStream(SndFileR* sndfile, bool bypass); // if bypass==true, this Stream will just read the file and return that.
			
			unsigned readAudio(float** deinterleaved_dest_buffer, int channel, int nFrames, bool overwrite); // returns the number of frames read.
				
		private:
			bool bypass;
			double stretch_ratio; // 2.0 will make it slower, 0.5 faster
		
	};
	
} // namespace MusECore

#endif

