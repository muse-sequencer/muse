//=================================================================
//  miditracker
//  midi editor a la soundtracker
//    miditracker.cpp
//  (C) Copyright 2006 Nil Geisweiller (a_lin@user.sourceforge.net)
//=================================================================

#include "miditracker.h"
#include "trackpattern.h"
#include "song.h"
#include "muse.h"
#include "part.h"

#define MAX(x,y) (x>y?x:y)

class TrackPattern;

//---------------------------------------------------------
//   MidiTrackerEditor
//---------------------------------------------------------

MidiTrackerEditor::MidiTrackerEditor(PartList* pl, bool /*init*/)
  : MidiEditor(pl) {
  //--------
  //menuView
  //--------
  menuView = menuBar()->addMenu(tr("&View"));
  
  //-------
  //ToolBar
  //-------
  tools = addToolBar(tr("MidiTracker Tools"));
  tools->addAction(undoAction);
  tools->addAction(redoAction);
  tools->addSeparator();

  tools->addAction(stepRecAction);
  stepRecAction->setChecked(INIT_SREC);
  
  tools->addAction(midiInAction);
  midiInAction->setChecked(INIT_MIDIIN);
  
  tools->addAction(speaker);
  speaker->setChecked(INIT_SPEAKER);
  
  tools->addAction(followSongAction);
  followSongAction->setChecked(INIT_FOLLOW);
  setFollow(INIT_FOLLOW);

  //panic button
  QToolBar* panicToolbar = addToolBar(tr("Panic"));
  panicToolbar->addAction(panicAction);

  //Transport Bar
  QToolBar* transport = addToolBar(tr("Transport"));
  muse->setupTransportToolbar(transport);
  
  //frame containing the different matrices of time and notes and FX
  //QFrame* matricesFrame = new QFrame

  //second bar
  addToolBarBreak();
  //row per bar
  QToolBar* rowfeatures = addToolBar(tr("row features"));

  QLabel* quantLabel = new QLabel(tr("Quantize"));
  quantLabel->setIndent(5);
  rowfeatures->addWidget(quantLabel);
  _quantCombo = new QuantCombo(rowfeatures);
  rowfeatures->addWidget(_quantCombo);

  //QLabel* rpmLabel = new QLabel(tr("Row per bar"), rowfeatures);
  //rpmLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  //rpmLabel->setIndent(3);
  //rowfeatures->addWidget(rpmLabel);  
  //_rpmSpinBox = new QSpinBox(rowfeatures);
  //_rpmSpinBox->setRange(1, 256);
  //_rpmSpinBox->setFixedHeight(24);
  //rowfeatures->addWidget(_rpmSpinBox);

  //init row per bar
  setQuant(96); //corresponds to 16 quant
  updateQuant();

  //number of visible rows
  //rowfeatures->addSeparator();
  QLabel* nvrLabel = new QLabel(tr("Number of visible rows"), rowfeatures);
  nvrLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  nvrLabel->setIndent(3);
  rowfeatures->addWidget(nvrLabel);
  _nvrSpinBox = new QSpinBox(rowfeatures);
  _nvrSpinBox->setRange(1, 256);
  _nvrSpinBox->setFixedHeight(24);
  rowfeatures->addWidget(_nvrSpinBox);
  //init number of visible row
  setNumVisibleRows(20);
  updateNumVisibleRows();
  
  //evaluate fisrtTick and lastTick
  unsigned firstTick = _pl->begin()->second->tick();
  unsigned lastTick = 0;
  for(ciPart p = _pl->begin(); p != _pl->end(); ++p) {
    Part* part = p->second;
    lastTick = MAX(lastTick, part->endTick());
  }

  //-------------
  //timing matrix
  //-------------
//  TimingPattern* timingPattern =
//    new TimingPattern(this, "Timing", firstTick, lastTick, _quant);

  //---------------
  //tracks matrices
  //---------------
  for(ciPart p = _pl->begin(); p != _pl->end(); ++p) {
    Part* part = p->second;
    Track* track = part->track();
    if(track->isMidiTrack()) {
      bool trackNotFound = true;
      for(unsigned int i = 0; i < _trackPatterns.size(); i++)
	if(_trackPatterns[i]->getTrack()==track) trackNotFound = false;
      if(trackNotFound) {
	TrackPattern* tp; 
	tp = new TrackPattern(this, firstTick, _quant, pl, (MidiTrack*) track);
	_trackPatterns.push_back(tp);
      }
    }
  }

  /*
  addToolBarBreak();
  toolbar = new Toolbar1(initRaster, initQuant);
  addToolBar(toolbar);

  addToolBarBreak();
  info = new NoteInfo(this);
  addToolBar(info);

  setCentralWidget(tcanvas);
  tcanvas->setCornerWidget(new QSizeGrip(tcanvas));
  
  connect(song,   SIGNAL(posChanged(int,const AL::Pos&,bool)), canvas(),
	  SLOT(setLocatorPos(int,const AL::Pos&,bool)));
  connect(canvas(), SIGNAL(posChanged(int,const AL::Pos&)),
	  SLOT(setPos(int,const AL::Pos&)));
  
  connect(canvas(), SIGNAL(toolChanged(int)), tools2, SLOT(set(int)));
  connect(tools2, SIGNAL(toolChanged(int)), canvas(), SLOT(setTool(int)));
  
  connect(info, SIGNAL(valueChanged(NoteInfo::ValType, int)),
	  SLOT(noteinfoChanged(NoteInfo::ValType, int)));
  
  connect(canvas(), SIGNAL(selectionChanged(int, Event&, Part*)), this,
	  SLOT(setSelection(int, Event&, Part*)));
  
  info->setEnabled(false);
  
  setWindowTitle(canvas()->getCaption());
  int s1, e;
  canvas()->range(&s1, &e);
  e += AL::sigmap.ticksMeasure(e);  // show one more measure
  canvas()->setTimeRange(s1, e);*/
  
}

//---------------------------------------------------------
// setRowPerBar
//---------------------------------------------------------
void MidiTrackerEditor::setQuant(int q) {
  _quant = q;
}

//---------------------------------------------------------
// getRowPerBar
//---------------------------------------------------------
int MidiTrackerEditor::getQuant() {
  return _quant;
}

//---------------------------------------------------------
// updateRowPerBar
//---------------------------------------------------------
void MidiTrackerEditor::updateQuant() {
  _quantCombo->blockSignals(true);
  _quantCombo->setQuant(_quant);
  _quantCombo->blockSignals(false);  
}

//---------------------------------------------------------
// setNumVisibleRows
//---------------------------------------------------------
void MidiTrackerEditor::setNumVisibleRows(int nvr) {
  _numVisibleRows = nvr;
}

//---------------------------------------------------------
// getNumVisibleRows
//---------------------------------------------------------
int MidiTrackerEditor::getNumVisibleRows() {
  return _numVisibleRows;
}

//---------------------------------------------------------
// updateNumVisibleRows
//---------------------------------------------------------
void MidiTrackerEditor::updateNumVisibleRows() {
  _nvrSpinBox->blockSignals(true);
  _nvrSpinBox->setValue(_numVisibleRows);
  _nvrSpinBox->blockSignals(false);  
}

//---------------------------------------------------------
// setFollow
//---------------------------------------------------------
void MidiTrackerEditor::setFollow(bool f) {
  _follow = f;
}

//---------------------------------------------------------
//   cmd
//    pulldown menu commands
//---------------------------------------------------------

void MidiTrackerEditor::cmd(QAction* /*a*/) {
  //int cmd = a->data().toInt();
  //canvas()->cmd(cmd, _quantStrength, _quantLimit, _quantLen);
}

