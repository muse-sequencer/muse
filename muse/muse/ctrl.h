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

#ifndef __CTRL_H__
#define __CTRL_H__

namespace AL {
      class Xml;
      };
using AL::Xml;

class MidiController;

//
// predefined audio controller id's:
//
const int NUM_AUX   = 32;     // max number of aux channels

const int AC_VOLUME = 0;
const int AC_PAN    = 1;
const int AC_MUTE   = 2;
const int AC_AUX    = 3;      // 3 -- 3+NUM_AUX
const int AC_AUX_PAN = AC_AUX + NUM_AUX;

const int CTRL_PLUGIN_MASK   = 0x3ff0000;
const int CTRL_PLUGIN_OFFSET = 0x10000;
const int CTRL_INDEX_MASK    = 0xffff;
const int CTRL_PREFADER      = 0x40000000;

//---------------------------------------------------------
//   genACnum
//    create a controller number out of plugin index,
//    controller index and prefader flag
//---------------------------------------------------------

inline static int genACnum(int plugin, int ctrl, bool prefader) {
      int pre = prefader ? CTRL_PREFADER : 0;
      return pre | ((plugin+1) * CTRL_PLUGIN_OFFSET + ctrl);
      }

//---------------------------------------------------------
//   getCtrlPlugin
//    destill plugin index, controller index and prefader
//    flag from controller id
//---------------------------------------------------------

inline static void getCtrlPlugin(int id, bool* prefader, int* pluginIndex, 
   int* ctrlIndex) {
      *prefader    = (id & CTRL_PREFADER) ? true : false;
      *pluginIndex = ((id & CTRL_PLUGIN_MASK) / CTRL_PLUGIN_OFFSET) - 1;
      *ctrlIndex   = id  & CTRL_INDEX_MASK;
      }

//---------------------------------------------------------
//   ControllerName
//---------------------------------------------------------

struct ControllerName {
      QString name;
      int id;
      ControllerName(const QString& s, int i) : name(s), id(i) {}
      };

typedef std::vector<ControllerName> ControllerNameList;
typedef ControllerNameList::iterator iControllerName;
typedef ControllerNameList::const_iterator ciControllerName;

//---------------------------------------------------------
//   CVal
//    Controller value, either float for audio or
//    int for midi.
//
//---------------------------------------------------------

struct CVal {
      union {
            float f;
            int i;
            };
      };

//---------------------------------------------------------
//   CtrlVal
//    This structure represents a controller event. Time
//    is a midi tick value or a frame value depending on
//    then containing controller list Ctrl
//---------------------------------------------------------

struct CtrlVal {
      int time;
      CVal val;
      CtrlVal(int f, CVal v) {
            time = f;
            val   = v;
            }
      CtrlVal(int f) {
            time = f;
            }
      };

//---------------------------------------------------------
//   CtrlRecVal
//    recorded controller event, mixer automation
//---------------------------------------------------------

struct CtrlRecVal : public CtrlVal {
      int id;
      int type;   // 0 - ctrlVal, 1 - start, 2 - end
      CtrlRecVal(int f, int n, CVal v) : CtrlVal(f, v), id(n), type(0) {}
      CtrlRecVal(int f, int n, int t) : CtrlVal(f), id(n), type(t) {}
      };

//---------------------------------------------------------
//   CtrlRecList
//---------------------------------------------------------

class CtrlRecList : public std::list<CtrlRecVal> {
   public:
      };

typedef CtrlRecList::iterator iCtrlRec;
typedef QMap<unsigned, CVal> CTRL;

typedef CTRL::iterator iCtrlVal;
typedef CTRL::const_iterator ciCtrlVal;


//---------------------------------------------------------
//   Ctrl
//    this is a controller list
//---------------------------------------------------------

class Ctrl : public CTRL {
   public:
      enum CtrlType {
            INTERPOLATE = 0,      // values are linear interpolated
            DISCRETE    = 1,      // midi controller events
            LINEAR      = 0,
            LOG         = 2,
            INT         = 4
            };

   private:
      int     _id;
      QString  _name;
      int   _type;         // bitmask of CtrlType
      CVal _default;
      CVal _curVal;        // used to optimize controller events
      CVal min, max;
      bool _changed;
      bool _touched;

   public:
      Ctrl();
      Ctrl(const MidiController*);
      Ctrl(int id, const QString& name, int t=INTERPOLATE);
      Ctrl(int id, const QString& name, int t, float a, float b);
      int type() const               { return _type;    }
      void setType(int t)            { _type = t;       }

      const CVal& getDefault() const { return _default; }
      void setDefault(float val)     { _default.f = val;  }
      void setDefault(CVal val)      { _default = val;  }
      void setDefault(int val)       { _default.i = val;  }

      const CVal& curVal() const     { return _curVal;  }
      void setCurVal(CVal v)         { _curVal   = v; }
      void setCurVal(float v)        { _curVal.f = v; }
      void setCurVal(int v)          { _curVal.i = v; }

      int id() const                 { return _id;      }
      void setId(int i)              { _id = i;         }
      QString name() const           { return _name;    }
      void setName(const QString& s) { _name = s;       }
      CVal value(unsigned);
      bool add(unsigned, CVal);
      void del(unsigned);
      void setChanged(bool val)      { _changed = val;  }
      bool changed() const           { return _changed; }
      void setTouched(bool val)      { _touched = val;  }
      bool touched() const           { return _touched; }
      void setRange(double min, double max);
      void setRange(int min, int max);
      CVal minVal() const            { return min; }
      CVal maxVal() const            { return max; }
      void read(QDomNode node, bool midi);
      void write(Xml&);
      int val2pixelR(CVal, int maxpixel);
      int cur2pixel(int maxpixel);
      int val2pixelR(int, int maxpixel);
      CVal pixel2val(int pixel, int maxpixel);
      CVal pixel2valR(int pixel, int maxpixel);
      };

//---------------------------------------------------------
//   CtrlList
//    List of controller value lists.
//    This list represents the controller state of a
//    mixer strip
//---------------------------------------------------------

typedef std::map<unsigned, Ctrl*, std::less<unsigned> > CLIST;
typedef CLIST::iterator iCtrl;
typedef CLIST::const_iterator ciCtrl;

class CtrlList : public CLIST {
   public:
      void add(Ctrl* vl);
      };

#endif

