//=================================================================
//  miditracker
//  midi editor a la soundtracker
//    miditracker.h
//  (C) Copyright 2006 Nil Geisweiller (a_lin@user.sourceforge.net)
//=================================================================

#ifndef __MTEDITOR_H__
#define __MTEDITOR_H__

#include "midieditor.h"
#include "trackpattern.h"
#include "quantcombo.h"
#include <vector>


class QuantCombo;

namespace AL {
      class Xml;
      };
using AL::Xml;

//---------------------------------------------------------
//   MidiTrackerEditor
//---------------------------------------------------------

class MidiTrackerEditor : public MidiEditor {
  Q_OBJECT
 private:
  int _quant;
  QuantCombo* _quantCombo;
  int _numVisibleRows;
  QSpinBox* _nvrSpinBox;

  QMenu* menuView;
  bool _follow;
  void setFollow(bool);

  std::vector<TrackPattern*> _trackPatterns;

 private slots:
  virtual void cmd(QAction*);
 public:
  MidiTrackerEditor(PartList*, bool);
  ~MidiTrackerEditor() {}

  void setQuant(int rpm);
  int getQuant();
  void updateQuant(); //update the gui

  void setNumVisibleRows(int nvr);
  int getNumVisibleRows();
  void updateNumVisibleRows(); //update the gui

  static const bool INIT_FOLLOW  = false;
  static const bool INIT_SPEAKER = true;
  static const bool INIT_SREC    = false;
  static const bool INIT_MIDIIN  = false;

};

#endif
