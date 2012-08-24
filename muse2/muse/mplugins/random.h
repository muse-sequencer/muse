//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: random.h,v 1.1.1.1 2003/10/27 18:52:43 wschweer Exp $
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
//
//  This code is an adaption of the random rhythm generator taken 
//  from "The JAZZ++ Midi Sequencer"
//  Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all 
//  rights reserved.
//  Distributed under the GNU General Public License
//=========================================================

#if 0
#ifndef random_h
#define random_h

#ifndef wx_wxh
#include "wx.h"
#endif

#ifndef _FSTREAM_H
#include <fstream.h>
#endif

#ifndef dynarray_h
#include "dynarray.h"
#endif

// gcc > 2.7.2 does not have ACG anymore?
#define USE_ACG 0

#if USE_ACG
#include <ACG.h>	// random generator from libg++
extern ACG rnd;

#else

class tRandomGenerator
{
  public:
    double asDouble();
};
extern tRandomGenerator rnd;

#endif


#undef min
#undef max


// array of probabilities

class tRndArray
{
  friend class tArrayEdit;
  protected:
    tIntArray array;
    int n;	// number of elements in array
    int nul, min, max;

  public:
    int Null()				{ return nul; }
    void SetNull(int n)			{ nul = n; }
    tRndArray(int n, int min, int max);
    tRndArray & operator = (const tRndArray &);
    tRndArray(tRndArray const &);

    virtual ~tRndArray();
    int &operator[] (int i) 		{ return array[i]; }
    int  operator[] (int i) const 	{ return array[i]; }
#ifdef FOR_MSW
    double operator[](double f);
    float operator[](float f) {
#else
    double operator[](double f) const;
    float operator[](float f) const {
#endif
      return (float)operator[]((double)f);
    }
    int Size() const 			{ return n; }
    int Min() const			{ return min; }
    int Max() const			{ return max; }
    void SetMinMax(int min, int max);
    void Resize(int nn) 		{ n = nn; }

    friend ostream & operator << (ostream &, tRndArray const &);
    friend istream & operator >> (istream &, tRndArray &);

    int Random();	// returns index 0..n-1 (arrayvalues -> empiric distribution)
    int Random(double rndval);	// returns index 0..n-1 (arrayvalues -> empiric distribution)
    int Random(int i);  // return 0/1
    int Interval(int seed);

    void SetUnion(tRndArray &o, int fuzz);
    void SetDifference(tRndArray &o, int fuzz);
    void SetIntersection(tRndArray &o, int fuzz);
    void SetInverse(int fuzz);
    int Fuzz(int fuzz, int v1, int v2) const;
    void Clear();
};


#define ARED_GAP	1
#define ARED_XTICKS 	2
#define ARED_YTICKS	4
#define ARED_MINMAX	8
#define ARED_RHYTHM	16
#define ARED_BLOCKS     32
#define ARED_LINES      64


class tArrayEditDrawBars {
  public:
    virtual void DrawBars() = 0;
};


class tArrayEdit : public wxCanvas
{
protected:
  // paint position
  long x, y, w, h, ynul;
  void DrawBar(int i, int Qt::black);

  int dragging;		// Dragging-Event valid
  int index;		// ctrl down: drag this one

  tRndArray &array;
  int &n, &min, &max, &nul;	// shorthand for array.n, array.min, ...
  char *label;
  tArrayEditDrawBars *draw_bars;

  // array size is mapped to this range for x-tick marks
  int xmin, xmax;

  virtual void DrawXTicks();
  virtual void DrawYTicks();
  virtual void DrawLabel();
  virtual void DrawNull();
  int  Index(wxMouseEvent &e);

  int  enabled;
  int  style_bits;

  virtual const char *GetXText(int xval);  // Text for x-tickmarks
  virtual const char *GetYText(int yval);  // Text for y-tickmarks


public:
  tArrayEdit(wxFrame *parent, tRndArray &array, long xx, long yy, long ww, long hh, int style_bits = (ARED_GAP | ARED_XTICKS));
  virtual ~tArrayEdit();

  virtual void OnPaint();
  virtual void OnSize(int ww, int hh);
  virtual void OnEvent(wxMouseEvent &e);
  virtual int Dragging(wxMouseEvent &);
  virtual int ButtonDown(wxMouseEvent &);
  virtual int ButtonUp(wxMouseEvent &);

  virtual void SetLabel(char const *llabel);
  void Enable(int enable = 1);
  void SetStyle(int style) { style_bits = style; }
  // min and max value in array (both values inclusive)
  void SetYMinMax(int min, int max);
  // for display x-axis only, does not resize the array (both values inclusive)
  void SetXMinMax(int xmin, int xmax);
  void DrawBarLine (long xx);
  void SetDrawBars(tArrayEditDrawBars *x) { draw_bars = x; }
  void Init() {}
};



class tRhyArrayEdit : public tArrayEdit
{
    int steps_per_count;
    int count_per_bar;
    int n_bars;
  protected:
    virtual void DrawXTicks();
  public:
    tRhyArrayEdit(wxFrame *parent, tRndArray &array, long xx, long yy, long ww, long hh, int style_bits = (ARED_GAP | ARED_XTICKS | ARED_RHYTHM));
    void SetMeter(int steps_per_count, int count_per_bar, int n_bars);
};


#endif
#endif

