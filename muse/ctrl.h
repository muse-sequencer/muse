//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrl.h,v 1.4.2.2 2006/10/29 07:54:51 terminator356 Exp $
//
//    controller for mixer automation
//
//  (C) Copyright 2003-2004 Werner Schweer (ws@seh.de)
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

#ifndef __CTRL_H__
#define __CTRL_H__

#include <map>
#include <list>
#include <qcolor.h>
#include <lo/lo_osc_types.h>

#define AC_PLUGIN_CTL_BASE         0x1000
#define AC_PLUGIN_CTL_BASE_POW     12
#define AC_PLUGIN_CTL_ID_MASK      0xFFF

namespace MusECore {

class Xml;

const int AC_VOLUME = 0;
const int AC_PAN    = 1;
const int AC_MUTE   = 2;

inline unsigned long genACnum(unsigned long plugin, unsigned long ctrl) { return (plugin + 1) * AC_PLUGIN_CTL_BASE + ctrl; }

enum CtrlValueType { VAL_LOG, VAL_LINEAR, VAL_INT, VAL_BOOL };
enum CtrlRecValueType { ARVT_VAL, ARVT_START, ARVT_STOP };

//---------------------------------------------------------
//   CtrlVal
//    controller "event"
//---------------------------------------------------------

struct CtrlVal {
      int frame;
      double val;
      CtrlVal(int f, double v) { 
            frame = f;
            val   = v;
            }
      };

//---------------------------------------------------------
//   CtrlRecVal
//    recorded controller event, mixer automation
//---------------------------------------------------------

struct CtrlRecVal : public CtrlVal {
      int id;
      CtrlRecValueType type;   // 0 - ctrlVal, 1 - start, 2 - end
      CtrlRecVal(int f, int n, double v) : CtrlVal(f, v), id(n), type(ARVT_VAL) {}
      CtrlRecVal(int f, int n, double v, CtrlRecValueType t) : CtrlVal(f, v), id(n), type(t) {}
      };

//---------------------------------------------------------
//   CtrlRecList
//---------------------------------------------------------

class CtrlRecList : public std::list<CtrlRecVal> {
   public:
      };

typedef CtrlRecList::iterator iCtrlRec;

//---------------------------------------------------------
//   MidiAudioCtrlMap
//    Describes midi control of audio controllers
//---------------------------------------------------------

struct MidiAudioCtrlStruct {
      
      };

typedef std::map<int, MidiAudioCtrlStruct, std::less<int> >::iterator iMidiAudioCtrlStructMap;
typedef std::map<int, MidiAudioCtrlStruct, std::less<int> >::const_iterator ciMidiAudioCtrlStructMap;
class MidiAudioCtrlStructMap : public std::map<int /*audio ctrl id number*/, MidiAudioCtrlStruct, std::less<int> > {
  public:
    
     };
      
typedef std::map<int, MidiAudioCtrlStructMap, std::less<int> >::iterator iMidiAudioCtrlMap;
typedef std::map<int, MidiAudioCtrlStructMap, std::less<int> >::const_iterator ciMidiAudioCtrlMap;
class MidiAudioCtrlMap : public std::map<int /*midi ctrl number*/, MidiAudioCtrlStructMap, std::less<int> > {
  public:
    
     };

typedef std::map<int, MidiAudioCtrlMap, std::less<int> >::iterator iMidiAudioCtrlChanMap;
typedef std::map<int, MidiAudioCtrlMap, std::less<int> >::const_iterator ciMidiAudioCtrlChanMap;
class MidiAudioCtrlChanMap : public std::map<int /*midi chan number*/, MidiAudioCtrlMap, std::less<int> > {
   public:
     
      };
      
// A reverse lookup thingy. Slower for now.
struct AudioMidiCtrlStruct {
      int _midi_port;
      int _midi_chan;
      int _midi_ctrl;
  
      AudioMidiCtrlStruct(int midi_port, int midi_chan, int midi_ctrl)
      {
        _midi_port = midi_port;
        _midi_chan = midi_chan;
        _midi_ctrl = midi_ctrl;
      }
      };
typedef std::multimap<int, AudioMidiCtrlStruct, std::less<int> >::iterator iAudioMidiCtrlStructMap;
typedef std::multimap<int, AudioMidiCtrlStruct, std::less<int> >::const_iterator ciAudioMidiCtrlStructMap;
class AudioMidiCtrlStructMap : public std::multimap<int /*audio ctrl id number*/, AudioMidiCtrlStruct, std::less<int> > {
  public:
    
     };
    
// The main top level midi to audio map.     
typedef std::map<int, MidiAudioCtrlChanMap, std::less<int> >::iterator iMidiAudioCtrlPortMap;
typedef std::map<int, MidiAudioCtrlChanMap, std::less<int> >::const_iterator ciMidiAudioCtrlPortMap;
class MidiAudioCtrlPortMap : public std::map<int /*midi port number*/, MidiAudioCtrlChanMap, std::less<int> > {
   public:
     
      // Add will replace if found.
      MidiAudioCtrlStruct*    add_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, int audio_ctrl_id);
      MidiAudioCtrlStructMap* find_ctrl_map(int midi_port, int midi_chan, int midi_ctrl_num);
      MidiAudioCtrlStruct*    find_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, int audio_ctrl_id);
      void find_audio_ctrl_structs(int audio_ctrl_id, AudioMidiCtrlStructMap* amcs);
      void erase_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, int audio_ctrl_id);
      };

      
      
      
      
//---------------------------------------------------------
//   CtrlList
//    arrange controller events of a specific type in a
//    list for easy retrieval
//---------------------------------------------------------

typedef std::map<int, CtrlVal, std::less<int> >::iterator iCtrl;
typedef std::map<int, CtrlVal, std::less<int> >::const_iterator ciCtrl;

class CtrlList : public std::map<int, CtrlVal, std::less<int> > {
   public:
      enum Mode { INTERPOLATE, DISCRETE};
      enum AssignFlags { ASSIGN_PROPERTIES=1, ASSIGN_VALUES=2 };  // Can be or'd together.
   private:
      Mode _mode;
      int _id;
      double _default;
      double _curVal;
      void del(CtrlVal);
      QString _name;
      double _min, _max;  
      CtrlValueType _valueType;
      QColor _displayColor;
      bool _visible;
      bool _dontShow; // when this is true the control exists but is not compatible with viewing in the arranger
      void initColor(int i);

   public:
      CtrlList();
      CtrlList(int id);
      CtrlList(int id, QString name, double min, double max, CtrlValueType v, bool dontShow=false);
      void assign(const CtrlList& l, int flags); 

      Mode mode() const          { return _mode; }
      void setMode(Mode m)       { _mode = m; }
      double getDefault() const   { return _default; }
      void setDefault(double val) { _default = val; }
      double curVal() const;
      void updateCurValue(int frame);
      void setCurVal(double val);
      int id() const             { return _id; }
      QString name() const       { return _name; }
      void setName(const QString& s) { _name = s; }
      void setRange(double min, double max) {
            _min = min;
            _max = max;
            }
      void range(double* min, double* max) const {
            *min = _min;
            *max = _max;
            }
      CtrlValueType valueType() const { return _valueType; }
      void setValueType(CtrlValueType t) { _valueType = t; }

      double value(int frame) const;
      void add(int frame, double value);
      void del(int frame);
      void read(Xml& xml);

      void setColor( QColor c ) { _displayColor = c;}
      QColor color() const { return _displayColor; }
      void setVisible(bool v) { _visible = v; }
      bool isVisible() const { return _visible; }
      bool dontShow() const { return _dontShow; }
      };

//---------------------------------------------------------
//   CtrlListList
//    List of controller value lists.
//    This list represents the controller state of a
//    mixer strip
//---------------------------------------------------------

typedef std::map<int, CtrlList*, std::less<int> >::iterator iCtrlList;
typedef std::map<int, CtrlList*, std::less<int> >::const_iterator ciCtrlList;

class CtrlListList : public std::map<int, CtrlList*, std::less<int> > {
   private:
      MidiAudioCtrlPortMap _midi_controls;  // For midi control of audio controllers.
   public:
      void add(CtrlList* vl);
      void clearDelete() {
            for(iCtrlList i = begin(); i != end(); ++i)
              delete i->second;
            clear();
           }     

      iCtrlList find(int id) {
            return std::map<int, CtrlList*, std::less<int> >::find(id);
            }
      ciCtrlList find(int id) const {
            return std::map<int, CtrlList*, std::less<int> >::find(id);
            }
            
      MidiAudioCtrlPortMap* midiControls() { return &_midi_controls; }  
      
      double value(int ctrlId, int frame, bool cur_val_only = false) const;
      void updateCurValues(int frame);
      void clearAllAutomation() {
            for(iCtrlList i = begin(); i != end(); ++i)
              i->second->clear();
           }     
      };

extern double midi2AudioCtrlValue(const CtrlList* audio_ctrl_list, const MidiAudioCtrlStruct* mapper, int midi_ctlnum, int midi_val);

} // namespace MusECore

#endif

