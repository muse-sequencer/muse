#include "trackpattern.h"
#include "event.h"
#include "al/sig.h"

class EventList;

#define MAX(x,y) (x>y?x:y)

#define EMPTYCHAR "-"
#define NONREADCHAR "*"
#define SPACECHAR "-"
#define PLUSCHAR "+"
#define STOPCHAR "="
#define SEPCHAR " "
//#define FONT "Console"
#define FONT "Monospace"
//#define FONT "MiscFixed"
#define FONT_HEIGHT 14
#define OFFSET_HEIGHT 3
#define OFFSET_Y 3

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
BasePat::BasePat() {
}
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
CtrlPat::CtrlPat(QString /*name*/) {
}

CtrlPat::~CtrlPat() {}

//----------------------------------------------------------
// BaseTrackPat
//----------------------------------------------------------
BaseTrackPat::BaseTrackPat(QMainWindow* parent, unsigned anr) {
  _parent = parent;
  _tree = new QTreeWidget(this);

  _absoluteNbrRow = anr;

  _update = false;

  connect(_tree, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
	  SLOT(currentItemChanged(QTreeWidgetItem*)));
  connect(_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), 
	  SLOT(currentItemChanged(QTreeWidgetItem*)));
  connect(_parent, SIGNAL(signalMoveCurrentRow(unsigned)), this,
	  SLOT(moveRowFromSignal(unsigned)));
  //connect(_tree, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
}

BaseTrackPat::~BaseTrackPat() {
}

void BaseTrackPat::setRowMag() {
  _rowMag = (unsigned) height()/_fontHeight - OFFSET_Y;
  _lastRow = _firstRow + _rowMag - 1;
}
void BaseTrackPat::setFirstRow(unsigned f) {
  _firstRow = f;
  _lastRow = f + _rowMag - 1;
}
void BaseTrackPat::setRelativeCurrentRow(unsigned r) {
  _relativeCurrentRow = r;
  _absoluteCurrentRow = r + _firstRow;
}
void BaseTrackPat::setAbsoluteCurrentRow(unsigned a) {
  _absoluteCurrentRow = a;
  _relativeCurrentRow = a - _firstRow;
}

unsigned BaseTrackPat::getRowMag() {
  return _rowMag;
}
unsigned BaseTrackPat::getFirstRow() {
  return _firstRow;
}
unsigned BaseTrackPat::getLastRow() {
  return _lastRow;
}
unsigned BaseTrackPat::getRelativeCurrentRow() {
  return _relativeCurrentRow;
}
unsigned BaseTrackPat::getAbsoluteCurrentRow() {
  return _absoluteCurrentRow;
}

unsigned BaseTrackPat::getAbsoluteNbrRow() {
  return _absoluteNbrRow;
}

void BaseTrackPat::moveRelativeCurrentRow(unsigned newIndex) {
  if(newIndex==0 && getFirstRow()>0) {
    setFirstRow(getFirstRow() - 1);
    setRelativeCurrentRow(newIndex + 1);
    _update = true;
  }
  else if(newIndex==getRowMag()-1 && getLastRow()<_absoluteNbrRow-1) {
    setFirstRow(getFirstRow() + 1);
    setRelativeCurrentRow(newIndex - 1);
    _update = true;
  }
  else setRelativeCurrentRow(newIndex);
}

//void BaseTrackPat::itemSelectionChanged() {
void BaseTrackPat::currentItemChanged(QTreeWidgetItem* nitem) {
  int index;
  if(nitem) {
    index = _tree->indexOfTopLevelItem(nitem);

    emit moveCurrentRow(index);
  }
}

void BaseTrackPat::moveRowFromSignal(unsigned index) {
  moveRelativeCurrentRow(index);
  if(_update==true) {
    fillPattern();
    _update = false;
  }
  selectCurrentRow();
}

void BaseTrackPat::resizeEvent(QResizeEvent* /*event*/) {
  setRowMag();
  fillPattern();
  selectCurrentRow();
}

void BaseTrackPat::selectCurrentRow() {
  _tree->blockSignals(true);

  QTreeWidgetItem* item = _tree->topLevelItem(getRelativeCurrentRow());
  item->setSelected(true);
  _tree->setCurrentItem(item);

  _tree->blockSignals(false);
}

//----------------------------------------------------------
// TrackPattern
//----------------------------------------------------------
TrackPattern::TrackPattern(QMainWindow* parent, QString name,
			   unsigned firstTick, unsigned lastTick,
			   int quant, PartList* pl, MidiTrack* t, unsigned anr) 
  : BaseTrackPat(parent, anr), BasePat(name, firstTick, lastTick, quant) {

  //set attributs
  _track = t;

  //build the list of parts belonging to track t
  _partList = new PartList;
  for(ciPart p = pl->begin(); p != pl->end(); p++) {
    Part* part = p->second;
    if(t==part->track())
      _partList->add(part);
  }

  //build the matrix of events
  buildEventMatrix();
  
  //configure and add the dockWidget
  setWindowTitle(_track->name());
  setFeatures(QDockWidget::DockWidgetClosable |QDockWidget::DockWidgetMovable);
  parent->addDockWidget(Qt::LeftDockWidgetArea, this, Qt::Horizontal);

  //build the treeWidget
  _tree->setColumnCount(_voiceColumns.size() + _ctrlColumns.size());
  QStringList headerLabels;
  for(unsigned i = 0; i < _voiceColumns.size(); i++) {
    headerLabels += QStringList(_voiceColumns[i]->getName());
  }
  for(unsigned i = 0; i < _ctrlColumns.size(); i++) {
    //TODO CTRL
    //headerLabels += QStringList(_ctrlColumns[i]->getName());
  }
  _tree->setHeaderLabels(headerLabels);
  //set some display properties
  _tree->setRootIsDecorated(false);
  _tree->setUniformRowHeights(true);
  _tree->setAlternatingRowColors(true);
  QFont font =_tree->font();
  font.setFamily(FONT);
  _tree->setFont(font);
  font.setPixelSize(FONT_HEIGHT);
  _fontHeight = font.pixelSize() + OFFSET_HEIGHT;
  setWidget(_tree);

  //set the range of rows to display
  setFirstRow(10); //TODO : choose accordingly to current position of muse song
  setAbsoluteCurrentRow(10); //TODO : the same
  setRowMag();

  //fill the treeWidget
  fillPattern();
  selectCurrentRow();
  
  //Resize the columns
  for(unsigned i = 0; i < _voiceColumns.size(); i++)
    _tree->resizeColumnToContents(i);

}

TrackPattern::~TrackPattern() {
}

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

void TrackPattern::setQuant(int /*quant*/) {
  //TODO
}

void TrackPattern::buildEventMatrix() {
  for(ciPart p = _partList->begin(); p != _partList->end(); p++) {
    Part* part = p->second;
    EventList* events = part->events();
    for(ciEvent e = events->begin(); e != events->end(); e++) {
      const Event* event = &e->second;
      unsigned rescaledTick = part->tick() + event->tick() - _firstTick;
      add(event, rescaledTick);
    }
  }
}

void TrackPattern::fillPattern() {
  _tree->blockSignals(true);

  _tree->clear();
  for(unsigned i = 0; i < _voiceColumns.size(); i++) {
    for(unsigned j = getFirstRow(); j <= getLastRow(); j++) {
      QTreeWidgetItem* item = _tree->topLevelItem(j - getFirstRow());
      if(!item) item = new QTreeWidgetItem(_tree);
      VoiceEventPat* vep = (_voiceColumns[i]->getEventsCol())[j];
      if(vep) item->setText(i, vep->str());
    }
  }
  for(unsigned i = 0; i < _ctrlColumns.size(); i++) {
    //TODO CTRL
  }

  _tree->blockSignals(false);
}

//---------------------------------------------------------------
// TimingEvent
//---------------------------------------------------------------
TimingEvent::TimingEvent(unsigned row) {
  _row = row;
}
TimingEvent::~TimingEvent() {
}

void TimingEvent::setBarBeatTick(unsigned tick) {
  AL::sigmap.tickValues(tick, &_bar, &_beat, &_tick);
}

QString TimingEvent::barBeatTickStr() {
  QString barS;
  barS.setNum(_bar + 1);
  if(_bar<10) barS = QString("000") + barS;
  else if(_bar<100) barS = QString("00") + barS;
  else if(_bar<1000) barS = QString("0") + barS;
  QString beatS;
  beatS.setNum(_beat + 1);
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
  : BasePat(name, firstTick, lastTick, quant), BaseTrackPat(parent) {
  //build the timing matrix
  buildTimingMatrix();
  
  //configure and add the dockWidget
  setWindowTitle(name);
  setFeatures(QDockWidget::DockWidgetClosable |QDockWidget::DockWidgetMovable);
  parent->addDockWidget(Qt::LeftDockWidgetArea, this, Qt::Horizontal);
  
  //build the treeWidget
  QStringList headerLabels;
  _tree->setHeaderLabels(QStringList("bar:bt:tick") + QStringList("row")); 
  _tree->setHeaderLabels(headerLabels);
  //set some display properties
  _tree->setRootIsDecorated(false);
  _tree->setUniformRowHeights(true);
  _tree->setAlternatingRowColors(true);
  QFont font =_tree->font();
  font.setFamily(FONT);
  _tree->setFont(font);
  font.setPixelSize(FONT_HEIGHT);
  _fontHeight = font.pixelSize() + OFFSET_HEIGHT;
  setWidget(_tree);

  //set the range of rows to display
  setFirstRow(10); //TODO : choose accordingly to current position of muse song
  setAbsoluteCurrentRow(10); //TODO : the same
  setRowMag();  

  //fill the treeWidget
  fillPattern();
  selectCurrentRow();

  //resize the columns
  for(int i = 0; i < _tree->columnCount(); i++)
    _tree->resizeColumnToContents(i);
}

TimingPattern::~TimingPattern() {
}

void TimingPattern::buildTimingMatrix() {
  for(unsigned tick = _firstTick; tick <= _lastTick; tick++) {
    if(isRow(tick)) {
      TimingEvent* te = new TimingEvent(tick2row(tick) - tick2row(_firstTick));
      te->setBarBeatTick(tick);
      _timingEvents.push_back(te);
    }
  }
  _absoluteNbrRow = _timingEvents.size();
}

void TimingPattern::fillPattern() {
  _tree->blockSignals(true);

  _tree->clear();
  for(unsigned i = getFirstRow(); i <= getLastRow(); i++) {
    QTreeWidgetItem* item = new QTreeWidgetItem(_tree);
    TimingEvent* te = _timingEvents[i];
    item->setText(0, te->barBeatTickStr());
    item->setText(1, te->rowStr());
  }

  _tree->blockSignals(false);
}
