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
//#include <stdlib.h>

#include "gconfig.h"
#include "fastlog.h"
#include "math.h"
#include "globals.h"
#include "ctrl.h"
#include "xml.h"
#include "audio.h"

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

CtrlList::CtrlList(int id)
      {
      _id      = id;
      _default = 0.0;
      _curVal  = 0.0;
      _mode    = INTERPOLATE;
      _dontShow = false;
      initColor(id);
      }
//---------------------------------------------------------
//   CtrlList
//---------------------------------------------------------
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
      initColor(id);
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
      initColor(0);
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

double CtrlList::value(int frame) const
{
      // Changed by Tim. p4.0.32...
      
      ///if (!automation || empty()) 
      ///      return _curVal;
      if(empty()) 
        return _curVal;

      double rv;
      ciCtrl i = upper_bound(frame); // get the index after current frame

      if (i == end()) { // if we are past all items just return the last value
            ///ciCtrl i = end();
            --i;
            ///const CtrlVal& val = i->second;
            ///_curVal = val.val;
            rv = i->second.val;
            }
      else if(_mode == DISCRETE)
      {
        if(i == begin())
        {
          ///_curVal = _default;
          //if(i->second.frame == frame)
            rv = i->second.val;
          //else  
          //  rv = _default;
        }  
        else
        {  
          --i;
          ///const CtrlVal& val = i->second;
          ///_curVal = val.val;
          rv = i->second.val;
        }  
      }
      else {
        ///int frame2 = i->second.frame;
        ///double val2 = i->second.val;
        ///int frame1;
        ///double val1;
        if (i == begin()) {
            ///frame1 = 0;
            ///val1   = _default;
            rv = i->second.val;
        }
        else {
            int frame2 = i->second.frame;
            double val2 = i->second.val;
            --i;
            ///frame1 = i->second.frame;
            ///val1   = i->second.val;
            int frame1 = i->second.frame;
            double val1   = i->second.val;
        ///}
            //printf("before val1=%f val2=%f\n", val1,val2);
            if (_valueType == VAL_LOG) {
              val1 = 20.0*fast_log10(val1);
              if (val1 < MusEConfig::config.minSlider)
                val1=MusEConfig::config.minSlider;
              val2 = 20.0*fast_log10(val2);
              if (val2 < MusEConfig::config.minSlider)
                val2=MusEConfig::config.minSlider;
            }
            //printf("after val1=%f val2=%f\n", val1,val2);
            frame -= frame1;
            val2  -= val1;
            frame2 -= frame1;
            val1 += (double(frame) * val2)/double(frame2);
    
            if (_valueType == VAL_LOG) {
              val1 = exp10(val1/20.0);
            }
            //printf("after val1=%f\n", val1);
            ///_curVal = val1;
            rv = val1;
          }
      }
// printf("autoVal %d %f\n", frame, _curVal);
      ///return _curVal;
      return rv;
}

//---------------------------------------------------------
//   curVal
//   returns the value at the current audio position 
//---------------------------------------------------------
double CtrlList::curVal() const
{ 
  //double v = value(Pos(audio->tickPos()).frame());      // p4.0.33
  //double v = value(audio->pos().frame());                 // Try this.
  //return v;
  return _curVal;
}

//---------------------------------------------------------
//   setCurVal
//---------------------------------------------------------
void CtrlList::setCurVal(double val)
{
  _curVal = val;
  //if (size() < 2)   // Removed p4.0.32
  //  add(0,val);
}

//---------------------------------------------------------
//   add
//---------------------------------------------------------

void CtrlList::add(int frame, double val)
      {
// printf("add %d %f\n", frame, val);
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
      if (e == end()) {
            //printf("CtrlList::del(%d): not found\n", frame);
            return;
            }
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
                              //_id = xml.s2().toInt();
                              _id = loc.toInt(xml.s2(), &ok);
                              if(!ok)
                                printf("CtrlList::read failed reading _id string: %s\n", xml.s2().toLatin1().constData());
                        }
                        else if (tag == "cur")
                        {
                              //_curVal = xml.s2().toDouble();
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
                        // Changed by Tim. Users in some locales reported corrupt reading,
                        //  because of the way floating point is represented (2,3456 not 2.3456).
                        /*
                        QByteArray ba = tag.toLatin1();
                        const char* s = ba;.constData();
                        int frame;
                        double val;

                        for (;;) {
                              char* endp;
                              while (*s == ' ' || *s == '\n')
                                    ++s;
                              if (*s == 0)
                                    break;
                              frame = strtol(s, &endp, 10);
                              s     = endp;
                              while (*s == ' ' || *s == '\n')
                                    ++s;
                              val = strtod(s, &endp);
                              add(frame, val);
                              s = endp;
                              ++s;
                              }
                           */   
                          
                          //printf("CtrlList::read tag:%s\n", tag.toLatin1().constData());
                          
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
                                
                                // Works OK, but only because if current locale fails it falls back on 'C' locale.
                                // So, let's skip the fallback and force use of 'C' locale.
                                //frame = fs.toInt(&ok);
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
                                
                                // Works OK, but only because if current locale fails it falls back on 'C' locale.
                                // So, let's skip the fallback and force use of 'C' locale.
                                //val = vs.toDouble(&ok); 
                                val = loc.toDouble(vs, &ok);
                                if(!ok)
                                {
                                  printf("CtrlList::read failed reading value string: %s\n", vs.toLatin1().constData());
                                  break;
                                }
                                  
                                //printf("CtrlList::read i:%d len:%d fs:%s frame %d: vs:%s val %f \n", i, len, fs.toLatin1().constData(), frame, vs.toLatin1().constData(), val);
                                
                                add(frame, val);
                                
                                if(i == len)
                                      break;
                          }
                        }
                        break;
                  case Xml::TagEnd:
                        if (xml.s1() == "controller")
                        {
                              //printf("CtrlList::read _id:%d _curVal:%f\n", _id, _curVal);
                              
                              return;
                        }      
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
//      printf("CtrlListList(%p)::add(id=%d) size %d\n", this, vl->id(), size());
      insert(std::pair<const int, CtrlList*>(vl->id(), vl));
      }

