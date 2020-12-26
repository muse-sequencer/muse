//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrl.h,v 1.4.2.2 2006/10/29 07:54:51 terminator356 Exp $
//
//    controller for mixer automation
//
//  (C) Copyright 2003-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2013 Tim E. Real (terminator356 on users dot sourceforge dot net)
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
#include <vector>
#include <QColor>
#include <QString>

#ifdef OSC_SUPPORT
#include <lo/lo_osc_types.h>
#else
#include <stdint.h>
#endif

#define AC_PLUGIN_CTL_BASE         0x1000
#define AC_PLUGIN_CTL_BASE_POW     12
#define AC_PLUGIN_CTL_ID_MASK      0xFFF

namespace MusECore {


// Forward declarations:
class Xml;

const int AC_VOLUME = 0;
const int AC_PAN    = 1;
const int AC_MUTE   = 2;

inline unsigned long genACnum(unsigned long plugin, unsigned long ctrl) { return (plugin + 1) * AC_PLUGIN_CTL_BASE + ctrl; }

enum CtrlValueType { VAL_LOG, VAL_LINEAR, VAL_INT, VAL_BOOL, VAL_ENUM };
enum CtrlRecValueType { ARVT_VAL, ARVT_START, ARVT_STOP };

typedef std::map<float, QString> CtrlEnumValues;

//---------------------------------------------------------
//   CtrlInterpolate
//    Controller interpolation values.
//    For speed: Can be filled once by CtrlList::getInterpolation(),
//     then passed repeatedly to CtrlList::interpolate().
//---------------------------------------------------------

struct CtrlInterpolate {
      unsigned int sFrame; // Starting frame. Always valid. Can be less than any first CtrlList item's frame, or zero !
      double sVal;       // Value at starting frame.
      unsigned int eFrame; // Ending frame if eFrameValid is true.
      bool   eFrameValid; // True if eFrame is valid. False if endless, eFrame is invalid.
      double eVal;       // Value at ending frame, or sVal if eFrameValid is false.
      bool   eStop;      // Whether to stop refreshing this struct from CtrlList upon eFrame. Control FIFO ring buffers
                         //  set this true and replace eFrame and eVal. Upon the next run slice, if eStop is set, eval
                         //  should be copied to sVal, eFrame to sFrame, doInterp cleared, and eFrame set to some frame or eFrameValid false.
      bool   doInterp;   // Whether to actually interpolate whenever this struct is passed to CtrlList::interpolate().
      CtrlInterpolate(unsigned int sframe = 0, unsigned int eframe = 0, bool eframevalid = false, double sval = 0.0, double eval = 0.0,
                      bool end_stop = false, bool do_interpolate = false) {
            sFrame = sframe;
            sVal   = sval;
            eFrame = eframe;
            eFrameValid = eframevalid;
            eVal   = eval;
            eStop = end_stop;
            doInterp = do_interpolate;
            }
      };

//---------------------------------------------------------
//   CtrlVal
//    controller "event"
//---------------------------------------------------------

struct CtrlVal {
      unsigned int frame;
      double val;
      CtrlVal(unsigned int f, double v) { 
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
      CtrlRecVal(unsigned int f, int n, double v) : CtrlVal(f, v), id(n), type(ARVT_VAL) {}
      CtrlRecVal(unsigned int f, int n, double v, CtrlRecValueType t) : CtrlVal(f, v), id(n), type(t) {}
      };

//---------------------------------------------------------
//   CtrlRecList
//---------------------------------------------------------

class CtrlRecList : public std::list<CtrlRecVal> {
   public:
      };

typedef CtrlRecList::iterator iCtrlRec;
typedef CtrlRecList::const_iterator ciCtrlRec;

//---------------------------------------------------------
//   MidiAudioCtrlMap
//    Describes midi control of audio controllers
//---------------------------------------------------------

class MidiAudioCtrlStruct {
        int _audio_ctrl_id;
  public:
        MidiAudioCtrlStruct();
        MidiAudioCtrlStruct(int audio_ctrl_id);
        int audioCtrlId() const        { return _audio_ctrl_id; } 
        void setAudioCtrlId(int actrl) { _audio_ctrl_id = actrl; } 
      };
      
typedef uint32_t MidiAudioCtrlMap_idx_t;

typedef std::multimap<MidiAudioCtrlMap_idx_t, MidiAudioCtrlStruct, std::less<MidiAudioCtrlMap_idx_t> >::iterator iMidiAudioCtrlMap;
typedef std::multimap<MidiAudioCtrlMap_idx_t, MidiAudioCtrlStruct, std::less<MidiAudioCtrlMap_idx_t> >::const_iterator ciMidiAudioCtrlMap;

// Reverse lookup based on audio control.
typedef std::vector<iMidiAudioCtrlMap>::iterator iAudioMidiCtrlStructMap;
typedef std::vector<iMidiAudioCtrlMap>::const_iterator ciAudioMidiCtrlStructMap;
class AudioMidiCtrlStructMap : public std::vector<iMidiAudioCtrlMap> {
  public:
    
     };
    
// Midi to audio controller map.     
// The index is a hash of port, chan, and midi control number.     
class MidiAudioCtrlMap : public std::multimap<MidiAudioCtrlMap_idx_t, MidiAudioCtrlStruct, std::less<MidiAudioCtrlMap_idx_t> > {
  public:
      MidiAudioCtrlMap_idx_t index_hash(int midi_port, int midi_chan, int midi_ctrl_num) const; 
      void hash_values(MidiAudioCtrlMap_idx_t hash, int* midi_port, int* midi_chan, int* midi_ctrl_num) const; 
      iMidiAudioCtrlMap add_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, const MidiAudioCtrlStruct& amcs); 
      void find_audio_ctrl_structs(int audio_ctrl_id, AudioMidiCtrlStructMap* amcs); // const;
      void erase_ctrl_struct(int midi_port, int midi_chan, int midi_ctrl_num, int audio_ctrl_id);
      void write(int level, Xml& xml) const;
      void read(Xml& xml);
      };

      
//---------------------------------------------------------
//   CtrlList
//    arrange controller events of a specific type in a
//    list for easy retrieval
//---------------------------------------------------------

typedef std::map<unsigned int, CtrlVal, std::less<unsigned int> > CtrlList_t;
typedef std::pair<unsigned int, CtrlVal> CtrlListInsertPair_t;

class CtrlList : public CtrlList_t {
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
      volatile bool _guiUpdatePending; // Gui heartbeat routines read this. Checked and cleared in Song::beat().
      void initColor(int i);

   public:
      CtrlList(bool dontShow=false);
      CtrlList(int id, bool dontShow=false);
      CtrlList(int id, QString name, double min, double max, CtrlValueType v, bool dontShow=false);
      CtrlList(const CtrlList& l, int flags);
      void assign(const CtrlList& l, int flags); 

      void swap(CtrlList&);
      std::pair<iterator, bool> insert(const CtrlListInsertPair_t& p);
      iterator insert(iterator ic, const CtrlListInsertPair_t& p);
      void insert(iterator first, iterator last);
      void erase(iterator ictl);
      size_type erase(unsigned int frame);
      void erase(iterator first, iterator last);
      void clear();
      CtrlList& operator=(const CtrlList&);

      Mode mode() const          { return _mode; }
      void setMode(Mode m)       { _mode = m; }
      double getDefault() const   { return _default; }
      void setDefault(double val) { _default = val; }
      double curVal() const;
      void updateCurValue(unsigned int frame);
      void setCurVal(double val);
      int id() const             { return _id; }
      QString name() const       { return _name; }
      void setName(const QString& s) { _name = s; }
      double minVal() const { return _min; }
      double maxVal() const { return _max; }
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
      void getInterpolation(unsigned int frame, bool cur_val_only, CtrlInterpolate* interp);
      double interpolate(unsigned int frame, const CtrlInterpolate& interp);
      
      double value(unsigned int frame, bool cur_val_only = false,
                   unsigned int* nextFrame = NULL, bool* nextFrameValid = NULL) const;  
      void add(unsigned int frame, double value);
      void del(unsigned int frame);
      void read(Xml& xml);

      void setColor( QColor c ) { _displayColor = c;}
      QColor color() const { return _displayColor; }
      void setVisible(bool v) { _visible = v; }
      bool isVisible() const { return _visible; }
      bool dontShow() const { return _dontShow; }
      bool guiUpdatePending() const { return _guiUpdatePending; }
      void setGuiUpdatePending(bool v) { _guiUpdatePending = v; }
      };

typedef CtrlList::iterator iCtrl;
typedef CtrlList::const_iterator ciCtrl;

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
      MidiAudioCtrlMap _midi_controls;  // For midi control of audio controllers.
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
            
      MidiAudioCtrlMap* midiControls() { return &_midi_controls; }  
      
      double value(int ctrlId, unsigned int frame, bool cur_val_only = false,
                   unsigned int* nextFrame = NULL, bool* nextFrameValid = NULL) const;   
      void updateCurValues(unsigned int frame);
      void clearAllAutomation() {
            for(iCtrlList i = begin(); i != end(); ++i)
              i->second->clear();
           }     
      void write(int level, Xml& xml) const;
      };

extern double midi2AudioCtrlValue(const CtrlList* audio_ctrl_list, const MidiAudioCtrlStruct* mapper, int midi_ctlnum, int midi_val);

} // namespace MusECore

#endif

