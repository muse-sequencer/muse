//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: random.cpp,v 1.1.1.1 2003/10/27 18:52:39 wschweer Exp $
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
#include "random.h"
#include "util.h"
#include <assert.h>


#if USE_ACG
ACG rnd(0, 55);
#else
#include <stdlib.h>
double tRandomGenerator::asDouble()
{
  return double(rand()) / double(RAND_MAX);
}
tRandomGenerator rnd;
#endif


// Array of probabilities

tRndArray::tRndArray(int nn, int mmin, int mmax)
{
  int i;
  n = nn;
  for (i = 0; i < n; i++)
    array[i] = mmin;
  min = mmin;
  max = mmax;
  nul = min > 0 ? min : 0;
}


void tRndArray::SetMinMax(int mi, int ma)
{
  min = mi;
  max = ma;
  nul = min > 0 ? min : 0;
  for (int i = 0; i < array.GetSize(); i++)
  {
    if (array[i] < min)
      array[i] = min;
    else if (array[i] > max)
      array[i] = max;
  }
}

tRndArray::~tRndArray()
{
}

#ifdef FOR_MSW
double tRndArray::operator[](double f)
#else
double tRndArray::operator[](double f) const
#endif
{
  int i = (int)f;
  if (i < 0)
    i = 0;
  else if (i >= n - 2)
    i = n - 2;
  tMapper map(i, i+1, array[i], array[i+1]);
  return map(f);
}


tRndArray & tRndArray::operator = (const tRndArray &o)
{
  if (this == &o)
    return *this;

  array = o.array;
  n = o.n;
  min = o.min;
  max = o.max;
  nul = o.nul;
  return *this;
}


tRndArray::tRndArray(const tRndArray &o)
  : array(o.array)
{
  n = o.n;
  min = o.min;
  max = o.max;
  nul = o.nul;
}


int tRndArray::Random()
{
  return Random(rnd.asDouble());
}

int tRndArray::Random(double rndval)
{
  double sum, dec;
  int i;

  assert(n > 0);

  sum = 0.0;
  for (i = 0; i < n; i++)
  {
    assert(array[i] >= 0);
    sum += array[i];
  }
  if (sum <= 0)
    return 0;

  dec = sum * rndval * 0.99999;
  assert(dec < sum);

  i = 0;
  while (dec >= 0.0)
  {
    dec -= array[i];
    i++;
  }
  i--;

  assert(i >= 0 && i < n);
  return i;
}


int tRndArray::Interval(int seed)
{
  if (seed < 0)		// initial ?
    seed = int(rnd.asDouble() * n);
  int delta = Random();
  if (rnd.asDouble() < 0.5)
    delta = -delta;
  seed = (seed + n + delta) % n;
  return seed;
}

int tRndArray::Random(int i)
{
  return rnd.asDouble() * (max - min) < array[i];
}


void tRndArray::SetUnion(tRndArray &o, int fuzz)
{
  for (int i = 0; i < n; i++)
  {
    int val = array[i];
    if (o.array[i] > val)
      val = o.array[i];
    array[i] = Fuzz(fuzz, array[i], val);
  }
}


void tRndArray::SetIntersection(tRndArray &o, int fuzz)
{
  for (int i = 0; i < n; i++)
  {
    int val = array[i];
    if (o.array[i] < val)
      val = o.array[i];
    array[i] = Fuzz(fuzz, array[i], val);
  }
}


void tRndArray::SetDifference(tRndArray &o, int fuzz)
{
  tRndArray tmp(o);
  tmp.SetInverse(tmp.Max());
  SetIntersection(tmp, fuzz);
}


void tRndArray::SetInverse(int fuzz)
{
  for (int i = 0; i < n; i++)
    array[i] = Fuzz(fuzz, array[i], min + max - array[i]);
}


int tRndArray::Fuzz(int fuz, int v1, int v2) const
{
  // interpolate between v1 and v2
  return (fuz - min) * v2 / (max - min) + (max - fuz) * v1 / (max - min);
}


void tRndArray::Clear()
{
  for (int i = 0; i < n; i++)
    array[i] = min;
}


ostream & operator << (ostream &os, tRndArray const &a)
{
  int i;

  os << a.n << " " << a.min << " " << a.max << endl;
  for (i = 0; i < a.n; i++)
    os << a.array[i] << " ";
  os << endl;
  return os;
}


istream & operator >> (istream &is, tRndArray &a)
{
  int i;
  is >> a.n >> a.min >> a.max;
  for (i = 0; i < a.n; i++)
    is >> a.array[i];
  return is;
}


// --------------------------------- tArrayEdit -------------------------------------

// length of tickmark line
#define TICK_LINE 0

tArrayEdit::tArrayEdit(wxFrame *frame, tRndArray &ar, long xx, long yy, long ww, long hh, int sty)
  : wxCanvas(frame, xx, yy, ww, hh),
    array(ar),
    n(ar.n),
    min(ar.min),
    max(ar.max),
    nul(ar.nul)
{
  draw_bars = 0;
  enabled = 1;
  dragging = 0;
  index = -1;
  label = 0;
  style_bits = sty;

  xmin = 0;
  xmax = n;

  x = 0;	// draw to topleft corner of canvas
  y = 0;
  w = ww;
  h = hh;

  float tw, th;
  wxDC *dc = GetDC();
  dc->SetFont(wxSMALL_FONT);
  dc->GetTextExtent("123", &tw, &th);
  if (style_bits & ARED_XTICKS)
  {
    // leave space for bottomline
    h -= (int)th;
  }

  if (style_bits & (ARED_MINMAX | ARED_YTICKS))
  {
    // leave space to display min / max
    x = (int)(tw + TICK_LINE);
    w -= (int)(tw + TICK_LINE);
  }

  ynul = y + h - h * (nul - min) / (max - min);
}


void tArrayEdit::OnSize(int ww, int hh)
{
  w = ww;
  h = hh;
  wxCanvas::OnSize(w, h);
  float tw, th;
  GetDC()->GetTextExtent("123", &tw, &th);
  if (style_bits & ARED_XTICKS)
    h -= (int)th;
  if (style_bits & (ARED_MINMAX | ARED_YTICKS))
  {
    x = (int)(tw + TICK_LINE);
    w -= (int)(tw + TICK_LINE);
  }
  ynul = y + h - h * (nul - min) / (max - min);
}

tArrayEdit::~tArrayEdit()
{
  delete [] label;
}

void tArrayEdit::DrawBar(int i, int Qt::black)
{
  wxDC *dc = GetDC();

  if (style_bits & ARED_LINES)
  {
    if (!Qt::black)
      dc->SetPen(wxWHITE_PEN);

    tMapper xmap(0, n, 0, w);
    tMapper ymap(min, max, h, 0);

    float x1 = (float)xmap(i + 0.5);
    float y1 = (float)ymap(array[i]);
    if (i > 0)
    {
      // draw line to prev position
      float x0 = (float)xmap(i - 0.5);
      float y0 = (float)ymap(array[i-1]);
      dc->DrawLine(x0, y0, x1, y1);
    }
    if (i < n-1)
    {
      // draw line to next position
      float x2 = (float)xmap(i + 1.5);
      float y2 = (float)ymap(array[i+1]);
      dc->DrawLine(x1, y1, x2, y2);
    }

    if (!Qt::black)
      dc->SetPen(wxBLACK_PEN);
    return;
  }

  int gap = 0;
  if (style_bits & ARED_GAP)
  {
    gap = w / n / 6;
    if (!gap && w / n > 3)
      gap = 1;
  }
  long xbar, ybar, wbar, hbar;

  wbar = w / n - 2 * gap;
  xbar = x + i * w / n + gap;
  hbar = h * (array[i] - nul) / (max - min);

  if (style_bits & ARED_BLOCKS)
  {
    /*
    ybar = ynul - hbar;
    if (hbar < 0)
      hbar = -hbar;
    hbar = (hbar < 2) ? hbar : 2;
    */
    int hblk = 12;

    ybar = ynul - hbar - hblk/2;
    hbar = hblk;
    if (ybar < y) {
      int d = y - ybar;
      ybar += d;
      hbar -= d;
    }
    if (ybar + hbar > y + h) {
      int d = (ybar + hbar) - (y + h);
      hbar -= d;
    }
    if (hbar < 2)
      hbar = 2;
  }
  else

  if (hbar < 0)
  {
    ybar = ynul;
    hbar = -hbar;
  }
  else
    ybar = ynul - hbar;

  if (ybar == y)
    ++ybar, --hbar;

  if (!Qt::black)
  {
    dc->SetBrush(wxWHITE_BRUSH);
    dc->SetPen(wxWHITE_PEN);
  }
  if (wbar && hbar)
    dc->DrawRectangle(xbar, ybar, wbar, hbar);
  if (!Qt::black)
  {
    dc->SetBrush(wxBLACK_BRUSH);
    dc->SetPen(wxBLACK_PEN);
  }
}

const char *tArrayEdit::GetXText(int xval)
{
  static char buf[8];
  sprintf(buf, "%d", xval);
  return buf;
}

const char *tArrayEdit::GetYText(int yval)
{
  static char buf[8];
  sprintf(buf, "%d", yval);
  return buf;
}

void tArrayEdit::DrawXTicks()
{
  float tw, th;

  if (!(style_bits & ARED_XTICKS))
    return;

  wxDC *dc = GetDC();
  dc->SetFont(wxSMALL_FONT);

  // compute tickmark x-distance
  dc->GetTextExtent("-123", &tw, &th);
  int max_labels = (int)(w / (tw + tw/2));
  if (max_labels > 0)
  {
    int step = (xmax - xmin + 1) / max_labels;
    if (step <= 0)
      step = 1;
    for (int val = xmin; val <= xmax; val += step)
    {
      const char *buf = GetXText(val);
      //sprintf(buf, "%d", val);
      dc->GetTextExtent((char *)buf, &tw, &th);
      float yy = y + h;
      float xx = x + w * (val - xmin) / (xmax - xmin + 1);
      xx -= tw/2;        // center text
      xx += 0.5 * w / n; // middle of bar
      dc->DrawText(buf, xx, yy);
      //dc->DrawLine(x - TICK_LINE, yy, x, yy);
    }
  }

  dc->SetFont(wxNORMAL_FONT);
}


void tArrayEdit::DrawYTicks()
{
  wxDC *dc = GetDC();
  dc->SetFont(wxSMALL_FONT);

  if (style_bits & ARED_YTICKS)
  {
    // compute tickmark y-distance
    float tw, th;
    dc->GetTextExtent("-123", &tw, &th);
    int max_labels = (int)(h / (th + th/2));
    if (max_labels > 0)
    {
      int step = (max - min) / max_labels;
      if (step <= 0)
        step = 1;
      for (int val = min; val < max; val += step)
      {
        const char *buf = GetYText(val);
	//sprintf(buf, "%d", val);
	dc->GetTextExtent((char *)buf, &tw, &th);
	float yy = y + h - h * (val - min) / (max - min) - th/2;
	dc->DrawText(buf, x - tw - TICK_LINE, yy);
	//dc->DrawLine(x - TICK_LINE, yy, x, yy);
      }
    }
  }

  else if (style_bits & ARED_MINMAX)
  {
    // min/max
    float tw, th;
    char buf[20];
    sprintf(buf, "%d", max);
    dc->GetTextExtent(buf, &tw, &th);
    dc->DrawText(buf, x - tw, y);
    sprintf(buf, "%d", min);
    dc->GetTextExtent(buf, &tw, &th);
    dc->DrawText(buf, x - tw, y + h - th);

  }

  dc->SetFont(wxNORMAL_FONT);

}

void tArrayEdit::DrawLabel()
{
  wxDC *dc = GetDC();
  dc->SetFont(wxSMALL_FONT);
  if (label)
    dc->DrawText(label, x + 5, y + 2);
  dc->SetFont(wxNORMAL_FONT);
}



void tArrayEdit::OnPaint()
{
  int i;
  wxDC *dc = GetDC();

  // surrounding rectangle
  dc->Clear();
  if (enabled)
    dc->SetBrush(wxWHITE_BRUSH);
  else
    dc->SetBrush(wxGREY_BRUSH);
  dc->SetPen(wxBLACK_PEN);
  if (w && h)
    dc->DrawRectangle(x, y, w, h);

  // sliders
  dc->SetBrush(wxBLACK_BRUSH);
  for (i = 0; i < n; i++)
    DrawBar(i, 1);

  DrawXTicks();
  DrawLabel();
  DrawYTicks();
  DrawNull();
  if (draw_bars)
    draw_bars->DrawBars();
}



void tArrayEdit::DrawNull()
{
  wxDC *dc = GetDC();
  dc->SetPen(wxCYAN_PEN);
  // draw y-null line
  if (min < nul && nul < max)
    dc->DrawLine(x, ynul, x+w, ynul);
  // draw x-null line
  if (xmin < 0 && 0 < xmax)
  {
    float x0 = w * (0 - xmin) / (xmax - xmin);
    dc->DrawLine(x0, y, x0, y + h);
  }
  dc->SetPen(wxBLACK_PEN);
}



void tArrayEdit::SetXMinMax(int xmi, int xma)
{
  xmin = xmi;
  xmax = xma;
}

int tArrayEdit::Index(wxMouseEvent &e)
{
  float ex, ey;
  e.Position(&ex, &ey);
  int i = (int)( ((short)ex - x) * n / w);
  i = i < 0 ? 0 : i;
  i = i >= n ? n-1 : i;
  return i;
}

int tArrayEdit::Dragging(wxMouseEvent &e)
{
  if (!dragging)
    return 0;

  if (index < 0)
    index = Index(e);

  int val = nul;
  if (e.LeftIsDown())
  {
    float ex, ey;
    e.Position(&ex, &ey);
    // $blk$ val = (int)( (y + h - (short)ey) * (max - min) / h + min);
    val = (int)( (double)(y + h - ey) * (max - min) / h + min + 0.5);
    val = val > max ? max : val;
    val = val < min ? min : val;
  }

#if 0
  {
    // in msw ex,ey are 65536 for negative values!
    wxDC *dc = GetDC();
    char buf[500];
    sprintf(buf, "x %4.0f, y %4.0f, sh %d", ex, ey, e.ShiftDown());
    dc->DrawText(buf, 50, 50);
  }
#endif

  if (e.ShiftDown())
  {
    int k;
    for (k = 0; k < n; k++)
    {
      DrawBar(k, 0);
      array[k] = val;
      DrawBar(k, 1);
    }
  }
  else if (e.ControlDown())
  {
    DrawBar(index, 0);
    array[index] = val;
    DrawBar(index, 1);
  }
  else
  {
    int i = Index(e);
    int k = i;
    if (i < index)
      for (; i <= index; i++)
      {
	DrawBar(i, 0);
	array[i] = val;
	DrawBar(i, 1);
      }
    else
      for (; i >= index; i--)
      {
	DrawBar(i, 0);
	array[i] = val;
	DrawBar(i, 1);
      }
    index = k;
  }

  return 0;
}

int tArrayEdit::ButtonDown(wxMouseEvent &e)
{
#ifdef wx_msw
  CaptureMouse();
#endif
  dragging = 1;
  index = Index(e);
  Dragging(e);
  return 0;
}

int tArrayEdit::ButtonUp(wxMouseEvent &e)
{
#ifdef wx_msw
  ReleaseMouse();
#endif
  dragging = 0;
  index    = -1;
  DrawLabel();
  DrawNull();
  return 0;
}


void tArrayEdit::OnEvent(wxMouseEvent &e)
{
  if (!enabled)
    return;
  if (e.ButtonDown())
    ButtonDown(e);
  else if (e.Dragging())
    Dragging(e);
  else if (e.ButtonUp())
    ButtonUp(e);
}

void tArrayEdit::Enable(int e)
{
  enabled = e;
}

void tArrayEdit::SetLabel(char const *llabel)
{
  delete label;
  label = copystring(llabel);
}

void tArrayEdit::SetYMinMax(int mi, int ma)
{
  array.SetMinMax(mi, ma);
  ynul = y + h - h * (nul - min) / (max - min);
}

void tArrayEdit::DrawBarLine (long xx)
{
  wxDC *dc = GetDC ();
  //  fprintf(stderr,"x: %ld, xx: %ld\n",x,xx);
  if (xx > x && xx + 1 < x + w)
    {
      dc->SetPen (wxLIGHT_GREY_PEN);
      dc->DrawLine (xx, y + 1, xx, y + h - 2);
      dc->SetPen (wxBLACK_PEN);
    }
}



tRhyArrayEdit::tRhyArrayEdit(wxFrame *parent, tRndArray &array, long xx, long yy, long ww, long hh, int sty)
  : tArrayEdit(parent, array, xx, yy, ww, hh, sty)
{
  steps_per_count = 4;
  count_per_bar   = 4;
  n_bars          = 4;
}

void tRhyArrayEdit::SetMeter(int s, int c, int b)
{
  steps_per_count = s;
  count_per_bar   = c;
  n_bars          = b;
  array.Resize(s * c * b);
  SetXMinMax(1, s * c * b);
}


void tRhyArrayEdit::DrawXTicks()
{
  if (!(style_bits & ARED_RHYTHM))
  {
    tArrayEdit::DrawXTicks();
    return;
  }

  char buf[20];
  float tw, th;

  wxDC *dc = GetDC();
  dc->SetFont(wxSMALL_FONT);

  // tick marks
  assert(steps_per_count && count_per_bar && n_bars);
  int i;
  for (i = 0; i < n; i += steps_per_count)
  {
    int mark = (i / steps_per_count) % count_per_bar + 1;
    sprintf(buf, "%d", mark);
    float yy = y + h;
    float xx = x + (i + 0.5) * w / n;
    dc->GetTextExtent(buf, &tw, &th);
    xx -= tw/2.0;
    dc->DrawText(buf, xx, yy);
  }
  dc->SetFont(wxNORMAL_FONT);
}
#endif

