//=============================================================================
//  AL
//  Audio Utility Library
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

#ifndef __DSP_H__
#define __DSP_H__

namespace AL {


//---------------------------------------------------------
//   f_max
//---------------------------------------------------------

static inline float f_max(float x, float a)
      {
      x -= a;
      x += fabsf(x);
      x *= 0.5f;
      x += a;
      return x;
      }

//---------------------------------------------------------
//   Dsp
//    standard version of all dsp routines without any
//    hw acceleration
//---------------------------------------------------------

class Dsp {
   public:
      Dsp() {}
      virtual ~Dsp() {}

      virtual float peak(float* buf, unsigned n, float current) {
            for (unsigned i = 0; i < n; ++i)
                  current = f_max(current, fabsf(buf[i]));
            return current;
            }
      virtual void applyGainToBuffer(float* buf, unsigned n, float gain) {
            for (unsigned i = 0; i < n; ++i)
                  buf[i] *= gain;
            }
      virtual void mixWithGain(float* dst, float* src, unsigned n, float gain) {
            for (unsigned i = 0; i < n; ++i)
                  dst[i] += src[i] * gain;
            }
      virtual void mix(float* dst, float* src, unsigned n) {
            for (unsigned i = 0; i < n; ++i)
                  dst[i] += src[i];
            }
      };

extern void initDsp();
extern Dsp* dsp;

}

#endif

