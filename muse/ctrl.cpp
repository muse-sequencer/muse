//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrl.cpp,v 1.1.2.4 2009/06/10 00:34:59 terminator356 Exp $
//
//    controller handling for mixer automation
//
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Time E. Real (terminator356 on users dot sourceforge dot net)
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


#include <QLocale>
#include <QColor>

#include "gconfig.h"
#include "fastlog.h"
#include "math.h"
#include "globals.h"
#include "ctrl.h"
#include "xml.h"
#include "audio.h"

namespace MusECore {

void CtrlList::initColor(int i)
{
  QColor collist[] = { Qt::red, Qt::yellow, Qt::blue , Qt::black, Qt::white, Qt::green };

  if (i < 6)
    _displayColor = collist[i%6];
  else
    _displayColor = Qt::green;
  _visible = false;
}



//---------------------------------------------------------
//   CtrlList
//---------------------------------------------------------

CtrlList::CtrlList()
      {
      _id      = 0;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _dontShow = false;
      _visible = false;
      initColor(0);
      }

CtrlList::CtrlList(int id)
      {
      _id      = id;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _dontShow = false;
      _visible = false;
      initColor(id);
      }

CtrlList::CtrlList(int id, QString name, double min, double max, CtrlValueType v, bool dontShow)
{
      _id      = id;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _name    = name;
      _min     = min;
      _max     = max;
      _valueType = v;
      _dontShow = dontShow;
      _visible = false;
      initColor(id);
}

//---------------------------------------------------------
//   assign
//---------------------------------------------------------

void CtrlList::assign(const CtrlList& l, int flags)
{
  if(flags & ASSIGN_PROPERTIES)
  {
    _id            = l._id;
    _default       = l._default;
    _curVal        = l._curVal;
    _mode          = l._mode;
    _name          = l._name;
    _min           = l._min;
    _max           = l._max;
    _valueType     = l._valueType;
    _dontShow      = l._dontShow;
    _displayColor  = l._displayColor;
    _visible       = l._visible;
  }
  
  if(flags & ASSIGN_VALUES)
  {
    *this = l; // Let the vector assign values.
  }
}

//---------------------------------------------------------
//   value
//---------------------------------------------------------

double CtrlList::value(int frame) const
{
      if(empty()) 
        return _curVal;

      double rv;
      ciCtrl i = upper_bound(frame); // get the index after current frame

      if (i == end()) { // if we are past all items just return the last value
            --i;
            rv = i->second.val;
            }
      else if(_mode == DISCRETE)
      {
        if(i == begin())
        {
            rv = i->second.val;
        }  
        else
        {  
          --i;
          rv = i->second.val;
        }  
      }
      else {
        if (i == begin()) {
            rv = i->second.val;
        }
        else {
            int frame2 = i->second.frame;
            double val2 = i->second.val;
            --i;
            int frame1 = i->second.frame;
            double val1   = i->second.val;

            if (_valueType == VAL_LOG) {
              val1 = 20.0*fast_log10(val1);
              if (val1 < MusEGlobal::config.minSlider)
                val1=MusEGlobal::config.minSlider;
              val2 = 20.0*fast_log10(val2);
              if (val2 < MusEGlobal::config.minSlider)
                val2=MusEGlobal::config.minSlider;
            }

            frame -= frame1;
            val2  -= val1;
            frame2 -= frame1;
            val1 += (double(frame) * val2)/double(frame2);
    
            if (_valueType == VAL_LOG) {
              val1 = exp10(val1/20.0);
            }

            rv = val1;
          }
      }
      return rv;
}

//---------------------------------------------------------
//   curVal
//   returns the value at the current audio position 
//---------------------------------------------------------
double CtrlList::curVal() const
{ 
  return _curVal;
}

//---------------------------------------------------------
//   setCurVal
//---------------------------------------------------------
void CtrlList::setCurVal(double val)
{
  _curVal = val;
}

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void CtrlList::add(int frame, double val)
      {
      iCtrl e = find(frame);
      if (e != end())
            e->second.val = val;
      else
            insert(std::pair<const int, CtrlVal> (frame, CtrlVal(frame, val)));
      }

//---------------------------------------------------------
//   del
//---------------------------------------------------------

void CtrlList::del(int frame)
      {
      iCtrl e = find(frame);
      if (e == end())
            return;
      
      erase(e);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void CtrlList::read(Xml& xml)
      {
      QLocale loc = QLocale::c();
      bool ok;
      for (;;) {
            Xml::Token token = xml.parse();
            const QString& tag = xml.s1();
            switch (token) {
                  case Xml::Error:
                  case Xml::End:
                        return;
                  case Xml::Attribut:
                        if (tag == "id")
                        {
                              _id = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "cur")
                        {
                              _curVal = loc.toDouble(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _curVal string: %s\n", xml.s2().toLatin1().constData());
                        }        
                        else if (tag == "visible")
                        {
                              _visible = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _visible string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "color")
                        {
#if QT_VERSION >= 0x040700
                              ok = _displayColor.isValidColor(xml.s2());
                              if (!ok) {
                                printf("CtrlList::read failed reading color string: %s\n", xml.s2().toLatin1().constData());
                                break;
                              }
#endif
                              _displayColor.setNamedColor(xml.s2());
                        }
                        else
                              printf("unknown tag %s\n", tag.toLatin1().constData());
                        break;
                  case Xml::Text:
                        {
                          int len = tag.length();
                          int frame;
                          double val;
  
                          int i = 0;
                          for(;;) 
                          {
                                while(i < len && (tag[i] == ',' || tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                
                                QString fs;
                                while(i < len && tag[i] != ' ')
                                {
                                  fs.append(tag[i]); 
                                  ++i;
                                }
                                if(i == len)
                                      break;
                                
                                frame = loc.toInt(fs, &ok);
                                if(!ok)
                                {
                                  printf("CtrlList::read failed reading frame string: %s\n", fs.toLatin1().constData());
                                  break;
                                }
                                  
                                while(i < len && (tag[i] == ' ' || tag[i] == '\n'))
                                  ++i;
                                if(i == len)
                                      break;
                                
                                QString vs;
                                while(i < len && tag[i] != ' ' && tag[i] != ',')
                                {
                                  vs.append(tag[i]); 
                                  ++i;
                                }
                                
                                val = loc.toDouble(vs, &ok);
                                if(!ok)
                                {
                                  printf("CtrlList::read failed reading value string: %s\n", vs.toLatin1().constData());
                                  break;
                                }
                                  
                                add(frame, val);
                                
                                if(i == len)
                                      break;
                          }
                        }
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "controller")
                              return;
                  default:
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void CtrlListList::add(CtrlList* vl)
      {
      insert(std::pair<const int, CtrlList*>(vl->id(), vl));
      }
      
} // namespace MusECore
