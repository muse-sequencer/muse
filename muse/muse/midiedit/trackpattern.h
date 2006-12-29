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
#include "part.h"

class PartList;

//----------------------------------------------------------
// EventPat
//  has to be derived, can be note or control
//----------------------------------------------------------
class EventPat {
 private:
 protected:
  bool _isReadable; //true iff the time of the event is the exact row time
  bool _isEmpty;
 public:
  EventPat(bool isEmpty, bool isReadable);
  EventPat(); //_isEmpty=true and_isReadable=true at the initialization
  ~EventPat();

  void setEmpty(bool);
  bool getEmpty();
  void setReadable(bool);
  bool getReadable();
};

//----------------------------------------------------------
// VoiceEventPat
//----------------------------------------------------------
class VoiceEventPat : public EventPat {
  private:
  int _noteNum; //absolute note number including octave
  int _velocity; //if velocity is 0 note is off
 public:
  VoiceEventPat(int noteNum, int velocity); // _isReadable is initialized true
  VoiceEventPat(bool isEmpty, bool isReadable);
  VoiceEventPat(); //_isEmpty = true, _isReadable = true
  ~VoiceEventPat();

  void setNoteNum(int n);
  int getNoteNum();
  void setVelocity(int n);
  int getVelocity();
  QString str(); //return the string to display on the entry of the pattern
};

//----------------------------------------------------------
// CtrlEventPat
//----------------------------------------------------------
class CtrlEventPat : public EventPat {
 private:
  int _ctrlNum;
  int _value; //if velocity is 0 note is off
 public:
  CtrlEventPat(int ctrlNum, int value); // _isReadable is initialized true
  CtrlEventPat(); // _isReadable is initialized false
  ~CtrlEventPat();

  void setCtrlNum(int n);
  int getCtrlNum();
  void setValue(int n);
  int getValue();
  QString str(); //return the string to display on the entry of the pattern
};

//----------------------------------------------------------
// BasePat
//----------------------------------------------------------
class BasePat {
 protected:
  QString _name;
  unsigned _firstTick;
  unsigned _lastTick;
  int _quant;
 public:
  BasePat();
  BasePat(QString name, unsigned firstTick, unsigned lastTick, int quant);
  ~BasePat();

  QString getName();

  bool isRow(unsigned tick); //return true iff tick coincides with one row
  unsigned tick2row(unsigned tick);  
};

//----------------------------------------------------------
// VoicePat
//----------------------------------------------------------
class VoicePat : public BasePat {
 private:
  std::vector<VoiceEventPat*> _eventsCol; //column of VoiceEventPat to display
  EventList* _events; //actual list of events, only one at a time
 public:
  VoicePat(QString name, unsigned firstTick, unsigned lastTick, int quant);
  ~VoicePat();
  
  std::vector<VoiceEventPat*> getEventsCol();

  bool add(const Event* e, unsigned tick); //add the Event e into the EventList
                                           //and update properly _events
                                           //return true if success, that is
                                           //there is an empty space of the
                                           //event
  bool isFreeSpace(const Event* e, unsigned tick); //return true iff there
                                                   //is space to add the
                                                   //event e without
                                                   //overlapping other events
};

//----------------------------------------------------------
// CtrlPat
//----------------------------------------------------------
class CtrlPat {
 private:
  std::vector<CtrlEventPat> _events; //column of CtrlEventPat
 public:
  CtrlPat(QString name);
  ~CtrlPat();
};

//------------------------------------------------------
// BaseTrackPat
//------------------------------------------------------
class BaseTrackPat : public QDockWidget {
 protected:
  QTreeWidget* _tree;
  QMainWindow* _parent;
  unsigned _currentRow;
  unsigned _numRow; //contains the number of rows
 public:
  BaseTrackPat(QMainWindow* parent);
  ~BaseTrackPat();

  void setNumRow(unsigned);
  unsigned getNumRow();
  unsigned getRowMag(); //returns the number of rows to display according
                        //to the size of the window
  unsigned getCurTreeRow(); 
  unsigned getLowRow();
  unsigned getUpRow(); 
};

//------------------------------------------------------
// TrackPattern
//------------------------------------------------------
class TrackPattern : public BaseTrackPat, public BasePat {
 private:
  PartList* _partList; //partList concerned by a track
  MidiTrack* _track;
  std::vector<VoicePat*> _voiceColumns; //matrix of voice events
  std::vector<CtrlPat*> _ctrlColumns; //matrix of ctrl events
 public:
  TrackPattern(QMainWindow* parent, QString name, 
	       unsigned firstTick, unsigned lastTick,
	       int quant, PartList* pl, MidiTrack* t);
  ~TrackPattern();

  void add(const Event* e, unsigned tick); //add the Event e and
                                           //build consequently
                                           //the matrix,
                                           //creating new voices when necessary
  MidiTrack* getTrack() {return _track;}
  void setQuant(int quant);
  void buildEventMatrix();
  void fillTrackPat(); //fill the treeWidget with the right set of events
                       //according to _currentRow and the size of the window

 protected:
  void resizeEvent(QResizeEvent *event);
};

//------------------------------------------------------
// TimingEvent
//------------------------------------------------------
class TimingEvent {
 private:
  int _bar;
  int _beat;
  unsigned _tick;
  unsigned _row;

 public:
  TimingEvent(unsigned row);
  ~TimingEvent();

  void setBarBeatTick(unsigned tick);

  QString barBeatTickStr();
  QString rowStr();
};

class TimingPattern : public BasePat, public BaseTrackPat {
 private:
  QTreeWidget* _tree;
  std::vector<TimingEvent*> _timingEvents;
 public:
  TimingPattern(QMainWindow* parent, QString name, unsigned firstTick,
		unsigned lastTick, int quant);
  ~TimingPattern();

  void buildTimingMatrix();
  void fillTimingPat(); //fill the treeWidget with the right window of times
                         //according to _currentRow and the size of the window

 protected:
  void resizeEvent(QResizeEvent *event);
};

#endif
