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

#include "midieditor.h"
#include "part.h"
#include "song.h"
#include "widgets/tools.h"
#include "ecanvas.h"
#include "icons.h"
#include "audio.h"

//---------------------------------------------------------
//   MidiEditor
//---------------------------------------------------------

MidiEditor::MidiEditor(PartList* pl)
   : Editor()
      {
      _pl     = pl;
      selPart = 0;
      tools2  = 0;
      info    = 0;
      tools   = 0;
      toolbar = 0;

      QMenuBar* mb = menuBar();

      //---------Menü-------------------------------
      menuEdit = new QMenu(tr("&Edit"));
      mb->addMenu(menuEdit);

      menuEdit->addAction(undoAction);
      menuEdit->addAction(redoAction);

      menuEdit->addSeparator();
      cutAction = menuEdit->addAction(tr("Cut"));
      cutAction->setIcon(*editcutIconSet);
      cutAction->setData(MidiEditor::CMD_CUT);
      cutAction->setShortcut(Qt::CTRL+Qt::Key_X);

      copyAction = menuEdit->addAction(tr("Copy"));
      copyAction->setIcon(*editcopyIconSet);
      copyAction->setData(MidiEditor::CMD_COPY);
      copyAction->setShortcut(Qt::CTRL+Qt::Key_C);

      pasteAction = menuEdit->addAction(tr("Paste"));
      pasteAction->setIcon(*editpasteIconSet);
      pasteAction->setData(MidiEditor::CMD_PASTE);
      pasteAction->setShortcut(Qt::CTRL+Qt::Key_V);

      speaker = new QAction(this);
      speaker->setCheckable(true);
      speaker->setIcon(QIcon(*speakerIcon));
      speaker->setText(tr("Play Events"));
      speaker->setToolTip(tr("Play Events"));

      stepRecAction = new QAction(this);
      stepRecAction->setIcon(QIcon(*steprecIcon));
      stepRecAction->setText(tr("Step Record"));
      stepRecAction->setToolTip(tr("Step Record"));
      stepRecAction->setCheckable(true);

      midiInAction = new QAction(this);
      midiInAction->setIcon(QIcon(*midiinIcon));
      midiInAction->setText(tr("Midi Input"));
      midiInAction->setToolTip(tr("Midi Input"));
      midiInAction->setCheckable(true);

      followSongAction = new QAction(this);
      followSongAction->setText("F");
      followSongAction->setToolTip(tr("Follow Song"));
      followSongAction->setCheckable(true);

      connect(song, SIGNAL(songChanged(int)), SLOT(songChanged(int)));

      QClipboard* cb = QApplication::clipboard();
      connect(cb, SIGNAL(dataChanged()), SLOT(clipboardChanged()));
      connect(cb, SIGNAL(selectionChanged()), SLOT(clipboardChanged()));
      connect(menuEdit, SIGNAL(triggered(QAction*)), SLOT(midiCmd(QAction*)));
      }

//---------------------------------------------------------
//   cmd
//---------------------------------------------------------

void MidiEditor::midiCmd(QAction* a)
      {
      switch (a->data().toInt()) {
            case CMD_CUT:
            	{
                  copy();
                  song->startUndo();
      		CItemList* items = canvas()->getItems();
                  for (iCItem i = items->begin(); i != items->end(); ++i) {
                        if (!i->second->isSelected())
                              continue;
                        CItem* e    = i->second;
                        Event event = e->event;
                        audio->msgDeleteEvent(event, e->part, false);
                        }
                  song->endUndo(SC_EVENT_REMOVED);
                  }
                  break;
            case CMD_COPY:
                  copy();
                  break;
            default:
            	cmd(a);
            	break;
            }
      }

//---------------------------------------------------------
//   copy
//    cut copy paste
//---------------------------------------------------------

void MidiEditor::copy()
      {
      QMimeData* drag = canvas()->getTextDrag();
      if (drag)
            QApplication::clipboard()->setMimeData(drag, QClipboard::Selection);
      }


//---------------------------------------------------------
//   setPos
//	snap locator positioning
//---------------------------------------------------------

void MidiEditor::setPos(int idx, const AL::Pos& pos)
      {
	song->setPos(idx, pos.snaped(_raster));
      }

//---------------------------------------------------------
//   genPartlist
//---------------------------------------------------------

void MidiEditor::genPartlist()
      {
      MidiTrackList* tl = song->midis();
      PartList* npl = new PartList;
      for (iPart ip = _pl->begin(); ip != _pl->end(); ++ip) {
            Part* part = ip->second;
      	for (iMidiTrack it = tl->begin(); it != tl->end(); ++it) {
                  PartList* pl2 = (*it)->parts();
                  iPart ip2 = pl2->begin();
                  for (; ip2 != pl2->end(); ++ip2)
                        if (ip2->second == part) {
                        	npl->add(part);
                        	break;
                        }
                  if (ip2 != pl2->end())
                        break;
                  }
            }
      delete _pl;
      _pl = npl;
      for (iPart ip = _pl->begin(); ip != _pl->end(); ++ip) {
      	if (ip->second == selPart)
                  return;
            }
      if (_pl->empty())
            return;
      selPart = _pl->begin()->second;
      if (canvas())
      	canvas()->setCurPart(selPart);
      }

//---------------------------------------------------------
//   MidiEditor
//---------------------------------------------------------

MidiEditor::~MidiEditor()
      {
      if (_pl)
            delete _pl;
      }

//---------------------------------------------------------
//   writePartList
//---------------------------------------------------------

void MidiEditor::writePartList(Xml& xml) const
      {
      for (ciPart p = _pl->begin(); p != _pl->end(); ++p) {
            Part* part   = p->second;
            Track* track = part->track();
            int trkIdx   = song->tracks()->indexOf(track);
            int partIdx  = track->parts()->index(part);
            xml.tag("part");
            xml.put("%d:%d", trkIdx, partIdx);
            xml.etag("part");
            }
      }

//---------------------------------------------------------
//   songChanged
//---------------------------------------------------------

void MidiEditor::songChanged(int type)
      {
      if (type & (SC_PART_REMOVED | SC_PART_MODIFIED
         | SC_PART_INSERTED | SC_TRACK_REMOVED)) {
            genPartlist();
            // close window if editor has no parts anymore
            if (parts()->empty()) {
                  close();
                  return;
                  }
            }
      canvas()->songChanged(type);
      }

//---------------------------------------------------------
//   setQuant
//---------------------------------------------------------

void MidiEditor::setQuant(int val)
	{
      _quant = val;
      if (toolbar)
	      toolbar->setQuant(val);
      if (canvas() && canvas()->part())
            canvas()->part()->setQuant(val);
      }

//---------------------------------------------------------
//   setRaster
//---------------------------------------------------------

void MidiEditor::setRaster(int val)
	{
      _raster = val;
      if (toolbar)
	      toolbar->setRaster(val);
      if (canvas()) {
            canvas()->setRaster(val);
      	if (canvas()->part())
	            canvas()->part()->setRaster(val);
            }
      }

//---------------------------------------------------------
//   setApplyTo
//---------------------------------------------------------

void MidiEditor::setApplyTo(int val)
	{
      _applyTo = val;
      if (toolbar)
            toolbar->setApplyTo(_applyTo);
      }

//---------------------------------------------------------
//   rasterStep
//---------------------------------------------------------

int MidiEditor::rasterStep(unsigned tick) const
	{
      return AL::sigmap.rasterStep(tick, raster());
      }

//---------------------------------------------------------
//   rasterVal
//---------------------------------------------------------

unsigned MidiEditor::rasterVal(unsigned v)  const
	{
      return AL::sigmap.raster(v, raster());
      }

//---------------------------------------------------------
//   rasterVal1
//---------------------------------------------------------

unsigned MidiEditor::rasterVal1(unsigned v) const
	{
      return AL::sigmap.raster1(v, raster());
      }

//---------------------------------------------------------
//   rasterVal2
//---------------------------------------------------------

unsigned MidiEditor::rasterVal2(unsigned v) const
	{
      return AL::sigmap.raster2(v, raster());
      }

//---------------------------------------------------------
//   quantVal
//---------------------------------------------------------

int MidiEditor::quantVal(int v) const
	{
      int q = quant();
      int val = ((v+q/2)/q)*q;
      if (val == 0)
            val = q;
      return val;
      }

//---------------------------------------------------------
//   xmag
//---------------------------------------------------------

double MidiEditor::xmag() const
   	{
      return tcanvas->xmag();
      }

//---------------------------------------------------------
//   setXmag
//---------------------------------------------------------

void MidiEditor::setXmag(double val)
	{
      if (canvas())
	      canvas()->setMag(val, tcanvas->ymag());
      }

//---------------------------------------------------------
//   tool
//---------------------------------------------------------

int MidiEditor::tool() const
	{
      return tcanvas->tool();
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void MidiEditor::setTool(int n)
      {
	tcanvas->setTool(n);
	tools2->set(n);
      }

//---------------------------------------------------------
//   canvasPos
//---------------------------------------------------------

QPoint MidiEditor::canvasPos() const
      {
      return tcanvas->getWPos();
      }

//---------------------------------------------------------
//   setCanvasPos
//---------------------------------------------------------

void MidiEditor::setCanvasPos(const QPoint& p)
      {
	if (tcanvas)
      	tcanvas->setWPos(p);
      }

//---------------------------------------------------------
//   clipboardChanged
//---------------------------------------------------------

void MidiEditor::clipboardChanged()
      {
	QString stype("x-muse-eventlist");
      QString s = QApplication::clipboard()->text(stype, QClipboard::Selection);
      pasteAction->setEnabled(!s.isEmpty());
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void MidiEditor::selectionChanged()
      {
      bool flag = canvas()->selectionSize() > 0;
      cutAction->setEnabled(flag);
      copyAction->setEnabled(flag);
      }

//---------------------------------------------------------
//   read
//---------------------------------------------------------

void MidiEditor::read(QDomNode node)
      {
      for (node = node.firstChild(); !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            if (tag == "CtrlEdit") {
                  int id = e.attribute("id","0").toInt();
                  int h = e.attribute("h","50").toInt();
                  canvas()->addController(id, h);
                  }
            else
      		AL::readProperties(this, node);
            }
      canvas()->layout1();
      }

//---------------------------------------------------------
//   write
//---------------------------------------------------------

void MidiEditor::write(Xml& xml) const
      {
	writePartList(xml);
      xml.tag(metaObject()->className());
      xml.writeProperties(this);
      const CtrlEditList* el = canvas()->getCtrlEditors();
      for (ciCtrlEdit i = el->begin(); i != el->end(); ++i) {
            xml.tagE("CtrlEdit h=\"%d\" id=\"%d\"",
               (*i)->height(), (*i)->ctrl()->id());
            }
      xml.etag(metaObject()->className());
      }

//---------------------------------------------------------
//   initFromPart
//---------------------------------------------------------

void MidiEditor::initFromPart()
      {
      Part* part         = canvas()->part();
	CtrlCanvasList* cl = part->getCtrlCanvasList();
      if (!cl->empty()) {
#if 0
	      for (iCtrlCanvas i = cl->begin(); i != cl->end(); ++i) {
printf("MidiEditor::initFromPart(): add controller %d %d\n", i->ctrlId, i->height);
			canvas()->addController(i->ctrlId, i->height);
                  }
#endif
	      canvas()->layout1();
            }
      }

