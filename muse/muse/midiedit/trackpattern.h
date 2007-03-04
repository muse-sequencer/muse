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
  Q_OBJECT

 protected:
  QTreeWidget* _tree;
  QMainWindow* _parent;

  unsigned _rowMag; //contains the number of rows
  unsigned _firstRow; //absolute index of the first row
  unsigned _lastRow; //absolute index of the last row, included
  unsigned _relativeCurrentRow; //index of the current according to the tree
  unsigned _absoluteCurrentRow; //index of the current row according to the
                                //event matrix
  unsigned _absoluteNbrRow; //contains the number of rows of the matrix

  int _fontHeight;

  bool _update; //if true then the tree must updated

 public:
  BaseTrackPat(QMainWindow* parent, unsigned anr = 0);
  ~BaseTrackPat();

  void setRowMag(); //set _rowMag with the number of rows to display according
                    //to the size of the window, adjust _lastRow accordingly,
                    //assum that first row is set appropriately
  void setFirstRow(unsigned f); //set _firstRow with f, that is the absolute index
                                //of the first row, adjust _lastRow appropriately
  void setRelativeCurrentRow(unsigned r); //set _relativeCurrentRow with r
                                          //and _absoluteCurrentRow accordingly
  void setAbsoluteCurrentRow(unsigned a); //set _absoluteCurrentRow with a
                                          //and _relativeCurrentRow accordingly

  unsigned getRowMag();
  unsigned getFirstRow();
  unsigned getLastRow();
  unsigned getRelativeCurrentRow();
  unsigned getAbsoluteCurrentRow();

  unsigned getAbsoluteNbrRow();

  void moveRelativeCurrentRow(unsigned newIndex); //update _firstRow, _lastrow
                                                  //relativeCurrentRow,
                                                  //absoluteCurrentRow, considering
                                                  //that the new relative index is
                                                  //newIndex

  void resizeEvent(QResizeEvent* /*event*/);

  virtual void fillPattern() {} //fill the treeWidget with the right window of times
                                //according to _firstRow and _lastRow


  void selectCurrentRow(); //block the signals and select the current row

 signals:
  void moveCurrentRow(unsigned i); //send the signal that the current row is moved
                                   //at the relative index i
 private slots:
  void currentItemChanged(QTreeWidgetItem* nitem);
  void moveRowFromSignal(unsigned index);

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
	       int quant, PartList* pl, MidiTrack* t, unsigned anr = 0);
  ~TrackPattern();

  void add(const Event* e, unsigned tick); //add the Event e and
                                           //build consequently
                                           //the matrix,
                                           //creating new voices when necessary
  MidiTrack* getTrack() {return _track;}
  void setQuant(int quant);
  void buildEventMatrix();

  virtual void fillPattern();
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
  //QTreeWidget* _tree;
  std::vector<TimingEvent*> _timingEvents;
 public:
  TimingPattern(QMainWindow* parent, QString name, unsigned firstTick,
		unsigned lastTick, int quant);
  ~TimingPattern();

  void buildTimingMatrix();

  virtual void fillPattern();
};

#endif
