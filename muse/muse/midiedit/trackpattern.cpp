#include "trackpattern.h"

//----------------------------------------------------------
// EventPat
//  has to be derived, can be note or control
//----------------------------------------------------------
EventPat::EventPat(bool r) {_isReadable = r;}
EventPat::EventPat() {EventPat(true);}
EventPat::~EventPat() {}

void EventPat::setReadable(bool r) {_isReadable = r;}
bool EventPat::getReadable() {return _isReadable;}

//----------------------------------------------------------
// VoicePat
//----------------------------------------------------------
VoicePat::VoicePat(int n, int v):EventPat(true) {
  _noteNum = n;
  _velocity = v;
}
VoicePat::VoicePat():EventPat(false) {}
VoicePat::~VoicePat() {}

void VoicePat::setNoteNum(int n) { _noteNum = n; }
int VoicePat::getNoteNum() { return _noteNum; }
void VoicePat::setVelocity(int n) { _velocity = n; }
int VoicePat::getVelocity() { return _velocity; }
QString VoicePat::str() {
  //TODO
  QString sNote;
  sNote.setNum(_noteNum);
  QString s = " ";
  QString sVel;
  sVel.setNum(_velocity);
  return sNote + s + sVel;
}

//----------------------------------------------------------
// CtrlPat
//----------------------------------------------------------
CtrlPat::CtrlPat(int c, int v):EventPat(true) {
  _ctrlNum = c;
  _value = v;
}
CtrlPat::CtrlPat():EventPat(false) {}
CtrlPat::~CtrlPat() {}

void CtrlPat::setCtrlNum(int n) { _ctrlNum = n; }
int CtrlPat::getCtrlNum() { return _ctrlNum; }
void CtrlPat::setValue(int n) { _value = n; }
int CtrlPat::getValue() { return _value; }
QString CtrlPat::str() {
  //TODO
  QString sCtrl;
  sCtrl.setNum(_ctrlNum);
  QString s = " ";
  QString sVal;
  sVal.setNum(_value);
  return sCtrl + s + sVal;
}


//------------------------------------------------------
// TrackPattern
//------------------------------------------------------
TrackPattern::TrackPattern(){}
TrackPattern::~TrackPattern(){}

