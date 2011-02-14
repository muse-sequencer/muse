//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: ctrl.h,v 1.4.2.2 2006/10/29 07:54:51 terminator356 Exp $
//
//    controller for mixer automation
//
//  (C) Copyright 2003-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CTRL_H__
#define __CTRL_H__

#include <map>
#include <list>
#include <qcolor.h>

const int AC_VOLUME = 0;
const int AC_PAN    = 1;
const int AC_MUTE   = 2;

#define AC_PLUGIN_CTL_BASE         0x1000
#define AC_PLUGIN_CTL_BASE_POW     12
#define AC_PLUGIN_CTL_ID_MASK      0xFFF

inline int genACnum(int plugin, int ctrl) { return (plugin + 1) * AC_PLUGIN_CTL_BASE + ctrl; }

class Xml;

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
//   CtrlList
//    arrange controller events of a specific type in a
//    list for easy retrieval
//---------------------------------------------------------

typedef std::map<int, CtrlVal, std::less<int> >::iterator iCtrl;
typedef std::map<int, CtrlVal, std::less<int> >::const_iterator ciCtrl;

class CtrlList : public std::map<int, CtrlVal, std::less<int> > {
   public:
      enum Mode { INTERPOLATE, DISCRETE};
      
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

      Mode mode() const          { return _mode; }
      void setMode(Mode m)       { _mode = m; }
      double getDefault() const   { return _default; }
      void setDefault(double val) { _default = val; }
      double curVal() const       { return _curVal; }
      void setCurVal(double val); //  { _curVal = val; }
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

      double value(int frame);
      void add(int tick, double value);
      void del(int tick);
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
   public:
      void add(CtrlList* vl);
      iCtrlList find(int id) {
            return std::map<int, CtrlList*, std::less<int> >::find(id);
            }
      ciCtrlList find(int id) const {
            return std::map<int, CtrlList*, std::less<int> >::find(id);
            }
      };

#endif

