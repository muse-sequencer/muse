//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: plugin.h,v 1.9.2.13 2009/12/06 01:25:21 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <list>
#include <vector>
#include <map>
#include <QSet>
#include <QMap>
#include <QPair>
#include <QFileInfo>
#include <QMainWindow>
#include <QMouseEvent>
#include <QUiLoader>
#include <QRect>
#include <QList>
#include <QMetaObject>

#include <ladspa.h>

#include "globaldefs.h"
#include "ctrl.h"
#include "controlfifo.h"
#include "config.h"
// REMOVE Tim. tmp. Added.
#include "stringparam.h"
#include "type_defs.h"
#include "xml.h"
// REMOVE Tim. tmp. Added. Moved from cpp file.
#include "plugin_scan.h"

#ifdef OSC_SUPPORT
#include "osc.h"
#endif

#ifdef DSSI_SUPPORT
#include <dssi.h>
#endif

// BUG I have filed a Qt bug report 64773. If we do NOT
//  set a position on a new QWidget, Qt seems to take control
//  of the position management such that it will NOT accept
//  any new position after that! It seems to decide to
//  remain in 'auto-placement' mode forever.
// If the bug is ever fixed by Qt, this define should then
//  become version-sensitive.         2017/11/28 Tim.
#define QT_SHOW_POS_BUG_WORKAROUND 1;


// Forward declarations:
class QScrollArea;
class QShowEvent;
class QHideEvent;
class QAction;
class QSpinBox;
class QToolButton;

namespace MusEGui {
class PluginGui;
class DoubleLabel;
class DoubleText;
}

// REMOVE Tim. tmp. Removed.
// namespace MusEPlugin {
// class PluginScanInfoStruct;
// }

namespace MusECore {
class AudioTrack;
// class Xml;

class PluginI;
struct SongChangedStruct_t;

// REMOVE Tim. tmp. Added.
//---------------------------------------------------------
//   PluginBase
//---------------------------------------------------------

class PluginBase {
   protected:
      MusEPlugin::PluginType _pluginType;
      MusEPlugin::PluginClass_t _pluginClass;
// REMOVE Tim. tmp. Added
   // public:
   //    //==============================================
   //    // These are original and translated strings
   //    //  representing the plugin types and classes.
   //    // The translated strings can be used for display.
   //    // Do NOT use them for storage in song files.
   //    // For that, use the original untranslated strings.
   //    // NOTE: Please ensure synchronization between them
   //    //  and the enumerations.
   //    //==============================================
   //    static const std::map<int, const char*> PluginTypeStrings;
   //    static MusEPlugin::PluginType pluginStringToType(const char *s);
   //    static QString pluginTypeToTranslatedString(const MusEPlugin::PluginType type);
   //
   //    static const std::map<int, const char*> PluginClassStrings;
   //    static QString pluginClassToTranslatedString(const MusEPlugin::PluginClass_t c);

   protected:
      unsigned long _uniqueID;
      void* _handle;
      int _references;
      QFileInfo _fileInfo;
      // Universal resource identifier, for plugins that use it (LV2).
      // If this exists, it should be used INSTEAD of file info, for comparison etc.
      // Never rely on the file info if a uri exists.
      QString _uri;
      QString _name;
      QString _label;
      QString _description;
      QString _maker;
      QString _version;
      QString _copyright;

      MusEPlugin::PluginFeatures_t _requiredFeatures;

      bool _usesTimePosition;
      unsigned long _freewheelPortIndex;
      unsigned long _latencyPortIndex;
      unsigned long _enableOrBypassPortIndex;
      MusEPlugin::PluginLatencyReportingType _pluginLatencyReportingType;
      MusEPlugin::PluginBypassType _pluginBypassType;
      MusEPlugin::PluginFreewheelType _pluginFreewheelType;

   public:
      PluginBase();
      PluginBase(const MusEPlugin::PluginScanInfoStruct&);
      virtual ~PluginBase();

      static void dump(const MusEPlugin::PluginScanInfoStruct& info, const char* prefixMessage);

      MusEPlugin::PluginType pluginType() const;
      MusEPlugin::PluginClass_t pluginClass() const;

      virtual MusEPlugin::PluginFeatures_t requiredFeatures() const;
      QString uri() const;
      QString name() const;
      QString maker() const;
      QString description() const;
      QString version() const;
      QString label() const;
      QString copyright() const;
      QString completeBaseName() const;
      QString baseName() const;
      QString absolutePath() const;
      QString path() const;
      QString filePath() const;
      QString fileName() const;

      unsigned long id() const;

      int references() const;
      virtual int incReferences(int) = 0;

      // Returns true if ANY of the midi input ports uses time position (transport).
      bool usesTimePosition() const;
      unsigned long freewheelPortIndex() const;
      unsigned long latencyPortIndex() const;
      unsigned long enableOrBypassPortIndex() const;
      MusEPlugin::PluginLatencyReportingType pluginLatencyReportingType() const;
      MusEPlugin::PluginBypassType pluginBypassType() const;
      MusEPlugin::PluginFreewheelType pluginFreewheelType() const;

      // Returns the plugin latency, if it has such as function.
      // NOTE: If the plugin has a latency controller out, use that instead.
      virtual float getPluginLatency(void* /*handle*/);
};

//---------------------------------------------------------
//   Plugin
//---------------------------------------------------------

class Plugin : public PluginBase {

   protected:
   friend class PluginI;

//       void* _handle;
//       int _references;
      int _instNo;
//       QFileInfo _fileInfo;
//       // Universal resource identifier, for plugins that use it (LV2).
//       // If this exists, it should be used INSTEAD of file info, for comparison etc.
//       // Never rely on the file info if a uri exists.
//       QString _uri;
      LADSPA_Descriptor_Function ladspa;
      const LADSPA_Descriptor *plugin;
//       unsigned long _uniqueID;
//       QString _label;
//       QString _name;
//       QString _maker;
//       QString _copyright;

//       bool _isDssiSynth;
//       bool _isDssi;
//       bool _isLV2Synth;
//       bool _isLV2Plugin;
//       // Hack: Special flag required.
//       bool _isDssiVst;
//       bool _isVstNativeSynth;
//       bool _isVstNativePlugin;

      #ifdef DSSI_SUPPORT
      const DSSI_Descriptor* dssi_descr;
      #endif

      unsigned long _portCount;
      unsigned long _inports;
      unsigned long _outports;
      unsigned long _controlInPorts;
      unsigned long _controlOutPorts;
      std::vector<unsigned long> rpIdx; // Port number to control input index. Item is -1 if it's not a control input.

//       bool _usesTimePosition;
//       unsigned long _freewheelPortIndex;
//       unsigned long _latencyPortIndex;
//       unsigned long _enableOrBypassPortIndex;
//       MusEPlugin::PluginLatencyReportingType _pluginLatencyReportingType;
//       MusEPlugin::PluginBypassType _pluginBypassType;
//       MusEPlugin::PluginFreewheelType _pluginFreewheelType;

//       MusEPlugin::PluginFeatures_t _requiredFeatures;

   public:
      Plugin();
      Plugin(const MusEPlugin::PluginScanInfoStruct&);
      virtual ~Plugin();
//      virtual MusEPlugin::PluginFeatures_t requiredFeatures() const;
//       QString uri() const;
//       virtual QString label() const;
//       QString name() const;
//       unsigned long id() const;
//       QString maker() const;
//       QString copyright() const;
      QString lib(bool complete = true) const;
      QString dirPath(bool complete = true) const;
//       QString filePath() const;
//       QString fileName() const;
        
//       int references() const;
      virtual int incReferences(int val) override;
      int instNo();

// REMOVE Tim. tmp. Removed.
//       inline bool isDssiPlugin() const { return _isDssi; }
//       inline bool isDssiSynth() const  { return _isDssiSynth; }
//       inline bool isDssiVst() const  { return _isDssiVst; }
//       inline bool isLV2Plugin() const { return _isLV2Plugin; } //inline it to use in RT audio thread
//       inline bool isLV2Synth() const { return _isLV2Synth; }
//       inline bool isVstNativePlugin() const { return _isVstNativePlugin; } //inline it to use in RT audio thread
//       inline bool isVstNativeSynth() const { return _isVstNativeSynth; }

      virtual LADSPA_Handle instantiate(PluginI *);
      virtual void activate(LADSPA_Handle handle);
      virtual void deactivate(LADSPA_Handle handle);
      virtual void cleanup(LADSPA_Handle handle);
      virtual void connectPort(LADSPA_Handle handle, unsigned long port, float* value);
      virtual void apply(LADSPA_Handle handle, unsigned long n, float /*latency_corr*/ = 0.0f);

      #ifdef OSC_SUPPORT
      int oscConfigure(LADSPA_Handle handle, const char* key, const char* value);
      #endif

      unsigned long ports();

      virtual LADSPA_PortDescriptor portd(unsigned long k) const;

      // This version of range does not apply any changes, such as sample rate, to the bounds.
      // The information returned is verbose. See the other range() which does apply changes.
      virtual LADSPA_PortRangeHint range(unsigned long i) const;

      virtual double defaultValue(unsigned long port) const;
      // This version of range applies any changes, such as sample rate, to the bounds.
      // The information returned is not verbose. See the other range() which does not apply changes.
      virtual void range(unsigned long i, float*, float*) const;
      virtual CtrlValueType ctrlValueType(unsigned long i) const;
      virtual CtrlList::Mode ctrlMode(unsigned long i) const;
      virtual const CtrlVal::CtrlEnumValues* ctrlEnumValues ( unsigned long ) const;
      // Returns a value unit string for displaying unit symbols.
      virtual QString unitSymbol ( unsigned long ) const;
      // Returns index into the global value units for displaying unit symbols.
      // Can be -1 meaning no units.
      virtual int valueUnit ( unsigned long ) const;

      virtual const char* portName(unsigned long i) const;

//       bool usesTimePosition() const;
//       unsigned long freewheelPortIndex() const;
//       unsigned long latencyPortIndex() const;
//       unsigned long enableOrBypassPortIndex() const;
//       MusEPlugin::PluginLatencyReportingType pluginLatencyReportingType() const;
//       MusEPlugin::PluginBypassType pluginBypassType() const;
//       MusEPlugin::PluginFreewheelType pluginFreewheelType() const;

//       // Returns the plugin latency, if it has such as function.
//       // NOTE: If the plugin has a latency controller out, use that instead.
//       virtual float getPluginLatency(void* /*handle*/);

      unsigned long inports() const;
      unsigned long outports() const;
      unsigned long controlInPorts() const;
      unsigned long controlOutPorts() const;

      const std::vector<unsigned long>* getRpIdx();
      };

typedef std::list<Plugin *>::iterator iPlugin;
typedef std::list<Plugin *>::const_iterator ciPlugin;


class PluginGroups : public QMap< QPair<QString, QString>, QSet<int> >
{
  public:
    QSet<int>& get(QString a, QString b);
    QSet<int>& get(const Plugin *p);
    void shift_left(int first, int last);
    void shift_right(int first, int last);
    void erase(int index);

  private:
    void replace_group(int old, int now);
};


//---------------------------------------------------------
//   PluginList
//---------------------------------------------------------

class PluginList : public std::list<Plugin *> {
   public:
      void add(const MusEPlugin::PluginScanInfoStruct& scan_info);
      // Each argument optional, can be empty.
// REMOVE Tim. tmp. Changed.
//       // If uri is not empty, the search is based solely on it, the other arguments are ignored.
//       Plugin* find(const QString& file, const QString& uri, const QString& label) const;
      // If uri is not empty, file and label are ignored.
      Plugin* find(
        MusEPlugin::PluginType type, const QString& file,
        const QString& uri, const QString& label) const;
      PluginList();
      };

//---------------------------------------------------------
//   Port
//---------------------------------------------------------

struct Port {
      unsigned long idx;
      
      //   NOTE: These values represent the lowest level of control value storage.
      //         The choice of float or double depends on the underlying system using this struct.
      //         For example plugins and synthesizers usually use floats to represent control values
      //          and they are directly pointed to this float, while our own track controls 
      //          (volume, pan etc.) take advantage of the double precision.
      //         Double precision is preferred if possible because above this lowest level all other 
      //          controller usage is in double precision. Thus our very own track controllers are 
      //          perfectly matched double precision throughout the system.
      union {
        float val;
        double dval;
        };
// REMOVE Tim. tmp. Removed.
      // union {
        // float tmpVal; // TODO Try once again and for all to remove this, was it really required?
        //double dtmpVal; // Not used, should not be required.
        // };
      
      bool enCtrl;  // Enable controller stream.
      CtrlInterpolate interp;
      };

//---------------------------------------------------------
//   PluginQuirks
//    An ugly but necessary class due to differences in plugin behaviour.
//---------------------------------------------------------

class PluginQuirks
{
public:
    enum NatUISCaling {GLOBAL, ON, OFF};
    // Whether the LV2 'speed' timePosition property switches to 0.0 in stop mode, or remains
    //  fixed at 1.0 regardless of play or stop mode. Fixes plugins like TAL NoiseMak3r
    //  stuck repeating small modulator segment at speed = 0.0.
    bool _fixedSpeed;
    // If the plugin uses our transport (ex. as an LV2 timePosition), sets whether the timePosition
    //  affects actual audio latency (not just say, beat or sequencer or arpeggiator latency).
    // Example: LV2 Example Metronome.
    bool _transportAffectsAudioLatency;
    // Override the plugin's reported latency. Some plugins get it wrong or not quite right.
    bool _overrideReportedLatency;
    // Value to override the reported latency.
    int _latencyOverrideValue;

  PluginQuirks();

  void write(int level, Xml& xml) const;
  // Return true on error.
  bool read(Xml& xml);
  bool fixNativeUIScaling() const;
  void setFixNativeUIScaling(NatUISCaling fixScaling);
  NatUISCaling getFixNativeUIScaling() const;

private:
  // Reverse scaling of native UI windows on HiDPI
  NatUISCaling _fixNativeUIScaling;
};

typedef enum {
    PROP_NONE = -1, PROP_INT = 0, PROP_LONG, PROP_FLOAT, PROP_DOUBLE, PROP_BOOL, PROP_STRING, PROP_PATH
} PropType;


// REMOVE Tim. tmp. Added.
struct PluginControlConfig
{
//   enum WriteOption
//   {
//     WriteSongVer4 = 0,
//     WriteSongVerPre4MissingEffect,
//     WriteSongVerPre4MissingSynth
//   };

  // Valid members. Can be OR'd together.
  enum ValidMember
  {
    NoneValid      = 0x0,
    CtlNumValid    = 0x1,
    NameValid      = 0x2,
    MinValid       = 0x4,
    MaxValid       = 0x8,
    ValueTypeValid = 0x10,
    CtlModeValid   = 0x20,
    ValueUnitValid = 0x40,
    AllValid = CtlNumValid | NameValid | MinValid | MaxValid | ValueTypeValid | CtlModeValid | ValueUnitValid,
  };
  typedef int ValidMembers;

  ValidMembers _validMembers;

//   // Can be -1. The 'ctl' songfile xml tag attribute was added version 4.0. It defaults to -1 for older files.
  // Must be >= 0, because the container is a map. The 'ctl' songfile xml tag attribute was added version 4.0.
  int _ctlnum;
  QString _name;
  float _val;
  float _min, _max;
  MusECore::CtrlValueType _valueType;
  CtrlList::Mode _ctlMode;
  // Index into the global value units for displaying unit symbols.
  // Can be -1 meaning no units.
  int _valueUnit;
  PluginControlConfig();
  PluginControlConfig(int ctlnum, const QString& name,
    float val, float min, float max,
    MusECore::CtrlValueType valueType, CtrlList::Mode mode, int valueUnit,
    ValidMembers validMembers);

//   // Returns true on error.
//   // If no 'ctl' attribute is found (song file versions < 4), give it defaultCtlNum if not -1.
//   // In that case we know that when the song was saved, ALL controls were stored and they were ALL
//   //  stored in order starting at control 0. Hand-constructed out-of-order entries are not supported.
//   bool read(Xml& xml, int defaultCtlNum = -1);

  // Returns true on error.
  // If fileVerMaj (and fileVerMin) are valid, it means while loading there was no plugin found
  //  and the file version is set to the original version. We need that info for the saving code
  //  to determine what exactly to save, for persistence.
  // It is only when the plugin is finally found that the plugin configuration structure's
  //  version maj and min are reset to -1.
//   bool write(int level, Xml& xml, WriteOption option = WriteSongVer4) const;
  bool write(int level, Xml& xml) const;
};

// REMOVE Tim. tmp. Added.
// Plugin control (parameter) properties.
class PluginControlList : public std::map<int, PluginControlConfig>
{
  public:
    // Returns true on error.
    // If no 'ctl' attribute is found (song file versions < 4), it is set to the increasing current size.
    // In that case we know that when the song was saved, ALL controls were stored and they were ALL
    //  stored in order starting at control 0. Hand-constructed out-of-order entries are not supported.
    bool read(Xml& xml);
    // Returns true on error.
    // If fileVerMaj (and fileVerMin) are valid, it means while loading there was no plugin found
    //  and the file version is set to the original version. We need that info for the saving code
    //  to determine what exactly to save, for persistence.
    // It is only when the plugin is finally found that the plugin configuration structure's
    //  version maj and min are reset to -1.
//     bool write(int level, Xml& xml, PluginControlConfig::WriteOption = PluginControlConfig::WriteSongVer4) const;
    bool write(int level, Xml& xml) const;
};
typedef PluginControlList::iterator iPluginControlList;
typedef PluginControlList::const_iterator ciPluginControlList;
typedef std::pair<PluginControlList::iterator, bool> PluginControlListInsertResult;


// REMOVE Tim. tmp. Added.
//---------------------------------------------------------
//   PluginConfiguration
//---------------------------------------------------------

class PluginConfiguration
{
public:
  MusEPlugin::PluginType _pluginType;
  //MusEPlugin::PluginClass_t _pluginClass;

  // The major and minor version of the song file that a plugin was created in - even if
  //  that file has been re-saved and now has a later version.
  // Can be -1 meaning don't care.
  // They are only valid and used if loading an older song file and plugins are missing.
  // Upon saving the file again, for persistent settings we need this info to determine
  //  what (not) to save in the file. The saved file will have a newer version and format,
  //  but still needs to retain this info until the plugins are eventually found.
  int _fileVerMaj, _fileVerMin;

  QString _file;
  QString _uri;
  QString _pluginLabel;
  QString _name;
  int _id;
  QRect _geometry;
  QRect _nativeGeometry;
  bool _guiVisible;
  bool _nativeGuiVisible;
  bool _on;
  bool _active;
  PluginQuirks _quirks;

  // List of initial floating point parameters, for plugins which use them.
//   std::vector<ControlConfig> _initParams;
  PluginControlList _initParams;

  // Custom params in xml song file.
  std::vector<QString> _accumulatedCustomParams;

  // Initial, and running, string parameters for plugins which use them, like dssi.
  StringParamMap _stringParamMap;

  // Optional automation controller list. Used for drag-copy.
  CtrlListList _ctrlListList;

  // Optional midi assignment list. Used for drag-copy.
  MidiAudioCtrlMap _midiAudioCtrlMap;

  PluginConfiguration();
  virtual ~PluginConfiguration();

  // // Returns true on error.
  // // In case no 'ctl' tag was found, give it defaultCtlNum if not -1.
  // // Tag 'ctl' was added in song file version 4.
  // // Class Plugin did not have the tag, and class Synth had even less info, storing only
  // //  the 'param=value' tag. That has now been changed to a full 'control' tag.
  // // Previously we looked up the control by name only.
  // // This would be flawed if two or more control names were the same, like blank or 'unused' etc.
  // //  which HAS been observed in some plugins.
  // // So we assume, as has always been the case, that when the app saved a song, ALL controls were
  // //  stored and they were ALL stored in order starting at control 0.
  // // However it is possible that a song file could have been hand-constructed and the controls were
  // //  stored out of order, which would still work, but it should have been known that the controls
  // //  were to be stored in order.
  // bool loadControl(Xml& xml, int defaultCtlNum = -1);
  // // Returns true on error.
  // bool writeControls(int level, Xml& xml) const;

  void writeProperties(int level, Xml& xml, bool isCopy, bool isFakeName, const Track *track = nullptr) const;

  // Handles tag values and attributes. Returns true if the tag was handled, false if not.
  bool readProperties(Xml&, const Xml::Token& token);
};

// REMOVE Tim. tmp. Added.
//---------------------------------------------------------
//   MissingPluginStruct
//   Structure and list to hold plugins not found during loading
//---------------------------------------------------------

struct MissingPluginStruct
{
  //Synth::Type _type;
  //QString _class;
  QString _file;
  QString _uri;
  QString _label;
  int _effectInstCount;
  int _synthInstCount;
  int _effectInstNo;
  int _synthInstNo;

  MissingPluginStruct();
  int effectInstNo();
  int synthInstNo();
};

class MissingPluginList : public std::vector<MissingPluginStruct>
{
  public:
    // Each argument optional, can be empty.
    // If uri is not empty, the search is based solely on it, the other arguments are ignored.
    iterator find(const QString& file, const QString& uri, const QString& label);
    // Returns the fake instance number.
    MissingPluginStruct& add(const QString& file, const QString& uri, const QString& label, bool isSynth /* vs. rack effect */);
};
//typedef std::vector<MissingPluginStruct>::iterator iMissingPluginList;
//typedef std::vector<MissingPluginStruct>::const_iterator ciMissingPluginList;


//---------------------------------------------------------
//   PluginIBase
//---------------------------------------------------------

class PluginIBase
{
// REMOVE Tim. tmp. Added.
   public:
      // Options when calling configure(). Can be OR'd together.
      // Determines what exactly to configure from the configuration structure.
      // Note that ConfigParams is ignored if ConfigCustomData is set and there is custom data.
      // If ConfigNativeGui is set, then ConfigDeferNativeGui determines whether
      //  the native gui attempts to open immediately or opening is deferred by setting
      //  the _showNativeGuiPending flag so that the native gui is opened later.
      enum ConfigureOption {
        ConfigNone = 0x0,
        ConfigActive = 0x1,
        ConfigOn = 0x2,
        ConfigQuirks = 0x4,
        ConfigCustomData = 0x8,
        ConfigParams = 0x10,
        ConfigGui = 0x20,
        ConfigNativeGui = 0x40,
        ConfigDeferNativeGui = 0x80,
        ConfigGeometry = 0x100,
        ConfigNativeGeometry = 0x200,
        ConfigPresetOnly = ConfigCustomData | ConfigParams,
        ConfigAll = ConfigActive | ConfigOn | ConfigQuirks | ConfigCustomData | ConfigParams |
          ConfigGui | ConfigNativeGui | ConfigGeometry | ConfigNativeGeometry
      };
      // Collection of ConfigureOptions.
      typedef int ConfigureOptions_t;

   protected:
      ControlFifo _controlFifo;
      MusEGui::PluginGui* _gui;
      QRect _guiGeometry;
      QRect _nativeGuiGeometry;
      PluginQuirks _quirks;
      // True if activate has been called. False by default or if deactivate has been called.
      bool _curActiveState;
      bool _showNativeGuiPending;

      void makeGui();

      virtual void activate() = 0;
      virtual void deactivate() = 0;

   public:
      PluginIBase();
      virtual ~PluginIBase();
      virtual MusEPlugin::PluginFeatures_t requiredFeatures() const = 0;
      inline virtual bool hasActiveButton() const = 0;
      inline virtual bool active() const = 0;
      inline virtual void setActive(bool v) = 0;
      inline virtual bool hasBypass() const = 0;
      virtual bool on() const = 0;
      virtual void setOn(bool val) = 0;
      virtual unsigned long pluginID() const = 0;
      virtual int id() const = 0;
// REMOVE Tim. tmp. Added.
      virtual QString name() const = 0;
      virtual QString pluginLabel() const = 0;
      virtual QString pluginName() const = 0;
      virtual QString uri() const = 0;
      virtual QString lib() const = 0;
      virtual QString dirPath() const = 0;
      virtual QString fileName() const = 0;
      virtual QString titlePrefix() const = 0;

      virtual AudioTrack* track() const = 0;

      virtual void enableController(unsigned long i, bool v = true) = 0;
      virtual bool controllerEnabled(unsigned long i) const = 0;
      virtual void enableAllControllers(bool v = true) = 0;
      virtual void updateControllers() = 0;

// REMOVE Tim. tmp. Added.
      // virtual PluginConfiguration getConfiguration() const = 0;
      // If isCopy is true, writes additional info including automation controllers and midi assignments.
      virtual void writeConfiguration(int level, Xml& xml, bool isCopy = false) = 0;
// REMOVE Tim. tmp. Changed.
//      virtual bool readConfiguration(Xml& xml, bool readPreset=false) = 0;
      // If reading a preset, set readPreset true. Then it will read some things but not others. Channels not required.
      // If not reading a preset (reading and creating a plugin), the number of channels to create is required.
      virtual bool readConfiguration(Xml& xml, bool readPreset=false, int channels=0) = 0;

      virtual bool addScheduledControlEvent(unsigned long i, double val, unsigned frame);    // returns true if event cannot be delivered
      virtual unsigned long parameters() const = 0;
      virtual unsigned long parametersOut() const = 0;
      virtual void setParam(unsigned long i, double val) = 0;
      virtual double param(unsigned long i) const = 0;
      virtual double paramOut(unsigned long i) const = 0;
      virtual const char* paramName(unsigned long i) const = 0;
      virtual const char* paramOutName(unsigned long i) const = 0;
      // These versions of range do not apply any changes, such as sample rate, to the bounds.
      // The information returned is verbose. See the other range() which does apply changes.
      // FIXME TODO: Either find a way to agnosticize these two ranges, or change them from ladspa ranges to a new MusE range class.
      virtual LADSPA_PortRangeHint range(unsigned long i) const = 0;
      virtual LADSPA_PortRangeHint rangeOut(unsigned long i) const = 0;
      // These versions of range apply any changes, such as sample rate, to the bounds.
      // The information returned is not verbose. See the other range() which does not apply changes.
      virtual void range(unsigned long i, float*, float*) const = 0;
      virtual void rangeOut(unsigned long i, float*, float*) const = 0;

      virtual bool usesTransportSource() const = 0;
      virtual unsigned long latencyOutPortIndex() const = 0;
      virtual float latency() const = 0;
      virtual unsigned long freewheelPortIndex() const = 0;
      virtual unsigned long enableOrBypassPortIndex() const = 0;
      virtual MusEPlugin::PluginLatencyReportingType pluginLatencyReportingType() const = 0;
      virtual MusEPlugin::PluginBypassType pluginBypassType() const = 0;
      virtual MusEPlugin::PluginFreewheelType pluginFreewheelType() const = 0;

      const PluginQuirks& cquirks() const;
      PluginQuirks& quirks();
// REMOVE Tim. tmp. Added.
      void setQuirks(const PluginQuirks&);

      // Returns true if, among other data, there was indeed custom data.
      // This means there is, or likely is, parameter values stored with the data,
      //  meaning we should not try to manually restore parameters since the data
      //  already has them.
      virtual bool setCustomData(const std::vector<QString> &);
      virtual CtrlValueType ctrlValueType(unsigned long i) const = 0;
      virtual CtrlList::Mode ctrlMode(unsigned long i) const = 0;
      virtual const CtrlVal::CtrlEnumValues *ctrlEnumValues(unsigned long i) const;
      virtual QString portGroup(long unsigned int i) const;
      virtual bool ctrlIsTrigger(long unsigned int i) const;
      virtual bool ctrlNotOnGui(long unsigned int i) const;
      virtual CtrlValueType ctrlOutValueType(unsigned long i) const = 0;
      virtual CtrlList::Mode ctrlOutMode(unsigned long i) const = 0;
      virtual const CtrlVal::CtrlEnumValues *ctrlOutEnumValues(unsigned long i) const;
      virtual QString portGroupOut(long unsigned int i) const;
      virtual bool ctrlOutIsTrigger(long unsigned int i) const;
      virtual bool ctrlOutNotOnGui(long unsigned int i) const;
      // Returns a value unit string for displaying unit symbols.
      virtual QString unitSymbol(long unsigned int i) const;
      virtual QString unitSymbolOut(long unsigned int i) const;
      // Returns index into the global value units for displaying unit symbols.
      // Can be -1 meaning no units.
      virtual int valueUnit ( unsigned long i) const;
      virtual int valueUnitOut ( unsigned long i) const;
// REMOVE Tim. tmp. Added.
      // Sets up a list of existing controllers based on info gathered from either the plugin
      //  or the plugin's persistent info if the plugin is missing.
      // Sets various controller properties such as names, ranges, value types, and value units.
//       virtual bool setupControllers(CtrlListList *cll) const = 0;

      QString dssi_ui_filename() const;

      MusEGui::PluginGui* gui() const;
      void deleteGui();
      void updateGuiWindowTitle() const;
      virtual void guiHeartBeat();
      virtual void showGui();
      virtual void showGui(bool);
      virtual bool guiVisible() const;
      // Sets the gui's geometry. Also updates the saved geometry.
      virtual void setGeometry(int x, int y, int w, int h);
      // Returns the current geometry of the gui, or if the gui does not exist, 
      //  the saved gui geometry.
      virtual void getGeometry(int *x, int *y, int *w, int *h) const;
      // Saves the current gui geometry.
      virtual void saveGeometry(int x, int y, int w, int h);
      // Returns the saved gui geometry.
      virtual void savedGeometry(int *x, int *y, int *w, int *h) const;

// REMOVE Tim. tmp. Added.
      virtual void updateNativeGuiWindowTitle();
      virtual void showNativeGui();
      virtual void showNativeGui(bool);
// REMOVE Tim. tmp. Added.
      // Sets a flag that defers opening the native gui until a later time.
      // Until, for example, after a track has been added to a track list,
      //  in the case of OSC which needs to find a track in a list before
      //  it can open a UI.
      virtual void showNativeGuiPending(bool);
      virtual bool isShowNativeGuiPending() const;
// REMOVE Tim. tmp. Added.
      virtual void closeNativeGui();
      virtual bool nativeGuiVisible() const;
      // Sets the gui's geometry. Also updates the saved geometry.
      virtual void setNativeGeometry(int x, int y, int w, int h);
      // Returns the current geometry of the gui, or if the gui does not exist, 
      //  the saved gui geometry.
      virtual void getNativeGeometry(int *x, int *y, int *w, int *h) const;
      // Saves the current gui geometry.
      virtual void saveNativeGeometry(int x, int y, int w, int h);
      // Returns the saved gui geometry.
      virtual void savedNativeGeometry(int *x, int *y, int *w, int *h) const;
};

//---------------------------------------------------------
//   PluginI
//    plugin instance
//---------------------------------------------------------

#define IS_AUDIO_IN (LADSPA_PORT_AUDIO  | LADSPA_PORT_INPUT)
#define IS_AUDIO_OUT (LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT)

class PluginI : public PluginIBase {
#ifdef LV2_SUPPORT
    friend class LV2PluginWrapper;
    friend class LV2Synth;    
#endif
#ifdef VST_NATIVE_SUPPORT
    friend class VstNativeSynth;
    friend class VstNativePluginWrapper;
#endif
      Plugin* _plugin;
// REMOVE Tim. tmp. Removed.
      // int channel;
      int instances;
      AudioTrack* _track;
      int _id;

      LADSPA_Handle* handle;         // per instance
      Port* controls;
      Port* controlsOut;
      Port* controlsOutDummy;

      unsigned long controlPorts;      
      unsigned long controlOutPorts;    

      float *_audioInSilenceBuf; // Just all zeros all the time, so we don't have to clear for silence.
      float *_audioOutDummyBuf;  // A place to connect unused outputs.
      
      bool _active;
      bool _on;
// REMOVE Tim. tmp. Removed.
//      bool initControlValues;
      QString _name;
//      QString _label;
// REMOVE Tim. tmp. Added.
      // Internal flag. If no name was found, a fake name based on label, which is not ideal, will be used.
      // This flag indicates a fake name was chosen. It is used when saving XML to leave out the fake name.
      // NOTE: If we ever add a setName() function to allow the user to change the plugin name,
      //        this should be reset to false upon setting a name.
      bool _isFakeName;

// REMOVE Tim. tmp. Added.
      // Holds initial controller values, parameters, sysex, custom data etc. for plugins which use them.
      PluginConfiguration _initConfig;

      #ifdef OSC_SUPPORT
      OscEffectIF _oscif;
      #endif
// REMOVE Tim. tmp. Removed.
//       bool _showNativeGuiPending;

      void init();

   protected:
      void activate();
      void deactivate();

   public:
      PluginI();
      virtual ~PluginI();

// REMOVE Tim. tmp. Added.
      // Returns nullptr if failure.
      static PluginI* createPluginI(const PluginConfiguration &config, int channels, ConfigureOptions_t opts);

      Plugin* plugin() const;

      virtual MusEPlugin::PluginFeatures_t requiredFeatures() const;

      inline bool hasActiveButton() const { return true; }
      inline bool active() const { return _active; }
      inline void setActive(bool v) { _active = v; }

      inline bool hasBypass() const { return true; }
      bool on() const;
      void setOn(bool val);

      void setTrack(AudioTrack* t);
      AudioTrack* track() const;
      unsigned long pluginID() const;
      void setID(int i);
      int id() const;
      void updateControllers();

      // This version takes a given Plugin to initialize this PluginI. The Plugin can be null.
      // A desired plugin name can be supplied. Otherwise it picks one.
      bool initPluginInstance(Plugin*, int channels, const QString& name = QString());
// REMOVE Tim. tmp. Added.
      // This version uses the built-in initial configuration member to both find and initialize this PluginI.
      // The initial configuration member must already be filled.
      // A desired plugin name can be supplied. Otherwise it picks one.
      bool initPluginInstance(int channels, const QString& name = QString());
      void setChannels(int);
      void connect(unsigned long ports, bool connectAllToDummyPorts, unsigned long offset, float** src, float** dst);
      void apply(unsigned pos, unsigned long n,
                 unsigned long ports, bool wantActive, float** bufIn, float** bufOut, float latency_corr_offset = 0.0f);

      void enableController(unsigned long i, bool v = true);
      bool controllerEnabled(unsigned long i) const;
      void enableAllControllers(bool v = true);

      void cleanup();

      QString pluginLabel() const;
      QString name() const;
      QString pluginName() const;
      QString lib() const;
      QString uri() const;
      QString dirPath() const;
      QString fileName() const;
      QString titlePrefix() const;

      #ifdef OSC_SUPPORT
      OscEffectIF& oscIF();
      int oscControl(unsigned long dssiPort, float val);
      int oscConfigure(const char *key, const char *val);
      int oscUpdate();
      #endif

// REMOVE Tim. tmp. Added.
      // Returns the plugin's current initial configuration.
      PluginConfiguration &initialConfiguration();
      // Returns the plugin's current initial configuration. Constant version.
      const PluginConfiguration &initialConfiguration() const;
      void setInitialConfiguration(const PluginConfiguration&);
      // This version uses the supplied configuration.
      // NOTE: The PluginI's track must already have been added to the track lists,
      //        because for DSSI, OSC needs to find the plugin in the track lists.
      // NOTE: If ConfigCustomData and ConfigParams options are given, and there is
      //        plugin custom data, ConfigParams will be ignored since that information
      //        is supposed to be inside the custom data.
      void configure(const PluginConfiguration&, ConfigureOptions_t);
      // This version uses the built-in initial configuration member, which must already be filled.
      // NOTE: The PluginI's track must already have been added to the track lists,
      //        because for DSSI, OSC needs to find the plugin in the track lists.
      // NOTE: If ConfigCustomData and ConfigParams options are given, and there is
      //        plugin custom data, ConfigParams will be ignored since that information
      //        is supposed to be inside the custom data.
      void configure(ConfigureOptions_t);
      // Returns a plugin configuration structure filled with the current state of the plugin.
      PluginConfiguration getConfiguration() const;
      // If isCopy is true, writes additional info including automation controllers and midi assignments.
      void writeConfiguration(int level, Xml& xml, bool isCopy = false);
// REMOVE Tim. tmp. Changed.
//       bool readConfiguration(Xml& xml, bool readPreset=false);
      bool readConfiguration(Xml& xml, bool readPreset=false, int channels=0);
// REMOVE Tim. tmp. Removed. Moved into PluginConfiguration.
//       bool loadControl(Xml& xml);
      bool setControl(const QString& s, double val);
      void showGui();
      void showGui(bool);
// REMOVE Tim. tmp. Changed.
//       bool isDssiPlugin() const;
//       bool isLV2Plugin() const;
//       bool isVstNativePlugin() const;
      MusEPlugin::PluginType pluginType() const;
      MusEPlugin::PluginClass_t pluginClass() const;
      void showNativeGui();
      void showNativeGui(bool);
      bool nativeGuiVisible() const;
// REMOVE Tim. tmp. Added.
      void closeNativeGui();
// REMOVE Tim. tmp. Added.
      void updateNativeGuiWindowTitle();
      void guiHeartBeat();

      unsigned long parameters() const;
      unsigned long parametersOut() const;
      void setParam(unsigned long i, double val);
      void putParam(unsigned long i, double val);
      double param(unsigned long i) const;
      double paramOut(unsigned long i) const;
      double defaultValue(unsigned long param) const;
      double defaultOutValue(unsigned long param) const;
      const char* paramName(unsigned long i) const;
      const char* paramOutName(unsigned long i) const;
      LADSPA_PortDescriptor portd(unsigned long i) const;
      LADSPA_PortDescriptor portdOut(unsigned long i) const;
      void range(unsigned long i, float* min, float* max) const;
      void rangeOut(unsigned long i, float* min, float* max) const;
      bool isAudioIn(unsigned long k) const;
      bool isAudioOut(unsigned long k) const;
      LADSPA_PortRangeHint range(unsigned long i) const;
      LADSPA_PortRangeHint rangeOut(unsigned long i) const;
      bool usesTransportSource() const;
      unsigned long latencyOutPortIndex() const;
      float latency() const;
      unsigned long freewheelPortIndex() const;
      unsigned long enableOrBypassPortIndex() const;
      MusEPlugin::PluginLatencyReportingType pluginLatencyReportingType() const;
      MusEPlugin::PluginBypassType pluginBypassType() const;
      MusEPlugin::PluginFreewheelType pluginFreewheelType() const;

      CtrlValueType ctrlValueType(unsigned long i) const;
      const CtrlVal::CtrlEnumValues* ctrlEnumValues( unsigned long i) const;
      CtrlList::Mode ctrlMode(unsigned long i) const;
      // Returns true if, among other data, there was indeed custom data.
      // This means there is, or likely is, parameter values stored with the data,
      //  meaning we should not try to manually restore parameters since the data
      //  already has them.
      bool setCustomData(const std::vector<QString> &customParams);
      CtrlValueType ctrlOutValueType(unsigned long i) const;
      const CtrlVal::CtrlEnumValues* ctrlOutEnumValues( unsigned long i) const;
      CtrlList::Mode ctrlOutMode(unsigned long i) const;
      // Returns a value unit string for displaying unit symbols.
      QString unitSymbol(long unsigned int i) const;
      QString unitSymbolOut(long unsigned int i) const;
      // Returns index into the global value units for displaying unit symbols.
      // Can be -1 meaning no units.
      int valueUnit ( unsigned long i) const;
      int valueUnitOut ( unsigned long i) const;

// REMOVE Tim. tmp. Added.
      // Sets up a list of existing controllers based on info gathered from either the plugin
      //  or the plugin's persistent info if the plugin is missing.
      // Sets various controller properties such as names, ranges, value types, and value units.
      bool setupControllers(CtrlListList *cll) const;
      };

//---------------------------------------------------------
//   Pipeline
//    chain of connected efx inserts
//---------------------------------------------------------

class Pipeline : public std::vector<PluginI*> {
   private:
      float* buffer[MusECore::MAX_CHANNELS];
      void initBuffers();
   public:
      Pipeline();
      Pipeline(const Pipeline&, AudioTrack*);
      ~Pipeline();
      void insert(PluginI* p, int index);
      void remove(int index);
      void removeAll();
      bool isActive(int idx) const;
      void setActive(int, bool);
      bool isOn(int idx) const;
      void setOn(int, bool);
      QString label(int idx) const;
      QString name(int idx) const;
      QString uri(int idx) const;
      void showGui(int, bool);
// REMOVE Tim. tmp. Changed.
//       bool isDssiPlugin(int) const;
//       bool isLV2Plugin(int idx) const;
//       bool isVstNativePlugin(int idx) const;
      MusEPlugin::PluginType pluginType(int idx) const;
      MusEPlugin::PluginClass_t pluginClass(int idx) const;
      bool hasNativeGui(int idx) const;
      void showNativeGui(int, bool);
      void deleteGui(int idx);
      void deleteAllGuis();
      bool guiVisible(int);
      bool nativeGuiVisible(int);
      void guiHeartBeat();

      void apply(unsigned pos, unsigned long ports, unsigned long nframes, bool wantActive, float** buffer);

      void move(int idx1, int idx2);
      bool empty(int idx) const;
      void setChannels(int);
      bool addScheduledControlEvent(int track_ctrl_id, double val, unsigned frame); // returns true if event cannot be delivered
      void enableController(int track_ctrl_id, bool en);
      bool controllerEnabled(int track_ctrl_id);
      float latency() const;
// REMOVE Tim. tmp. Added.
      // Returns the first plugin instance with the given name found in the rack.
      PluginI* findPlugin(const QString &);
      // Returns the first plugin instance with the given name found in the rack. Const version.
      const PluginI* findPlugin(const QString &) const;
      };

typedef Pipeline::iterator iPluginI;
typedef Pipeline::const_iterator ciPluginI;

extern void initPlugins();

extern bool ladspaDefaultValue(const LADSPA_Descriptor* plugin, unsigned long port, float* val);
extern void ladspaControlRange(const LADSPA_Descriptor* plugin, unsigned long port, float* min, float* max);
extern bool ladspa2MidiControlValues(const LADSPA_Descriptor* plugin, unsigned long port, int ctlnum, int* min, int* max, int* def);
extern float midi2LadspaValue(const LADSPA_Descriptor* plugin, unsigned long port, int ctlnum, int val);
extern CtrlValueType ladspaCtrlValueType(const LADSPA_Descriptor* plugin, int port);
extern CtrlList::Mode ladspaCtrlMode(const LADSPA_Descriptor* plugin, int port);

} // namespace MusECore


namespace MusEGui {
class DoubleLabel;

//   PluginLoader
//---------------------------------------------------------

class PluginLoader : public QUiLoader
{
   public:
      virtual QWidget* createWidget(const QString & className, QWidget * parent = 0, const QString & name = QString());
      PluginLoader(QObject * parent = 0);
};

//---------------------------------------------------------
//   GuiParam
//---------------------------------------------------------

struct GuiParam {
      enum {
            GUI_SLIDER, GUI_CHECKBOX, GUI_SWITCH, GUI_METER, GUI_ENUM
            };
      int type;
      int hint;
      bool pressed;

      MusEGui::DoubleLabel* label;
      MusEGui::DoubleText* textLabel;
      QWidget* actuator;  // Slider or Toggle Button (SWITCH)
      };

//---------------------------------------------------------
//   GuiWidgets
//---------------------------------------------------------

struct GuiWidgets {
      enum {
            SLIDER, DOUBLE_LABEL,
            CHECKBOX,
            SWITCH,
            QCOMBOBOX
            };
      QWidget* widget;
      int type;
      int hint;
      unsigned long param;
      bool pressed;
      };

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

class PluginGui : public QMainWindow {
      Q_OBJECT

      MusECore::PluginIBase* plugin;        // plugin instance

      GuiParam* params;
      GuiParam* paramsOut;
      unsigned long nobj;             // number of widgets in gw
      GuiWidgets* gw;

      QAction* activeButton;
      QAction* onOff;
      QWidget* mw;            // main widget
      QScrollArea* view;

      QMetaObject::Connection _configChangedMetaConn;
// REMOVE Tim. tmp. Added.
      // QMetaObject::Connection _songChangedMetaConn;

      void updateControls();
      void getPluginConvertedValues(LADSPA_PortRangeHint range,
                     double &lower, double &upper, double &dlower, double &dupper, double &dval);
      void constructGUIFromFile(QFile& uifile);
      void constructGUIFromPluginMetadata();

   protected:
      virtual void showEvent(QShowEvent *e);
      virtual void hideEvent(QHideEvent *e);
    
   private slots:
      void load();
      void save();
      void activeToggled(bool);
      void bypassToggled(bool);
      void showSettings();
      void sliderChanged(double value, int id, int scrollMode);
      void switchChanged(bool value, int id);
      void labelChanged(double, int);
      void comboChanged(unsigned long);
      void guiParamChanged(unsigned long int);
      void sliderPressed(double, int);
      void sliderReleased(double, int);
      void switchPressed(int);
      void switchReleased(int);
      void guiParamPressed(unsigned long int);
      void guiParamReleased(unsigned long int);
      void guiSliderPressed(double, unsigned long int);
      void guiSliderReleased(double, unsigned long int);
      void ctrlRightClicked(const QPoint &, int);
      void guiSliderRightClicked(const QPoint &, unsigned long int);
      void guiContextMenuReq(unsigned long int idx);

   public slots:
      void heartBeat();
      void configChanged();
// REMOVE Tim. tmp. Added.
      // void songChanged(MusECore::SongChangedStruct_t);

   public:
      PluginGui(MusECore::PluginIBase*);
      ~PluginGui();

      void setActive(bool);
      void setOn(bool);
      void updateValues();
// REMOVE Tim. tmp. Added.
      void updateWindowTitle(const QString&);
      };


} // namespace MusEGui

namespace MusEGlobal {
extern MusECore::PluginList plugins;
extern MusECore::PluginGroups plugin_groups;
extern QList<QString> plugin_group_names;
extern MusECore::MissingPluginList missingPlugins;

void writePluginGroupConfiguration(int level, MusECore::Xml& xml);
void readPluginGroupConfiguration(MusECore::Xml& xml);
}
#endif

