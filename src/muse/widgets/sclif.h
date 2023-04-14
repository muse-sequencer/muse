//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: sclif.h,v 1.1.1.1 2003/10/27 18:54:33 wschweer Exp $

//    Copyright (C) 1997  Josef Wilgen
//    (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#ifndef __SCALE_IF_H__
#define __SCALE_IF_H__

#include "scldraw.h"

namespace MusEGui {

//---------------------------------------------------------
//   ScaleIf
//---------------------------------------------------------

class ScaleIf
      {
  public:
  enum ScaleType { ScaleLinear, ScaleLog, ScaleDB };

  private:
	bool d_userScale;

  protected:
  ScaleType _scaleType;
	ScaleDraw d_scale;
	int d_maxMajor;
	int d_maxMinor;
  double d_scaleStep;
	bool hasUserScale() {return d_userScale;}
	virtual void scaleChange() = 0;

  public:
	ScaleIf();
      virtual ~ScaleIf() {};
	
  // In log mode, dBFactor sets the dB factor when conversions are done.
  // For example 20 * log10() for signals, 10 * log10() for power, and 40 * log10() for MIDI volume.
  // logFactor sets the scale of the range. For example a MIDI volume control can set a logFactor = 127
  //  so that the range can conveniently be set to 0-127. (With MIDI volume, dBFactor would be
  //  set to 40.0, as per MMA specs.)
  void setScale (double vmin, double vmax,
    ScaleType scaleType = ScaleLinear, double dBFactor = 20.0, double logFactor = 1.0);
  void setScale (double vmin, double vmax, double step,
    ScaleType scaleType = ScaleLinear, double dBFactor = 20.0, double logFactor = 1.0);
  void setScale(const ScaleDiv &s);
  void setScaleMaxMajor( int ticks);
  void setScaleMaxMinor( int ticks);
  void setScaleBackBone(bool v) { d_scale.setBackBone(v); }
  QString specialText() const           { return d_scale.specialText(); }
  void setSpecialText(const QString& s) { d_scale.setSpecialText(s); }
  void autoScale();

  int scaleMaxMinor() const {return d_maxMinor;}
  int scaleMaxMajor() const {return d_maxMinor;}
  double scaleStep() const { return d_scaleStep; }
      };

} // namespace MusEGui

#endif

