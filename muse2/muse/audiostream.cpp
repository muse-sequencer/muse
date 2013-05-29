//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: audiostream.cpp,v flo93 Exp $
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

#include "audiostream.h"
#include "samplerate.h"
#include <rubberband/RubberBandStretcher.h>
#include <string>
using namespace std;
using namespace RubberBand;

namespace MusECore {

AudioStream::AudioStream(SndFileR sndfile_, int sampling_rate, int out_chans, bool stretching)
{
	sndfile = sndfile_;
	
	output_sampling_rate=sampling_rate;
	n_output_channels=out_chans;
	doStretch=stretching;
	
	n_input_channels = sndfile->channels();
	input_sampling_rate=sndfile->samplerate();
	
	if (!doStretch)
	{
		stretcher=NULL; // not needed
		
		float src_ratio = (double)output_sampling_rate/input_sampling_rate;
		int* error;
		srcState = src_new(SRC_SINC_MEDIUM_QUALITY, n_input_channels, error); // TODO configure this
		if (!srcState) // panic!
		{
			throw string("error creating new sample rate converter");
		}
		
		if (src_set_ratio(srcState, src_ratio))
		{
			throw string("error setting sampling rate ratio");
		}
	}
	else
	{
		srcState = NULL; // not needed
		
		stretcher = new RubberBandStretcher(sampling_rate, n_input_channels, 
					RubberBandStretcher::PresetOption::DefaultOptions, // TODO configure this
					1.0, 1.0); // these values will be overridden anyway soon.
		
		set_pitch_ratio(1.0); // this might call stretcher.setPitch() with something
		set_stretch_ratio(1.0); // different from 1.0, in order to do sampling rate conversion.
	}

	
	currentPositionInInput=0;
	currentPositionInOutput=0;
}

AudioStream::~AudioStream()
{
	if (srcState)
		src_delete(srcState);
	
	if (stretcher)
		delete stretcher;
}

void AudioStream::seek(unsigned int frame, XTick xtick)
{
	unsigned int destFrame; // which frame in the input file to seek to.
	
	if (doStretch) // we're only interested in the xtick
		destFrame = xtickToFrameInFile(xtick);
	else // we're only interested in the frame
		destFrame = frame*input_sampling_rate/output_sampling_rate;
	
	sndfile->seek(destFrame);
	
	currentPositionInInput=destFrame; // that might be a lie if the stretcher still has stuff in behind? flush stretcher?
	currentPositionInOutput=frame;
	
	update_stretch_ratio();
}


/*unsigned int StretcherStream::readAudio(float* dest_buffer, int channel, int nFrames, bool overwrite)
{
	if (!bypass)
	{
		int n_frames_to_read = nFrames / stretch_ratio;
		
		float sndfile_buffer[n_frames_to_read*n_input_channels];
		float result_buffer[nFrames*n_input_channels];
		
		sndfile->readDirect(sndfile_buffer, n_frames_to_read);

		// TODO: do the stretching here!
		float deinterleaved_sndfile_buffer[n_input_channels][n_frames_to_read];
		deinterleave(n_input_channels, sndfile_buffer, deinterleaved_sndfile_buffer, n_frames_to_read);


		stretcher->process(deinterleaved_sndfile_buffer, n_frames_to_read, /*bool final = * /false); // TODO: set final correctly!
		size_t n_frames_to_convert = stretcher->retrieve(deinterleaved_stretched_buffer, n_frames_from_stretcher);
		
	}
	else
	{
		sndfile->readDirect(dest_buffer, n_frames_to_read);
	}

}*/

void AudioStream::set_stretch_ratio(double ratio)
{
	effective_stretch_ratio = ratio * output_sampling_rate / input_sampling_rate;
	stretcher->setTimeRatio(effective_stretch_ratio);
}

void AudioStream::set_pitch_ratio(double ratio)
{
	effective_pitch_ratio = ratio * input_sampling_rate / output_sampling_rate;
	stretcher->setPitchScale(effective_pitch_ratio);
}


unsigned int AudioStream::readAudio(float** deinterleaved_dest_buffer, int channel, int nFrames, bool overwrite)
{
	// convention: _buffers[] are interleaved, and deinterleaved_..._buffers[][] are deinterleaved
	
	/* there are two fundamentally different approaches: when time stretching using the rubberband library
	 * is done, then all sampling rate conversions are done by librubberband as well. given that the current
	 * stretch rate is 1.0, the input sampling rate is 22kHz and the output rate shall be 44kHz, then
	 * we tell rubberband to time-stretch by factor 0.5, and pitch-shift by the same factor. As a result,
	 * rubberband internally probably only pitchshifts, and we have the effect we want.
	 * 
	 * if no time stretching is wished, then we use libsamplerate for sample rate converting.
	 */
	
	if (doStretch)
	{
		float deinterleaved_result_buffer[n_input_channels][nFrames];
		
		size_t n_already_read = 0;
		while (n_already_read < nFrames)
		{
			size_t n_frames_to_read = stretcher->getSamplesRequired();
			float sndfile_buffer[n_frames_to_read*n_input_channels];
			float deinterleaved_sndfile_buffer[n_input_channels][n_frames_to_read];
			
			sndfile->readDirect(sndfile_buffer, n_frames_to_read);
			deinterleave(n_input_channels, sndfile_buffer, deinterleaved_sndfile_buffer, n_frames_to_read);
			currentPositionInInput+=n_frames_to_read;
			
			stretcher->process(deinterleaved_sndfile_buffer, n_frames_to_read, /*bool final = */false); // TODO: set final correctly!
			size_t n_frames_retrieved;
			
			n_frames_retrieved = stretcher->retrieve(deinterleaved_result_buffer+n_already_read, nFrames-n_already_read);
			
			n_already_read += n_frames_retrieved;
		}
		
		copy_and_adjust_channels(n_input_channels, n_output_channels, deinterleaved_result_buffer, deinterleaved_dest_buffer, nFrames, overwrite);
		
		/*
		int sndfile_buffer_nframes = nFrames / effective_stretch_ratio;
		float sndfile_buffer[sndfile_buffer_nframes*n_input_channels];
		float deinterleaved_sndfile_buffer[n_input_channels][sndfile_buffer_nframes];
		float result_buffer[nFrames*n_input_channels];
		
		
		int n_frames_to_read = sndfile_buffer_nframes;
		sndfile->readDirect(sndfile_buffer, n_frames_to_read);
		deinterleave(n_input_channels, sndfile_buffer, deinterleaved_sndfile_buffer, n_frames_to_read);
		
		stretcher->process(deinterleaved_sndfile_buffer, n_frames_to_read, /*bool final = * /false); // TODO: set final correctly!
		size_t n_frames_retrieved = stretcher->retrieve(deinterleaved_dest_buffer, nFrames);
		size_t n_total_frames = n_frames_retrieved;
		
		while (n_total_frames < nFrames)
		{
			n_frames_to_read = stretcher->getSamplesRequired();
			if (n_frames_to_read > sndfile_buffer_nframes)
			{
				printf("DEBUG: uh, stretcher wants more *additional* samples than should have been neccessary in the first place\n");
				n_frames_to_read = sndfile_buffer_nframes;
			}
			
			sndfile->readDirect(sndfile_buffer, n_frames_to_read);
			deinterleave(n_input_channels, sndfile_buffer, deinterleaved_sndfile_buffer, n_frames_to_read);
			
			stretcher->process(deinterleaved_sndfile_buffer, n_frames_to_read, /*bool final = * /false); // TODO: set final correctly!
			ssize_t n_frames_retrieved = stretcher->retrieve(deinterleaved_dest_buffer+n_total_frames, nFrames-n_total_frames);
			
			n_total_frames+=n_frames_retrieved;
		}*/
		
	}
	else
	{
		int n_frames_to_read = nFrames * input_sampling_rate / output_sampling_rate;
		float sndfile_buffer[n_frames_to_read*n_input_channels];
		float result_buffer[nFrames*n_input_channels];
		
		sndfile->readDirect(sndfile_buffer, n_frames_to_read);
		currentPositionInInput+=n_frames_to_read;

		SRC_DATA src_data;
		src_data.input_frames=n_frames_to_read;
		src_data.output_frames=nFrames;
		src_data.src_ratio = (double)output_sampling_rate/input_sampling_rate;
		src_data.data_in = sndfile_buffer;
		src_data.data_out = result_buffer;
		src_process(srcState, src_data);
		
		if (src_data.input_frames_used != src_data.input_frames) // TODO
			printf("THIS IS AN ERROR (which was thought to not happen): src_data.input_frames_used = %i != src_data.input_frames = %i!\n",  src_data.input_frames_used , src_data.input_frames);
		
		int n_frames_read = src_data.output_frames_gen;
		while(n_frames_read < nFrames)
		{
			sndfile->readDirect(sndfile_buffer, 1);
			currentPositionInInput++;
			
			src_data.input_frames=1;
			src_data.output_frames=nFrames-n_frames_read;
			src_data.data_in=sndfile_buffer;
			src_data.data_out=result_buffer+n_frames_read;
			src_process(srcState, src_data);

			if (src_data.input_frames_used != src_data.input_frames)
				printf("THIS IS AN ERROR (which was thought to not happen): src_data.input_frames_used = %i != src_data.input_frames = %i!\n",  src_data.input_frames_used , src_data.input_frames);
			
			n_frames_read+=src_data.output_frames_gen;
		}
		
		deinterleave_and_adjust_channels(n_input_channels, n_output_channels, result_buffer, deinterleaved_dest_buffer, nFrames, overwrite);
	}
	
	currentPositionInOutput+=nFrames;
	maybe_update_stretch_ratio();
}

/*unsigned int AudioStream::readAudio(float** deinterleaved_dest_buffer, int channel, int nFrames, bool overwrite)
{
	// convention: _buffers[] are interleaved, and deinterleaved_..._buffers[][] are deinterleaved
	
	int n_frames_to_read = nFrames * input_sampling_rate / output_sampling_rate; // TODO: if doStretch: then funny calculations!
	float sndfile_buffer[n_frames_to_read*n_input_channels];
	float result_buffer[nFrames*n_input_channels];
	
	sndfile->readDirect(sndfile_buffer, n_frames_to_read);
	
	float* stretched_buffer;
	if (doStretch)
	{
		// TODO: do the stretching here!
		float deinterleaved_sndfile_buffer[n_input_channels][n_frames_to_read];
		deinterleave(n_input_channels, sndfile_buffer, deinterleaved_sndfile_buffer, n_frames_to_read);


		int n_frames_from_stretcher = 42; // TODO: funny calculations
		
		stretcher->process(deinterleaved_sndfile_buffer, n_frames_to_read, /*bool final = * /false); // TODO: set final correctly!
		size_t n_frames_to_convert = stretcher->retrieve(deinterleaved_stretched_buffer, n_frames_from_stretcher);
		
	}
	else
	{
		stretched_buffer = sndfile_buffer; // shallow copy
	}
	// TODO: loop the above, adjust the n_frames_to_read
	
	SRC_DATA src_data;
	src_data.input_frames=n_frames_to_read;
	src_data.output_frames=nFrames;
	src_data.src_ratio = (double)output_sampling_rate/input_sampling_rate;
	src_data.data_in = sndfile_buffer;
	src_data.data_out = result_buffer;
	src_process(srcState, src_data);
	
	// TODO: handle input_frames_used < input_frames!
	
	int n_frames_read = src_data.output_frames_gen;
	while(nFrames>n_frames_read)
	{
		sndfile->readDirect(sndfile_buffer, 1);
		src_data.input_frames=1;
		src_data.output_frames=nFrames-n_frames_read;
		src_data.data_in=sndfile_buffer;
		src_data.data_out=result_buffer+n_frames_read;
		src_process(srcState, src_data);
		
		n_frames_read+=src_data.output_frames_gen;
	}
	
	deinterleave_and_adjust_channels(n_input_channels, n_output_channels, result_buffer, deinterleaved_dest_buffer, n_frames, overwrite);
}*/


void AudioStream::maybe_update_stretch_ratio()
{
	// TODO
	update_stretch_ratio();
}
void AudioStream::update_stretch_ratio()
{
	if (doStretch)
	{
		XTick keyframe_xtick = frameToxtick(currentPositionInOutput) + XTick(386);
		
		unsigned keyframe_pos_in_stream = xtickToFrame(keyframe_xtick);
		unsigned keyframe_pos_in_file = xtickToFrameInFile(keyframe_xtick);
		
		// so we must play the file's frames from currentPositionInInput to keyframe_pos_in_file
		// within (keyframe_pos_in_stream-currentPositionInOutput) frames.
		
		double stretch_ratio = (double)(keyframe_pos_in_stream-currentPositionInOutput)/(keyframe_pos_in_file-currentPositionInInput);
		set_stretch_ratio(stretch_ratio);
	}
}


} // namespace MusECore
