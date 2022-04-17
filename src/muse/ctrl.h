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
#include <set>

#include <QColor>
#include <QString>
#include <QUuid>

#include <stdint.h>

#define AC_PLUGIN_CTL_BASE         0x1000
#define AC_PLUGIN_CTL_BASE_POW     12
#define AC_PLUGIN_CTL_ID_MASK      0xFFF

namespace MusECore {


// Forward declarations:
class Xml;
class Track;

const int AC_VOLUME = 0;
const int AC_PAN    = 1;
const int AC_MUTE   = 2;

inline unsigned long genACnum(unsigned long plugin, unsigned long ctrl) { return (plugin + 1) * AC_PLUGIN_CTL_BASE + ctrl; }

enum CtrlValueType { VAL_LOG, VAL_LINEAR, VAL_INT, VAL_BOOL, VAL_ENUM };
enum CtrlRecValueType { ARVT_VAL, ARVT_START, ARVT_STOP };
// Group end is valid only if selected is true.
// It is useful for dragging and dropping or copying and pasting xml.
// Group end is NOT managed automatically by the controller class. (Too complicated).
// It is generally only valid and to be trusted while we are in controller move mode,
//  ie. Song::_audioCtrlMoveModeBegun is true, since that is the only time we really need it.
// It is valid and trusted as well during XML pasting.
enum CtrlValueFlag { VAL_NOFLAGS = 0x00, VAL_SELECTED = 0x01, VAL_GROUP_END = 0x02 };
// A combination of CtrlValueFlag values that can be OR'd together.
typedef int CtrlValueFlags;

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
                      bool end_stop = false, bool do_interpolate = false);
      };

//---------------------------------------------------------
//   CtrlVal
//    controller "event"
//---------------------------------------------------------

class CtrlVal {
  private:
      double val;
      CtrlValueFlags _flags;

  public:
      CtrlVal();

      // Group end is valid only if selected is true.
      CtrlVal(double v, bool selected = false, bool groupEnd = true);
      CtrlValueFlags flags() const;
      void setFlags(CtrlValueFlags);
      bool selected() const;
      void setSelected(bool);
      bool groupEnd() const;
      void setGroupEnd(bool);
      double value() const;
      void setValue(double);
      };

//---------------------------------------------------------
//   CtrlRecVal
//    recorded controller event, mixer automation
//---------------------------------------------------------

struct CtrlRecVal {
      unsigned int frame;
      double val;

      int id;
      CtrlRecValueType type;   // 0 - ctrlVal, 1 - start, 2 - end
      CtrlRecVal(unsigned int f, int n, double v);
      CtrlRecVal(unsigned int f, int n, double v, CtrlRecValueType t);
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
        int audioCtrlId() const;
        void setAudioCtrlId(int actrl);
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


// Forward reference.
class PasteCtrlTrackMap;

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
      // PasteErase means erase target under the areas given if in PasteAreas mode.
      // PasteEraseRange means erase target full range from first to last paste item.
      enum PasteEraseOptions { PasteNoErase=0, PasteErase, PasteEraseRange, PasteEraseOptionsEnd };
   private:
      Mode _mode;
      // The controller id. This can be -1 meaning no particular id, for copy/paste etc.
      // Be careful if it is -1. For example it can't be added to a CtrlListList.
      int _id;
      double _default;
      double _curVal;
      QString _name;
      double _min, _max;  
      CtrlValueType _valueType;
      QColor _displayColor;
      bool _visible;
      bool _dontShow; // when this is true the control exists but is not compatible with viewing in the arranger
      volatile bool _guiUpdatePending; // Gui heartbeat routines read this. Checked and cleared in Song::beat().

   public:
      CtrlList(bool dontShow=false);
      CtrlList(int id, bool dontShow=false);
      CtrlList(int id, QString name, double min, double max, CtrlValueType v, bool dontShow=false);
      CtrlList(const CtrlList& l, int flags);
      CtrlList(const CtrlList& cl);

      void assign(const CtrlList& l, int flags); 

      //------------------------------------------------------------------------------
      // NOTICE: We override (hide) these intrinsic methods so that we may catch them.
      //         When newer C++ versions come out, additions and/or corrections
      //          may be required. Currently these (almost completely) cover C++17.
      //------------------------------------------------------------------------------
      void swap(CtrlList&) noexcept;
      std::pair<iterator, bool> insert(const CtrlListInsertPair_t& p);
      template< class P > std::pair<iterator, bool> insert(P&& value);
      std::pair<iterator, bool> insert(CtrlListInsertPair_t&& p);
      iterator insert(const_iterator ic, const CtrlListInsertPair_t& p);
      template< class P > iterator insert(const_iterator ic, P&& value);
      iterator insert(const_iterator hint, CtrlListInsertPair_t&& value);
      void insert(iterator first, iterator last);
      // TODO: Fix errors in code.
      //void insert(std::initializer_list<CtrlListInsertPair_t> ilist/*, bool doUpdateGroups = true*/);
      //insert_return_type insert(node_type&& nh/*, bool doUpdateGroups = true*/);
      //iterator insert(const_iterator ic, node_type&& nh/*, bool doUpdateGroups = true*/);
      template <class M> std::pair<iterator, bool> insert_or_assign(const unsigned int& k, M&& obj);
      template <class M> std::pair<iterator, bool> insert_or_assign(unsigned int&& k, M&& obj);
      template <class M> iterator insert_or_assign(const_iterator hint, const unsigned int& k, M&& obj);
      template <class M> iterator insert_or_assign(const_iterator hint, unsigned int&& k, M&& obj);
      iterator erase(iterator ictl);
      iterator erase(const_iterator ictl);
      iterator erase(const_iterator first, const_iterator last);
      size_type erase(unsigned int frame);
      void clear() noexcept;
      CtrlList& operator=(const CtrlList&);

      Mode mode() const;
      void setMode(Mode m);
      double getDefault() const;
      void setDefault(double val);
      double curVal() const;
      void updateCurValue(unsigned int frame);
      void setCurVal(double val);
      int id() const;
      void setId(int v);
      QString name() const;
      void setName(const QString& s);
      double minVal() const;
      double maxVal() const;
      void setRange(double min, double max);
      void range(double* min, double* max) const;
      CtrlValueType valueType() const;
      void setValueType(CtrlValueType t);
      void getInterpolation(unsigned int frame, bool cur_val_only, CtrlInterpolate* interp);
      double interpolate(unsigned int frame, const CtrlInterpolate& interp);
      
      double value(unsigned int frame, bool cur_val_only = false,
                   unsigned int* nextFrame = nullptr, bool* nextFrameValid = nullptr) const;
      std::pair<iterator, bool> add(unsigned int frame, double value, bool selected = false, bool groupEnd = true);
      void del(unsigned int frame);
      // Updates the entire container's group end markers, according to the current selection states
      //  of all the items. Returns true if anything changed.
      bool updateGroups();
      // Updates the container's group end markers around this one given frame, according to the current
      //  selection states of the items to the left and right of the frame and at the frame itself.
      // Returns true if anything changed.
      bool updateGroups(unsigned int frame);
      // Updates the container's group end markers around this one given pos, according to the current
      //  selection states of the items to the left and right of the pos frame and at the frame itself.
      // Returns true if anything changed.
      bool updateGroups(iterator);
      // The samplerate of the complete controller graph is given. Graph times are converted.
      void readValues(const QString& tag, const int samplerate);
      bool read(Xml& xml);
      void write(int level, Xml& xml, bool isCopy = false) const;

      void initColor(int i);
      void setColor( QColor c );
      QColor color() const;
      void setVisible(bool v);
      bool isVisible() const;
      bool dontShow() const;
      bool guiUpdatePending() const;
      void setGuiUpdatePending(bool v);
      };

typedef CtrlList::iterator iCtrl;
typedef CtrlList::const_iterator ciCtrl;
typedef std::pair<iCtrl, bool> CtrlListInsertResult_t;

//---------------------------------------------------------
//   CtrlListList
//    List of controller value lists.
//    This list represents the controller state of a
//    mixer strip
//---------------------------------------------------------

typedef std::map<int, CtrlList*, std::less<int> >::iterator iCtrlList;
typedef std::map<int, CtrlList*, std::less<int> >::const_iterator ciCtrlList;
typedef std::pair<int, CtrlList*> CtrlListListInsertPair_t;
typedef std::pair<iCtrlList, bool> CtrlListListInsertResult_t;

class CtrlListList : public std::map<int, CtrlList*, std::less<int> > {
   private:
      MidiAudioCtrlMap _midi_controls;  // For midi control of audio controllers.
   public:
      // Returns true if successfully added. Returns false if list already exists, or error.
      bool add(CtrlList* vl);
      // Returns true if something was deleted, false if id not found or error.
      bool del(int id);
      // Returns true if something was deleted, false if i at end or error.
      bool del(iCtrlList i);
      // Updates the entire list of container's group end markers, according to the current selection states
      //  of all the items. Returns true if anything changed.
      bool updateGroups();
      void clearDelete();
      iCtrlList find(int id);
      ciCtrlList find(int id) const;
            
      MidiAudioCtrlMap* midiControls();
      
      double value(int ctrlId, unsigned int frame, bool cur_val_only = false,
                   unsigned int* nextFrame = nullptr, bool* nextFrameValid = nullptr) const;
      void updateCurValues(unsigned int frame);
      void clearAllAutomation();
      void write(int level, Xml& xml) const;
      void initColors();
      };

//-------------------------------
//  AudioAutomationItem etc.
//  The following classes are useful for
//   both Canvas item graphics and Tag Lists.
//  So we include them here rather than in any
//   specific module.
//-------------------------------

typedef std::set<unsigned int /*frame*/> AudioAutomationOriginalFrameList;
typedef AudioAutomationOriginalFrameList::iterator iAudioAutomationOriginalFrameList;
typedef AudioAutomationOriginalFrameList::const_iterator ciAudioAutomationOriginalFrameList;

struct AudioAutomationItem {
    // The original value of the controller item.
    double _value;
    // Local working frame of the controller item.
    unsigned int _wrkFrame;
    // Local working value of the controller item.
    double _wrkVal;
    // Whether this item is the end of a group ie. the next item is unselected.
    bool _groupEnd;

    AudioAutomationItem();
    AudioAutomationItem(unsigned int frame, double value, bool groupEnd = false);
};

typedef std::map<unsigned int /*frame*/, AudioAutomationItem> AudioAutomationItemList;
typedef AudioAutomationItemList::iterator iAudioAutomationItemList;
typedef AudioAutomationItemList::const_iterator ciAudioAutomationItemList;
typedef std::pair<unsigned int /*frame*/, AudioAutomationItem> AudioAutomationItemListInsertPair;
typedef std::pair<iAudioAutomationItemList, bool> AudioAutomationItemListInsertResult;

struct AudioAutomationItemMapStruct {
    AudioAutomationItemList _selectedList;
};

class AudioAutomationItemMap : public std::map<CtrlList*, AudioAutomationItemMapStruct, std::less<CtrlList* >>
{
  public:
    // Returns true if insertion took place.
    // Returns false if assignment took place, or on error.
    bool addSelected(CtrlList*, unsigned int frame, const AudioAutomationItem&);
    // Returns true if deletion took place.
    // Returns false if no deletion took place (not found), or on error.
    bool delSelected(CtrlList*, unsigned int frame);
    // Clears all selected items in all controllers.
    // Returns true if clearing took place.
    // Returns false if no clearing took place, or on error.
    bool clearSelected();
    // Clears all selected items in a specific controller.
    // Returns true if clearing took place.
    // Returns false if no clearing took place (ctrl list not found), or on error.
    bool clearSelected(CtrlList*);
    // Returns true if any items in any controllers are selected.
    bool itemsAreSelected() const;
    // Returns true if any items in a specific controller are selected.
    // Returns false if ctrl list not found, or on error.
    bool itemsAreSelected(CtrlList*) const;
};
typedef AudioAutomationItemMap::iterator iAudioAutomationItemMap;
typedef AudioAutomationItemMap::const_iterator ciAudioAutomationItemMap;
typedef std::pair<CtrlList*, AudioAutomationItemMapStruct> AudioAutomationItemMapInsertPair;
typedef std::pair<iAudioAutomationItemMap, bool> AudioAutomationItemMapInsertResult;

class AudioAutomationItemTrackMap : public std::map<Track*, AudioAutomationItemMap, std::less<Track* >>
{
  public:
    // Returns true if insertion took place.
    // Returns false if assignment took place, or on error.
    bool addSelected(Track*, CtrlList*, unsigned int frame, const AudioAutomationItem&);
    // Returns true if deletion took place.
    // Returns false if no deletion took place (not found), or on error.
    bool delSelected(Track*, CtrlList*, unsigned int frame);
    // Clears all selected items in all controllers on all tracks.
    // Returns true if clearing took place.
    // Returns false if no clearing took place (track or ctrl list not found), or on error.
    bool clearSelected();
    // Clears all selected items in all controllers on a specific track.
    // Returns true if clearing took place.
    // Returns false if no clearing took place (track or ctrl list not found), or on error.
    bool clearSelected(Track*);
    // Clears all selected items in a specific controller on a specific track.
    // Returns true if clearing took place.
    // Returns false if no clearing took place (track or ctrl list not found), or on error.
    bool clearSelected(Track*, CtrlList*);
    // Returns true if any items in any controllers on any tracks are selected.
    bool itemsAreSelected() const;
    // Returns true if any items in any controllers on a specific track are selected.
    // Returns false if track not found, or on error.
    bool itemsAreSelected(Track*) const;
    // Returns true if any items in a specific controller on a specific track are selected.
    // Returns false if track or ctrl list not found, or on error.
    bool itemsAreSelected(Track*, CtrlList*) const;
};
typedef AudioAutomationItemTrackMap::iterator iAudioAutomationItemTrackMap;
typedef AudioAutomationItemTrackMap::const_iterator ciAudioAutomationItemTrackMap;
typedef std::pair<Track*, AudioAutomationItemMap> AudioAutomationItemTrackMapInsertPair;
typedef std::pair<iAudioAutomationItemTrackMap, bool> AudioAutomationItemTrackMapInsertResult;

struct PasteCtrlListStruct
{
  CtrlList _ctrlList;
  // The lowest time value of any items in the ctrl list.
  // If _ctrlList is not empty, this is valid.
  unsigned int _minFrame;
  PasteCtrlListStruct();
};

//---------------------------------------------------------
//   PasteCtrlListList
//    Special structure and container for pasting controllers
//---------------------------------------------------------

class PasteCtrlListList : public std::map<int /*ctrl id*/, PasteCtrlListStruct>
{
  public:
    // The lowest time value of any items in any of the ctrl lists.
    // If the container is not empty, this is valid.
    unsigned int _minFrame;
    PasteCtrlListList();
    // Returns true if insertion took place.
    bool add(int ctrlId, const PasteCtrlListStruct&);
};
typedef PasteCtrlListList::iterator iPasteCtrlListList;
typedef PasteCtrlListList::const_iterator ciPasteCtrlListList;
typedef std::pair<int /*ctrl id*/, PasteCtrlListStruct> PasteCtrlListListInsertPair;
typedef std::pair<iPasteCtrlListList, bool> PasteCtrlListListInsertResult;

class PasteCtrlTrackMap : public std::map<QUuid /*track uuid*/, PasteCtrlListList, std::less<QUuid >>
{
  public:
    // The lowest time value of any items in any of the ctrl lists in any of the tracks.
    // If the container is not empty, this is valid.
    unsigned int _minFrame;
    PasteCtrlTrackMap();
    // Returns true if insertion took place.
    bool add(const QUuid& trackUuid, const PasteCtrlListList&);
};
typedef PasteCtrlTrackMap::iterator iPasteCtrlTrackMap;
typedef PasteCtrlTrackMap::const_iterator ciPasteCtrlTrackMap;
typedef std::pair<const QUuid& /*track uuid*/, PasteCtrlListList> PasteCtrlTrackMapInsertPair;
typedef std::pair<iPasteCtrlTrackMap, bool> PasteCtrlTrackMapInsertResult;


extern double midi2AudioCtrlValue(const CtrlList* audio_ctrl_list, const MidiAudioCtrlStruct* mapper, int midi_ctlnum, int midi_val);

} // namespace MusECore

#endif

