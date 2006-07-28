//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#include "song.h"
#include "masteredit.h"
#include "awl/poslabel.h"
#include "awl/tempoedit.h"
#include "awl/tempolabel.h"
#include "awl/sigedit.h"
#include "master.h"
#include "al/tempo.h"
#include "widgets/tools.h"

static int rasterTable[] = { 1, 0, 768, 384, 192, 96 };
static const char* rastval[] = {
            QT_TR_NOOP("Off"), "Bar", "1/2", "1/4", "1/8", "1/16"
            };

int MasterEdit::initRaster  = MasterEdit::INIT_RASTER;
int MasterEdit::initWidth   = MasterEdit::INIT_WIDTH;
int MasterEdit::initHeight  = MasterEdit::INIT_HEIGHT;
double MasterEdit::initXmag = 0.04;
double MasterEdit::initYmag = 1.0;

//---------------------------------------------------------
//   MasterEdit
//---------------------------------------------------------

MasterEdit::MasterEdit()
   : Editor()
      {
      setWindowTitle(tr("MusE: Mastertrack"));

      //---------ToolBar----------------------------------
      tools = addToolBar(tr("edit tools"));
      tools->addAction(undoAction);
      tools->addAction(redoAction);
      tools->addSeparator();

      tools2 = new EditToolBar(this,
         PointerTool | PencilTool | RubberTool);
      addToolBar(tools2);

      QToolBar* enableMaster = addToolBar(tr("EnableTempomap"));
      enableMasterAction = enableMaster->addAction(tr("Tempomap"));
      enableMasterAction->setCheckable(true);
      enableMasterAction->setToolTip(tr("Enable use of tempo map"));
      enableMasterAction->setChecked(song->masterFlag());
      connect(enableMasterAction, SIGNAL(triggered(bool)), song, SLOT(setMasterFlag(bool)));

      addToolBarBreak();
      QToolBar* info = new QToolBar(tr("Info"), this);
      addToolBar(info);

      QLabel* label = new QLabel(tr("Cursor"));
      info->addWidget(label);

      cursorPos = new PosLabel;
      info->addWidget(cursorPos);
      tempo     = new TempoLabel(info);
      info->addWidget(tempo);

      info->addWidget(new QLabel(tr("Snap")));
      rasterLabel = new QComboBox;
      info->addWidget(rasterLabel);
      for (int i = 0; i < 6; i++)
            rasterLabel->addItem(tr(rastval[i]), i);
      connect(rasterLabel, SIGNAL(activated(int)), SLOT(_setRaster(int)));

      cursorPos->setToolTip(tr("time at cursor position"));
      tempo->setToolTip(tr("tempo at cursor position"));

      //---------values for current position---------------
      info->addWidget(new QLabel(tr("CurPos ")));
      curTempo = new TempoEdit(info);
      info->addWidget(curTempo);
      curSig   = new SigEdit(info);
      info->addWidget(curSig);
      curSig->setValue(AL::TimeSignature(4, 4));
      curTempo->setToolTip(tr("tempo at current position"));
      curSig->setToolTip(tr("time signature at current position"));
      connect(curSig, SIGNAL(valueChanged(const AL::TimeSignature&)), song, SLOT(setSig(const AL::TimeSignature&)));
      connect(curTempo, SIGNAL(tempoChanged(int)), song, SLOT(setTempo(int)));

      //---------------------------------------------------
      //    master
      //---------------------------------------------------

      tcanvas = new MasterCanvas;
      setCentralWidget(tcanvas);
      tcanvas->setEndPos(AL::Pos(song->len()));
      setRaster(INIT_RASTER);
      tcanvas->setMag(initXmag, initYmag);

      connect(song, SIGNAL(posChanged(int,const AL::Pos&,bool)), tcanvas, SLOT(setLocatorPos(int,const AL::Pos&,bool)));
      connect(song, SIGNAL(posChanged(int,const AL::Pos&,bool)), SLOT(posChanged(int,const AL::Pos&,bool)));
      connect(song, SIGNAL(lenChanged(const AL::Pos&)), tcanvas, SLOT(setEndPos(const AL::Pos&)));
      connect(song, SIGNAL(tempoChanged()), tcanvas, SLOT(tempoChanged()));

      connect(tcanvas, SIGNAL(posChanged(int,const AL::Pos&)), song, SLOT(setPos(int,const AL::Pos&)));
      connect(tcanvas, SIGNAL(cursorPos(const AL::Pos&,bool)),  cursorPos, SLOT(setValue(const AL::Pos&,bool)));

      connect(tcanvas, SIGNAL(tempoChanged(int)), SLOT(setTempo(int)));
      connect(tools2, SIGNAL(toolChanged(int)), tcanvas, SLOT(setTool(int)));
      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

      resize(initWidth, initHeight);
      }

//---------------------------------------------------------
//   ~MasterEdit
//---------------------------------------------------------

MasterEdit::~MasterEdit()
      {
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MasterEdit::songChanged(int type)
      {
      if (type & SC_TEMPO) {
            int tempo = AL::tempomap.tempo(song->cpos());
            curTempo->setTempo(tempo);
            }
      if (type & SC_MASTER)
      	enableMasterAction->setChecked(song->masterFlag());
      }

//---------------------------------------------------------
//   _setRaster
//---------------------------------------------------------

void MasterEdit::_setRaster(int index)
      {
	setRaster(rasterTable[index]);
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void MasterEdit::setRaster(int val)
      {
      _raster = val;
	tcanvas->setRaster(val);
      for (unsigned i = 0; i < sizeof(rasterTable)/sizeof(*rasterTable); ++i) {
            if (rasterTable[i] == val) {
                  rasterLabel->setCurrentIndex(i);
            	return;
                  }
            }
      rasterLabel->setCurrentIndex(1);
      }

//---------------------------------------------------------
//   posChanged
//---------------------------------------------------------

void MasterEdit::posChanged(int idx, const AL::Pos& pos, bool)
      {
      if (idx == 0) {
            unsigned val = pos.tick();
            int tempo = AL::tempomap.tempo(val);
            AL::TimeSignature sig = AL::sigmap.timesig(val);
            curSig->blockSignals(true);

            curTempo->setTempo(tempo);
            curSig->setValue(sig);

            curSig->blockSignals(false);
            }
      }

//---------------------------------------------------------
//   setTempo
//---------------------------------------------------------

void MasterEdit::setTempo(int val)
      {
      if (val == -1)
            tempo->setEnabled(false);
      else {
            tempo->setEnabled(true);
            tempo->setValue(val);
            }
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MasterEdit::read(QDomNode node)
      {
      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling())
      	AL::readProperties(this, node);
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MasterEdit::write(Xml& xml) const
      {
      xml.tag(metaObject()->className());
      xml.writeProperties(this);
      xml.etag(metaObject()->className());
      }

//---------------------------------------------------------
//   tool
//---------------------------------------------------------

int MasterEdit::tool() const
	{
      return tcanvas->tool();
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void MasterEdit::setTool(int n)
      {
	tcanvas->setTool(n);
	tools2->set(n);
      }

//---------------------------------------------------------
//   canvasPos
//---------------------------------------------------------

QPoint MasterEdit::canvasPos() const
      {
      return tcanvas->getWPos();
      }

//---------------------------------------------------------
//   setCanvasPos
//---------------------------------------------------------

void MasterEdit::setCanvasPos(const QPoint& p)
      {
	tcanvas->setWPos(p);
      }

//---------------------------------------------------------
//   xmag
//---------------------------------------------------------

double MasterEdit::xmag() const
   	{
      return tcanvas->xmag();
      }

//---------------------------------------------------------
//   setXmag
//---------------------------------------------------------

void MasterEdit::setXmag(double val)
	{
	tcanvas->setMag(val, tcanvas->ymag());
      }

//---------------------------------------------------------
//   ymag
//---------------------------------------------------------

double MasterEdit::ymag() const
   	{
      return tcanvas->ymag();
      }

//---------------------------------------------------------
//   setYmag
//---------------------------------------------------------

void MasterEdit::setYmag(double val)
	{
	tcanvas->setMag(tcanvas->xmag(), val);
      }


