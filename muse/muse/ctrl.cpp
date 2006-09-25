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

#include "fastlog.h"
#include "globals.h"
#include "ctrl.h"
#include "al/xml.h"
#include "midictrl.h"

//---------------------------------------------------------
//   Ctrl
//---------------------------------------------------------

Ctrl::Ctrl(int id, const QString& s, int t)
   : _id(id), _name(s), _type(t)
      {
      setRange(.0f, 1.f);
      _default.f  = 0.0f;
      _curVal.f   = 0.0f;
      _schedVal.f = 0.0f;
      _schedValRaw.f = 0.0f;
      _touched    = false;
      _changed    = false;
      }

Ctrl::Ctrl(int id, const QString& s, int t, float a, float b)
   : _id(id), _name(s), _type(t)
      {
      if (_type & INT)
            setRange(int(a), int(b));
      else
            setRange(a, b);
      _default.f  = 0.0f;
      _curVal.f   = 0.0f;
      _schedValRaw.f = 0.0f;
      _touched    = false;
      _changed    = false;
      }

Ctrl::Ctrl()
      {
      _type       = INTERPOLATE;
      setRange(0.0f, 1.0f);
      _id         = 0;
      _default.f  = 0.0f;
      _curVal.f   = 0.0f;
      _schedValRaw.f = 0.0f;
      _touched    = false;
      _changed    = false;
      }

Ctrl::Ctrl(const MidiController* mc)
      {
      _type       = DISCRETE | INT;
      setRange(mc->minVal(), mc->maxVal());
      _id         = mc->num();
      _default.i  = mc->initVal();
      _curVal.i   = CTRL_VAL_UNKNOWN;
      _schedValRaw.i = CTRL_VAL_UNKNOWN;
      _name       = mc->name();
      _touched    = false;
      _changed    = false;
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

CVal Ctrl::value(unsigned time)
      {
      CVal rv;

      if (empty() || _touched)
            return _curVal;
      if (_type & DISCRETE) {
            //
            // midi controller
            //
            ciCtrlVal i = upper_bound(time);
            if (i == end()) {
                  --i;
                  rv = i->second;
                  }
            else if (i == begin()) {
                  if (i->first == time)
                        rv = i->second;
                  else
                        return _curVal;
                  }
            else {
                  --i;
                  rv = i->second;
                  }
            }
      else {
            ciCtrlVal i = upper_bound(time);
            if (i == end()) {
                  --i;
                  return i->second;
                  }
            int frame2 = i->first;
            CVal val2 = i->second;
            int frame1;
            CVal val1;
            if (i == begin()) {
                  frame1 = 0;
                  val1   = _default;
                  }
            else {
                  --i;
                  frame1 = i->first;
                  val1   = i->second;
                  }
            time -= frame1;
            frame2 -= frame1;
            if (_type & INT) {
                  val2.i -= val1.i;
                  rv.i = val1.i + (time * val2.i)/frame2;
                  }
            else {
                  val2.f  -= val1.f;
                  rv.f = val1.f + (time * val2.f)/frame2;
                  }
            }
      if (_type & LOG) {
            if (rv.f == -1000.0f)
                  rv.f = 0.0f;
            else
                  rv.f = pow(10.0f, rv.f);
            }
      return rv;
      }

//---------------------------------------------------------
//   add
//    return true if new value added
//---------------------------------------------------------

bool Ctrl::add(unsigned frame, CVal val)
      {
      if (_type & LOG) {
            if (val.f <= 0.0)
                  val.f = -1000.0f;
            else
                  val.f = fast_log10(val.f);
            }
      iCtrlVal e = find(frame);
      if (e != end()) {
            e->second = val;
            return false;
            }
      insert(std::pair<const int, CVal> (frame, val));
      return true;
      }

//---------------------------------------------------------
//   setSchedVal
//---------------------------------------------------------

void Ctrl::setSchedVal(CVal val)
      {
      if (_type & LOG) {
            if (val.f <= 0.0)
                  _schedValRaw.f = -1000.0f;
            else
                  _schedValRaw.f = fast_log10(val.f);
            }
      else {
            _schedValRaw = val;
            }
      _schedVal = val;
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void Ctrl::del(unsigned frame)
      {
      iCtrlVal e = find(frame);
      erase(e);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void Ctrl::read(QDomNode node, bool)
      {
      QDomElement e = node.toElement();
      _id           = e.attribute("id").toInt();
      _name         = e.attribute("name");
      _type         = e.attribute("type","0").toInt();

      char* minS    = "0.0f";
      char* maxS    = "1.0f";

      if (_type & INT) {
            minS  = "0";
            maxS  = "127";
            _curVal.i  = e.attribute("cur","-1").toInt();
            _default.i = e.attribute("default","-1").toInt();
            }
      else {
            minS  = "0.0f";
            maxS  = "1.0f";
            if (_id == AC_PAN) {
                  minS = "-1.0f";
                  maxS = "+1.0f";
                  }
            _curVal.f  = e.attribute("cur","0.0").toFloat();
            _default.f = e.attribute("default","0.0").toFloat();
            }
      setSchedVal(_curVal);
      if (_type & INT) {
            min.i = e.attribute("min", minS).toInt();
            max.i = e.attribute("max", maxS).toInt();
            }
      else {
            min.f = e.attribute("min", minS).toFloat();
            max.f = e.attribute("max", maxS).toFloat();
            }

      QStringList vp = e.text().simplified().split(",", QString::SkipEmptyParts);
      int n = vp.size();
      for (int i = 0; i < n; ++i) {
            QStringList sl = vp.at(i).simplified().split(" ");
            bool ok;
            int frame = sl.at(0).toInt(&ok, 0);
            if (!ok) {
                  printf("Ctrl::read(1): conversion <%s><%s> to int failed\n",
                     vp.at(i).simplified().toAscii().data(),
                     sl.at(0).toAscii().data());
                  break;
                  }
            CVal val;
            if (_type & INT)
                  val.i = sl.at(1).toInt(&ok, 0);
            else
                  val.f = sl.at(1).toDouble(&ok);
            if (!ok) {
                  printf("Ctrl::read(2): conversion <%s><%s> failed\n",
                     vp.at(i).simplified().toAscii().data(),
                     sl.at(1).toAscii().data());
                  break;
                  }
            add(frame, val);
            }
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void CtrlList::add(Ctrl* vl)
      {
      insert(std::pair<const int, Ctrl*>(vl->id(), vl));
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void Ctrl::write(Xml& xml)
      {
      QString s("controller id=\"%1\" name=\"%2\" cur=\"%3\" type=\"%4\" min=\"%5\" max=\"%6\" default=\"%7\"");

      if (empty()) {
            if (_type & INT)
                  xml.tagE(s.arg(id()).arg(_name).arg(curVal().i).arg(_type).arg(min.i).arg(max.i).arg(_default.i).toAscii().data());
            else
                  xml.tagE(s.arg(id()).arg(_name).arg(curVal().f).arg(_type).arg(min.f).arg(max.f).arg(_default.f).toAscii().data());
            return;
            }
      if (_type & INT)
            xml.tag(s.arg(id()).arg(_name).arg(curVal().i).arg(_type).arg(min.i).arg(max.i).arg(_default.i).toAscii().data());
      else
            xml.tag(s.arg(id()).arg(_name).arg(curVal().f).arg(_type).arg(min.f).arg(max.f).arg(_default.f).toAscii().data());

      int i = 0;
      for (ciCtrlVal ic = begin(); ic != end(); ++ic) {
            if (i == 0)
                  xml.putLevel();
            int time  = ic->first;
            CVal val = ic->second;
            if (_type & LOG)
                  val.f = (val.f == -1000.0) ? 0.0f : pow(10.0f, val.f);
            if (_type & INT) {
                  xml.nput("%d %d,", time, val.i);
                  }
            else {
                  QString fval,ttime;
                  fval.setNum(val.f);
                  ttime.setNum(time);
                  QString str=ttime + " "+ fval + ",";
                  xml.nput(str.toAscii().data());
                  }
            ++i;
            if (i >= 4) {
                  xml.nput("\n");
                  i = 0;
                  }
            }
      if (i)
            xml.nput("\n");
      xml.etag("controller");
      }

//---------------------------------------------------------
//   val2pixelR
//    input val is "raw" data
//---------------------------------------------------------

int Ctrl::val2pixelR(CVal val, int maxpixel)
      {
      if (_type & INT)
            return maxpixel - ((maxpixel * (val.i - min.i) + (max.i-min.i)/2) / (max.i-min.i));
      else
            return maxpixel - lrint(float(maxpixel) * (val.f - min.f) / (max.f-min.f));
      }

int Ctrl::val2pixelR(int val, int maxpixel)
      {
      if (_type & INT)
            return maxpixel - (maxpixel * (val - min.i) / (max.i-min.i));
      else
            return maxpixel - lrint(float(maxpixel) * (val - min.f) / (max.f-min.f));
      }

//---------------------------------------------------------
//   pixel2val
//---------------------------------------------------------

CVal Ctrl::pixel2val(int pixel, int maxpixel)
      {
      pixel = maxpixel - pixel;

// printf("pixel2val %d(%d) int %d, min %d, max %d\n",
//   pixel, maxpixel, _type & INT, min.i, max.i);
      CVal rv;
      if (_type & INT) {
            rv.i = (pixel * (max.i - min.i) + (maxpixel+min.i)/2) / maxpixel + min.i;
            if (rv.i < min.i)
                  rv.i = min.i;
            else if (rv.i > max.i)
                  rv.i = max.i;
            }
      else {
            rv.f = float(pixel) * (max.f - min.f) / float(maxpixel) + min.f;
            if (rv.f < min.f)
                  rv.f = min.f;
            else if (rv.f > max.f)
                  rv.f = max.f;
            }

      if (_type & LOG)
            rv.f = pow(10.0f, rv.f);
      return rv;
      }

//---------------------------------------------------------
//   setRange
//---------------------------------------------------------

void Ctrl::setRange(double _min, double _max)
      {
      if (_type & LOG) {
            min.f = fast_log10(_min);
            max.f = fast_log10(_max);
            }
      else {
            min.f = _min;
            max.f = _max;
            }
      }

void Ctrl::setRange(int _min, int _max)
      {
      min.i = _min;
      max.i = _max;
      }

