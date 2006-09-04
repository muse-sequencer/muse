//=================================================================
//  trackpattern.h
//  TrackPattern class for miditracker, QDock, QTree, Part, matrix
//    miditracker.h
//  (C) Copyright 2006 Nil Geisweiller (a_lin@user.sourceforge.net)
//=================================================================

#ifndef __TRACKERPATTERN_H__
#define __TRACKERPATTERN_H__

#include <vector>
#include "miditrack.h"

//----------------------------------------------------------
// EventPat
//  has to be derived, can be note or control
//----------------------------------------------------------
class EventPat {
 private:
  bool _isReadable; //true iff the time of the event is the exact row time
 public:
  EventPat(bool);
  EventPat(); //_isReadable is true at the initialization
  ~EventPat();

  void setReadable(bool);
  bool getReadable();
};

//----------------------------------------------------------
// VoicePat
//----------------------------------------------------------
class VoicePat : public EventPat {
  private:
  int _noteNum; //absolute note number including octave
  int _velocity; //if velocity is 0 note is off
 public:
  VoicePat(int noteNum, int velocity); // _isReadable is initialized true
  VoicePat(); // _isReadable is initialized false
  ~VoicePat();

  void setNoteNum(int n);
  int getNoteNum();
  void setVelocity(int n);
  int getVelocity();
  QString str(); //return the string to display on the entry of the pattern
};

//----------------------------------------------------------
// CtrlPat
//----------------------------------------------------------
class CtrlPat : public EventPat {
 private:
  int _ctrlNum;
  int _value; //if velocity is 0 note is off
 public:
  CtrlPat(int ctrlNum, int value); // _isReadable is initialized true
  CtrlPat(); // _isReadable is initialized false
  ~CtrlPat();

  void setCtrlNum(int n);
  int getCtrlNum();
  void setValue(int n);
  int getValue();
  QString str(); //return the string to display on the entry of the pattern
};

//----------------------------------------------------------
// TrackRowPat
//----------------------------------------------------------
class TrackRowPat {
 private:
   std::vector<EventPat> _events; //one event for each voice/ctrl of a track
 public:
  TrackRowPat();
  ~TrackRowPat();

  //TODO methods
};


//------------------------------------------------------
// TrackPattern
//------------------------------------------------------
class TrackPattern {
 private:
  QDockWidget* _dock;
  QTreeWidget* _tree;
  PartList* _partList; //partList concerned by a track
  MidiTrack* _track;
  TrackRowPat* _trackRow;
 public:
  TrackPattern();
  ~TrackPattern();

  //TODO methods
};

#endif
