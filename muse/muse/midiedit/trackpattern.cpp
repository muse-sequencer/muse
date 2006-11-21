#include "trackpattern.h"
#include "event.h"
#include "al/sig.h"

class EventList;

#define EMPTYCHAR "-"
#define NONREADCHAR "*"
#define SPACECHAR "-"
#define PLUSCHAR "+"
#define STOPCHAR "="
#define SEPCHAR " "
//#define FONT "Console"
//#define FONT "Monospace"
#define FONT "MiscFixed"

//----------------------------------------------------------
// EventPat
//  has to be derived, can be note or control
//----------------------------------------------------------
EventPat::EventPat(bool e, bool r) {_isEmpty = e; _isReadable = r;}
EventPat::EventPat() {EventPat(true, true);}
EventPat::~EventPat() {}

void EventPat::setEmpty(bool e) {_isEmpty = e;}
bool EventPat::getEmpty() {return _isEmpty;}
void EventPat::setReadable(bool r) {_isReadable = r;}
bool EventPat::getReadable() {return _isReadable;}

//----------------------------------------------------------
// VoiceEventPat
//----------------------------------------------------------
VoiceEventPat::VoiceEventPat(int n, int v):EventPat(false, true) {
  _noteNum = n;
  _velocity = v;
}
VoiceEventPat::VoiceEventPat(bool e, bool r):EventPat(e, r) {}
VoiceEventPat::VoiceEventPat():EventPat(true, true) {}
VoiceEventPat::~VoiceEventPat() {}

void VoiceEventPat::setNoteNum(int n) { _noteNum = n; }
int VoiceEventPat::getNoteNum() { return _noteNum; }
void VoiceEventPat::setVelocity(int n) { _velocity = n; }
int VoiceEventPat::getVelocity() { return _velocity; }
QString VoiceEventPat::str() {
  if(_isEmpty) {
    return QString(EMPTYCHAR EMPTYCHAR EMPTYCHAR EMPTYCHAR)
      + QString(SEPCHAR) + QString(EMPTYCHAR EMPTYCHAR EMPTYCHAR);
  }
  else if(_isReadable) {
    if(_velocity==0) {
      return QString(STOPCHAR STOPCHAR STOPCHAR STOPCHAR)
	+ QString(SEPCHAR) + QString(EMPTYCHAR EMPTYCHAR EMPTYCHAR);
    }
    else {
      int octave = _noteNum/12 - 2;
      int note = _noteNum%12;
      QString sNote;
      switch(note) {
      case 0: sNote = QString("C") + QString(SPACECHAR);
	break;
      case 1: sNote = QString("C#");
	break;
      case 2: sNote = QString("D") + QString(SPACECHAR);
	break;
      case 3: sNote = QString("D#");
	break;
      case 4: sNote = QString("E") + QString(SPACECHAR);
	break;
      case 5: sNote = QString("F") + QString(SPACECHAR);
	break;
      case 6: sNote = QString("F#");
	break;
      case 7: sNote = QString("G") + QString(SPACECHAR);
	break;
      case 8: sNote = QString("G#");
	break;
      case 9: sNote = QString("A") + QString(SPACECHAR);
	break;
      case 10: sNote = QString("A#");
	break;
      case 11: sNote = QString("F") + QString(SPACECHAR);
	break;
      default:
	printf("VoiceEventPat::str() Error : case note not treated\n");
	break;
      }
      QString sOctave;
      sOctave.setNum(octave);
      if(octave>=0) {
	sOctave = QString(PLUSCHAR) + sOctave;
      }
      QString sVel;
      sVel.setNum(_velocity);
      if(_velocity<10) {
	sVel = QString("00") + sVel;
      }
      else if(_velocity<100) {
	sVel = QString("0") + sVel;
      }
      return sNote + sOctave + QString(SEPCHAR) + sVel;
    }
  }
  else {
    return QString(NONREADCHAR NONREADCHAR NONREADCHAR NONREADCHAR)
      + QString(SEPCHAR) + QString(NONREADCHAR NONREADCHAR NONREADCHAR);
  }
}

//----------------------------------------------------------
// CtrlEventPat
//----------------------------------------------------------
CtrlEventPat::CtrlEventPat(int c, int v):EventPat(false, true) {
  _ctrlNum = c;
  _value = v;
}
CtrlEventPat::CtrlEventPat():EventPat(true, true) {}
CtrlEventPat::~CtrlEventPat() {}

void CtrlEventPat::setCtrlNum(int n) { _ctrlNum = n; }
int CtrlEventPat::getCtrlNum() { return _ctrlNum; }
void CtrlEventPat::setValue(int n) { _value = n; }
int CtrlEventPat::getValue() { return _value; }
QString CtrlEventPat::str() {
  //TODO
  if(_isReadable) {
    QString sCtrl;
    sCtrl.setNum(_ctrlNum);
    QString s = " ";
    QString sVal;
    sVal.setNum(_value);
    return sCtrl + s + sVal;
  }
  else {
    QString s = "***";
    return s;
  }
}

//----------------------------------------------------------
// BasePat
//----------------------------------------------------------
BasePat::BasePat(QString name, unsigned firstTick,
		 unsigned lastTick, int quant) {
  _name = name;
  _firstTick = firstTick;
  _lastTick = lastTick;
  _quant = quant;
}

BasePat::~BasePat() {}

QString BasePat::getName() {
  return _name;
}

bool BasePat::isRow(unsigned tick) {
  Pos p = AL::sigmap.raster(_firstTick + tick, _quant);
  return p.tick() == _firstTick + tick;
}

unsigned BasePat::tick2row(unsigned tick) {
  return (_firstTick + tick) / _quant - (_firstTick / _quant);
}

//----------------------------------------------------------
// VoicePat
//----------------------------------------------------------
VoicePat::VoicePat(QString name, unsigned firstTick, unsigned lastTick,
		   int quant):BasePat(name, firstTick, lastTick, quant) {
  _events = new EventList();
}
VoicePat::~VoicePat() {
  delete(_events);
}

std::vector<VoiceEventPat*> VoicePat::getEventsCol() {
  return _eventsCol;
}

bool VoicePat::add(const Event* ev, unsigned tick) {
  Event* e = new Event(*ev);
  if(isFreeSpace(e, tick)) {
    //add into the list of events
    _events->add(*e, tick);
    //add the begin tick into the column
    unsigned beginRow = tick2row(tick);
    if(beginRow >= _eventsCol.size()) {
      for(unsigned i = _eventsCol.size(); i <= beginRow; i++) {
	//empty voice event
	_eventsCol.push_back(new VoiceEventPat());
      }
    }
    VoiceEventPat* vbep;
    if(isRow(tick)) vbep = new VoiceEventPat(e->pitch(), e->velo());
    else vbep = new VoiceEventPat(false, false); //non-readable
    if(_eventsCol[beginRow]) delete(_eventsCol[beginRow]);
    _eventsCol[beginRow] = vbep;

    //add the end tick into the column
    unsigned endTick = e->lenTick() + tick;
    unsigned endRow = tick2row(endTick);
    if(endRow >= _eventsCol.size()) {
      for(unsigned i = _eventsCol.size(); i <= endRow; i++) {
	//empty voice event
	_eventsCol.push_back(new VoiceEventPat());
      }
    }
    VoiceEventPat* veep;
    if(isRow(endTick)) veep = new VoiceEventPat(e->pitch(), 0);
    else veep = new VoiceEventPat(false, false); //non-readable
    if(_eventsCol[endRow]
       &&
       (_eventsCol[endRow]->getEmpty() || tick2row(endTick)==tick2row(tick))) {
      delete(_eventsCol[endRow]);
      _eventsCol[endRow] = veep;
    }
    return true;
  }
  else {
    return false;
  }
}

bool VoicePat::isFreeSpace(const Event* e, unsigned tick) {
  bool isFree = true;
  for(ciEvent ce = _events->begin(); ce != _events->end(); ce++) {
    const Event* cevent = &ce->second;
    unsigned beginTick = ce->first;
    unsigned endTick = ce->first + cevent->lenTick();
    isFree = (endTick <= tick) || (beginTick >= tick + e->lenTick());
    if(!isFree) break;
  }
  return isFree;
}

//----------------------------------------------------------
// CtrlPat
//----------------------------------------------------------
CtrlPat::CtrlPat(QString name) {
}
CtrlPat::~CtrlPat() {}

//------------------------------------------------------
// TrackPattern
//------------------------------------------------------
TrackPattern::TrackPattern(QMainWindow* parent, unsigned firstTick,
			   int quant, PartList* pl, MidiTrack* t) {
  //set attributs
  _track = t;
  _quant = quant;
  _firstTick = firstTick;

  //build the list of parts belonging to track t
  _partList = new PartList;
  for(ciPart p = pl->begin(); p != pl->end(); p++) {
    Part* part = p->second;
    if(t==part->track())
      _partList->add(part);
  }

  //build the matrix of events
  for(ciPart p = _partList->begin(); p != _partList->end(); p++) {
    Part* part = p->second;
    EventList* events = part->events();
    for(ciEvent e = events->begin(); e != events->end(); e++) {
      const Event* event = &e->second;
      unsigned rescaledTick = part->tick() + event->tick() - _firstTick;
      add(event, rescaledTick);
    }
  }
  
  //build the dockWidget
  _dock = new QDockWidget(_track->name());
  _dock->setFeatures(QDockWidget::DockWidgetClosable |
		     QDockWidget::DockWidgetMovable);
  parent->addDockWidget(Qt::LeftDockWidgetArea, _dock, Qt::Horizontal);

  //build the treeWidget
  _tree = new QTreeWidget(_dock);
  _tree->setColumnCount(_voiceColumns.size() + _ctrlColumns.size());
  QStringList headerLabels;
  for(unsigned i = 0; i < _voiceColumns.size(); i++) {
    headerLabels += QStringList(_voiceColumns[i]->getName());
  }
  for(unsigned i = 0; i < _ctrlColumns.size(); i++) {
    //TODO
    //headerLabels += QStringList(_ctrlColumns[i]->getName());
  }
  _tree->setHeaderLabels(headerLabels);
  //set some display properties
  _tree->setRootIsDecorated(false);
  _tree->setUniformRowHeights(true);
  QFont font =_tree->font();
  font.setFamily(FONT);
  _tree->setFont(font);
  _dock->setWidget(_tree);

  //fill the treeWidget
  for(unsigned i = 0; i < _voiceColumns.size(); i++) {
    for(unsigned j = 0; j < _voiceColumns[i]->getEventsCol().size(); j++) {
      QTreeWidgetItem* item = _tree->topLevelItem(j);
      if(!item) item = new QTreeWidgetItem(_tree);
      VoiceEventPat* vep = (_voiceColumns[i]->getEventsCol())[j];
      if(vep) item->setText(i, vep->str());
    }
  }
  for(unsigned i = 0; i < _ctrlColumns.size(); i++) {
    //TODO CTRL
  }
  
  
  for(unsigned i = 0; i < _voiceColumns.size(); i++)
    _tree->resizeColumnToContents(i);

}

TrackPattern::~TrackPattern(){}

void TrackPattern::add(const Event* e, unsigned tick) {
  if(e->isNote()) {
    bool success = false;
    for(unsigned i = 0; i < _voiceColumns.size(); i++) {
      success = _voiceColumns[i]->add(e, tick);
      if(success) break;
    }
    if(!success) {
      QString voiceName;
      voiceName.setNum(_voiceColumns.size());
      voiceName = QString("Voice " + voiceName);
      VoicePat* vp = new VoicePat(voiceName, _firstTick, 0, _quant);
      _voiceColumns.push_back(vp);
      bool success = vp->add(e, tick);
      if(!success) printf("Error TrackPattern::add\n");      
    }
  }
  else {
    //TODO Ctrl
  }
}

void TrackPattern::setQuant(int quant) {
  //TODO
}

//---------------------------------------------------------------
// TimingEvent
//---------------------------------------------------------------
TimingEvent::TimingEvent(unsigned row) {
  _row = row;
}

void TimingEvent::setBarBeatTick(unsigned tick) {
  AL::sigmap.tickValues(tick, &_bar, &_beat, &_tick);
}

QString TimingEvent::barBeatTickStr() {
  QString barS;
  barS.setNum(_bar);
  if(_bar<10) barS = QString("000") + barS;
  else if(_bar<100) barS = QString("00") + barS;
  else if(_bar<1000) barS = QString("0") + barS;
  QString beatS;
  beatS.setNum(_beat);
  if(_beat<10) beatS = QString("0") + beatS;
  QString tickS;
  tickS.setNum(_tick);
  if(_tick<10) tickS = QString("00") + tickS;
  else if(_tick<100) tickS = QString("0") + tickS;
  return barS + QString(":") + beatS + QString(":") + tickS;
}

QString TimingEvent::rowStr() {
  QString r;
  r.setNum(_row);
  if(_row<10) r = QString("00") + r;
  else if(_row<100) r = QString("0") + r;
  return r;
}

//---------------------------------------------------------------
// TimingPattern
//---------------------------------------------------------------
TimingPattern::TimingPattern(QMainWindow* parent, QString name,
			     unsigned firstTick, unsigned lastTick, int quant)
  : BasePat(name, firstTick, lastTick, quant) {
  
  //build the timing matrix
  buildTimingMatrix();
  
  //build the dockWidget
  _dock = new QDockWidget(_name);
  _dock->setFeatures(QDockWidget::DockWidgetClosable |
		     QDockWidget::DockWidgetMovable);
  parent->addDockWidget(Qt::LeftDockWidgetArea, _dock, Qt::Horizontal);
  
  //build the treeWidget
  _tree = new QTreeWidget(_dock);
  _tree->setColumnCount(2);
  QStringList headerLabels;
  _tree->setHeaderLabels(QStringList("bar:bt:tick") + QStringList("row")); 
  _tree->setHeaderLabels(headerLabels);
  //set some display properties
  _tree->setRootIsDecorated(false);
  _tree->setUniformRowHeights(true);
  QFont font =_tree->font();
  font.setFamily(FONT);
  _tree->setFont(font);
  _dock->setWidget(_tree);

  //fill the treeWidget
  for(unsigned i = 0; i < _timingEvents.size(); i++) {
    QTreeWidgetItem* item = new QTreeWidgetItem(_tree);
    TimingEvent* te = _timingEvents[i];
    item->setText(0, te->barBeatTickStr());
    item->setText(1, te->rowStr());
  }
  
  for(int i = 0; i < _tree->columnCount(); i++)
    _tree->resizeColumnToContents(i);
}

void TimingPattern::buildTimingMatrix() {
  for(unsigned tick = _firstTick; tick <= _lastTick; tick++) {
    if(isRow(tick)) {
      TimingEvent* te = new TimingEvent(tick2row(tick));
      te->setBarBeatTick(tick);
      _timingEvents.push_back(te);
    }
  }
}
