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
#include "muse.h"
#include "arranger.h"
#include "tlwidget.h"
#include "tlswidget.h"
#include "icons.h"
#include "widgets/simplebutton.h"
#include "configtrack.h"
#include "canvas.h"
#include "widgets/utils.h"
#include "widgets/splitter.h"
#include "mixer/astrip.h"
#include "mixer/mstrip.h"
#include "audio.h"
#include "shortcuts.h"
#include "trackinfo.h"
#include "midictrl.h"
#include "gconfig.h"
#include "part.h"

int Arranger::trackNameWidth = 80;

//---------------------------------------------------------
//   TrElement elements
//    list of available track gui elements
//---------------------------------------------------------

const TrElement trElements[] = {
      TrElement(TR_NAME,     1, "trackname",        -1),
      TrElement(TR_RECORD,   0, "record",
        Track::M_MIDI | Track::M_AUDIO_OUTPUT | Track::M_WAVE),
      TrElement(TR_OFF,      2, "off",
          Track::M_AUDIO_OUTPUT
        | Track::M_AUDIO_GROUP
        | Track::M_WAVE
        | Track::M_AUDIO_INPUT
        | Track::M_AUDIO_SOFTSYNTH),
      TrElement(TR_DRUMMAP,  2, "use drum map", Track::M_MIDI),
      TrElement(TR_MUTE,     3, "mute",             -1),
      TrElement(TR_SOLO,     3, "solo",             -1),
      TrElement(TR_MONITOR,  3, "monitor",
         Track::M_MIDI | Track::M_WAVE),
      TrElement(TR_AREAD,    4, "automation read",
            -1 & ~(Track::M_MIDI_IN | Track::M_MIDI)),
      TrElement(TR_AWRITE,   4, "automation write",
            -1 & ~(Track::M_MIDI_IN | Track::M_MIDI)),
      TrElement(TR_OCHANNEL,   5, "output channel", Track::M_MIDI),
      TrElement(TR_INSTRUMENT, 6, "instrument",     Track::M_MIDI_OUT),
      TrElement(TR_PATCH,      7, "patch",          Track::M_MIDI),
      };

const int nTrElements = sizeof(trElements)/sizeof(*trElements);

TrGroupList glist[Track::TRACK_TYPES];

extern void populateAddTrack(QMenu*);

//---------------------------------------------------------
//   sizeHint
//---------------------------------------------------------

QSize InfoStack::sizeHint() const 
      {
      return QSize(infoWidth, height());
      }

//---------------------------------------------------------
//   TLayout
//    simple layout for trackList
//---------------------------------------------------------

class TLayout : public QLayout {
      QList<QLayoutItem*> itemList;

   public:
      TLayout()  {}
      ~TLayout();

      void addItem(QLayoutItem* item);
      void insertWidget(int index, QWidget* item);
      Qt::Orientations expandingDirections() const { return 0; }
      bool hasHeightForWidth() const               { return false; }
      int count() const                            { return itemList.size(); }
      void setGeometry(const QRect &rect);
      QSize sizeHint() const               { return ((QWidget*)parent())->size(); }
      QLayoutItem *itemAt(int index) const { return itemList.value(index); }
      QLayoutItem *takeAt(int idx) {
            return idx >= 0 && idx < itemList.size() ? itemList.takeAt(idx) : 0;
            }
      void clear() {
            QLayoutItem* child;
            while ((child = takeAt(0)) != 0) {
                  delete child;
                  }
            }
      };

//---------------------------------------------------------
//   TLayout
//---------------------------------------------------------

TLayout::~TLayout()
      {
      QLayoutItem* child;
      while ((child = takeAt(0)) != 0)
            delete child;
      }

//---------------------------------------------------------
//   insertWidget
//---------------------------------------------------------

void TLayout::insertWidget(int index, QWidget* item)
      {
      if (item->parent() == 0)
            item->setParent((QWidget*)parent());
      itemList.insert(index, new QWidgetItem(item));
      update();
      }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

void TLayout::addItem(QLayoutItem* item)
      {
      itemList.append(item);
      update();
      }

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void TLayout::setGeometry(const QRect& r)
      {
      int y = r.y();
      int n = itemList.size();
      int width = r.width();  // ((QWidget*)parent())->width();
      for (int i = 0; i < n; ++i) {
            QLayoutItem *item = itemList.at(i);
            QWidget* w = item->widget();
            int h = w->height();
            w->setGeometry(0, y, width, h);
            y += h;
            }
      }

//---------------------------------------------------------
//   TlsvLayout
//---------------------------------------------------------

class TlsvLayout : public QLayout {
      QList<QLayoutItem*> itemList;
      int dx, dy;

   public:
      TlsvLayout()  {
            dx = 0;
            dy = 0;
            }
      ~TlsvLayout();

      void setOffset(int x, int y) {
            dx = x;
            dy = y;
            }
      void addItem(QLayoutItem* item);
      void insertWidget(int index, QWidget* item);
      Qt::Orientations expandingDirections() const { return 0; }
      bool hasHeightForWidth() const               { return false; }
      int count() const                            { return itemList.size(); }
      void setGeometry(const QRect &rect);
      QSize sizeHint() const               { return ((QWidget*)parent())->size(); }
      QLayoutItem *itemAt(int index) const { return itemList.value(index); }
      QLayoutItem *takeAt(int idx) {
            return idx >= 0 && idx < itemList.size() ? itemList.takeAt(idx) : 0;
            }
      };

//---------------------------------------------------------
//   TlsvLayout
//---------------------------------------------------------

TlsvLayout::~TlsvLayout()
      {
      QLayoutItem* child;
      while ((child = takeAt(0)) != 0)
            delete child;
      }

//---------------------------------------------------------
//   insertWidget
//---------------------------------------------------------

void TlsvLayout::insertWidget(int index, QWidget* item)
      {
      if (item->parent() == 0)
            item->setParent((QWidget*)parent());
      itemList.insert(index, new QWidgetItem(item));
      update();
      }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

void TlsvLayout::addItem(QLayoutItem* item)
      {
      itemList.append(item);
      update();
      }

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void TlsvLayout::setGeometry(const QRect& r)
      {
      QLayoutItem *item = itemList.at(0);
      QWidget* w = item->widget();
      w->setGeometry(dx, dy, r.width(), w->height());
      }

//---------------------------------------------------------
//   newAddTrackMenu
//---------------------------------------------------------

QMenu* newAddTrackMenu(QWidget* parent)
      {
      QMenu* menu = new QMenu(parent);
      populateAddTrack(menu);
      return menu;
      }

//---------------------------------------------------------
//   Arranger
//---------------------------------------------------------

Arranger::Arranger(QMainWindow* parent)
   : QWidget(parent)
      {
      setFocusPolicy(Qt::StrongFocus);
      setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

      for (int i = 0; i < Track::TRACK_TYPES; ++i)
            trackInfos[i] = 0;

      _curTrack = 0;
      strip     = 0;
      info      = 0;
      tool      = PointerTool;

      for (int tt = 0; tt < Track::TRACK_TYPES; ++tt) {
            int gn = 0;
            TrElementList group;
            for (int i = 0; i < nTrElements; ++i) {
                  if (!(trElements[i].trackMask & (1<<tt)))
                        continue;
                  if (trElements[i].grp != gn) {
                        glist[tt].push_back(group);
                        group.clear();
                        gn = trElements[i].grp;
                        }
                  group.push_back(&trElements[i]);
                  }
            if (!group.empty())
                  glist[tt].push_back(group);
            }

      configTrackEditor = 0;
      QLayout* ml = new QHBoxLayout;
      ml->setSpacing(0);
      ml->setMargin(0);
      setLayout(ml);

      infoDock = new QDockWidget(tr("TrackInfo"));
      infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
      infoDock->setMinimumWidth(infoWidth);
      infoDock->layout()->setMargin(1);
      infoDock->layout()->setSpacing(0);
      infoDockAction = infoDock->toggleViewAction();

      mixerDock = new QDockWidget(tr("Mix"));
      mixerDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
//      mixerDock->setMaximumWidth(STRIP_WIDTH);
      mixerDock->layout()->setMargin(1);
      mixerDock->layout()->setSpacing(0);
      mixerDockAction = mixerDock->toggleViewAction();

      parent->setDockNestingEnabled(true);

      parent->addDockWidget(Qt::LeftDockWidgetArea, infoDock, Qt::Horizontal);
      parent->splitDockWidget(infoDock, mixerDock, Qt::Horizontal);

      infoView = new QScrollArea;
      infoDock->setWidget(infoView);
      infoView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

      trackInfo = new InfoStack();
      infoView->setWidget(trackInfo);
      infoView->setWidgetResizable(true);
      trackInfo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);

      trackInfoVisible  = false;
      mixerStripVisible = false;

      infoDock->setVisible(false);
      mixerDock->setVisible(false);

      connect(infoDockAction, SIGNAL(toggled(bool)), SLOT(toggleTrackInfo(bool)));
      connect(mixerDockAction, SIGNAL(toggled(bool)), SLOT(toggleMixerStrip(bool)));

      tlsv = new TrackListWidget;
      tlsv->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
      tlsvLayout = new TlsvLayout;
      tlsv->setLayout(tlsvLayout);
      trackList = new QWidget;
      trackList->setAttribute(Qt::WA_StaticContents);
      trackList->setMouseTracking(true);

      tlsv->layout()->addWidget(trackList);
      tl = new TLayout;
      trackList->setLayout(tl);

      //
      //  Toolbox
      //
      QHBoxLayout* ttoolsLayout = new QHBoxLayout;
      ttoolsLayout->setMargin(1);
      ttoolsLayout->setSpacing(1);

      SimpleButton* configButton = new SimpleButton(configIcon, configIcon);
      configButton->setAutoRaise(true);
      configButton->setToolTip(tr("Config Tracklist"));
      configButton->setFixedHeight(rulerHeight-4);
      connect(configButton, SIGNAL(clicked()), SLOT(configTrackList()));
      ttoolsLayout->addWidget(configButton);
      ttoolsLayout->addStretch(100);

      gmute = newMuteButton();
      gmute->setFixedWidth(rulerHeight);
      gmute->setToolTip(tr("all mute off"));
      gmute->setFixedHeight(rulerHeight-4);
      ttoolsLayout->addWidget(gmute);
      setGMute();
      connect(song, SIGNAL(muteChanged(Track*,bool)), SLOT(setGMute()));
      connect(gmute, SIGNAL(clicked(bool)), SLOT(offGMute()));

      gsolo = newSoloButton();
      gsolo->setFixedWidth(rulerHeight);
      gsolo->setToolTip(tr("all solo off"));
      gsolo->setFixedHeight(rulerHeight-4);
      ttoolsLayout->addWidget(gsolo);
      setGSolo();
      connect(song, SIGNAL(soloChanged(Track*,bool)), SLOT(setGSolo()));
      connect(gsolo, SIGNAL(clicked(bool)), SLOT(offGSolo()));

      gar   = newAutoReadButton();
      gar->setFixedWidth(rulerHeight);
      gar->setToolTip(tr("all autoRead off"));
      gar->setFixedHeight(rulerHeight-4);
      ttoolsLayout->addWidget(gar);
      setGar();
      connect(song, SIGNAL(autoReadChanged(Track*,bool)), SLOT(setGar()));
      connect(gar, SIGNAL(clicked(bool)), SLOT(offGar()));

      gaw   = newAutoWriteButton();
      gaw->setFixedWidth(rulerHeight);
      gaw->setToolTip(tr("all autoWrite off"));
      gaw->setFixedHeight(rulerHeight-4);
      ttoolsLayout->addWidget(gaw);
      setGaw();
      connect(song, SIGNAL(autoWriteChanged(Track*,bool)), SLOT(setGaw()));
      connect(gaw, SIGNAL(clicked(bool)), SLOT(offGaw()));

      QHBoxLayout* infoboxLayout = new QHBoxLayout;
      infoboxLayout->setMargin(1);
      infoboxLayout->setSpacing(1);

      SimpleButton* tifButton = new SimpleButton(QString());
      tifButton->setCheckable(true);
      tifButton->setFixedSize(infoHeight-2, infoHeight);
      tifButton->setDefaultAction(infoDockAction);
      infoDockAction->setText(tr("i"));
      infoDockAction->setToolTip(tr("Show Track Info"));
      infoboxLayout->addWidget(tifButton);

      SimpleButton* mstButton = new SimpleButton(QString());
      mstButton->setCheckable(true);
      mstButton->setFixedSize(infoHeight-2, infoHeight);
      mstButton->setDefaultAction(mixerDockAction);
      mixerDockAction->setText(tr("m"));
      mixerDockAction->setToolTip(tr("Show Mixer Strip"));
      infoboxLayout->addWidget(mstButton);

      infoboxLayout->addStretch(100);

      split = new Splitter(Qt::Horizontal);
      split->setOpaqueResize(true);
      split->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      QWidget* tw = new QWidget;
      split->addWidget(tw);
      QVBoxLayout* trackListGrid = new QVBoxLayout;
      trackListGrid->setMargin(0);
      trackListGrid->setSpacing(0);
      tw->setLayout(trackListGrid);

      trackListGrid->addLayout(ttoolsLayout);
      QFrame* ruler = hLine(0);
      ruler->setLineWidth(2);
      trackListGrid->addWidget(ruler);
      trackListGrid->addWidget(tlsv, 100);
      trackListGrid->addLayout(infoboxLayout);

      //
      //  canvas widget
      //
      canvas = new PartCanvas;
      canvas->setCornerWidget(new QSizeGrip(canvas));
      split->addWidget(canvas);
      split->setStretchFactor(1, 100);
      canvas->setTool(tool);
      canvas->verticalScrollBar()->setSingleStep(minTrackHeight/2);

      connect(tlsv,   SIGNAL(mouseWheel(QWheelEvent*)), SLOT(mouseWheel(QWheelEvent*)));
      connect(song,   SIGNAL(posChanged(int,const AL::Pos&,bool)), canvas, SLOT(setLocatorPos(int,const AL::Pos&,bool)));
      connect(song,   SIGNAL(lenChanged(const AL::Pos&)), canvas, SLOT(setEndPos(const AL::Pos&)));
      connect(song,   SIGNAL(tempoChanged()), canvas, SLOT(tempoChanged()));
      connect(canvas, SIGNAL(kbdMovementUpdate(Track*, Part*)), SLOT(kbdMovementUpdate(Track*, Part*)));
      connect(muse,   SIGNAL(rasterChanged(int)), canvas, SLOT(setRaster(int)));
      connect(canvas, SIGNAL(cursorPos(const AL::Pos&,bool)), SIGNAL(cursorPos(const AL::Pos&,bool)));
      connect(canvas, SIGNAL(contentsMoving(int,int)),  SLOT(setTLViewPos(int,int)));
      connect(canvas, SIGNAL(posChanged(int,const AL::Pos&)), SLOT(setPos(int,const AL::Pos&)));

      connect(canvas, SIGNAL(createLRPart(Track*)),   song, SLOT(cmdCreateLRPart(Track*)));
      connect(canvas, SIGNAL(doubleClickPart(Part*)), SIGNAL(editPart(Part*)));
      connect(canvas, SIGNAL(startEditor(Part*,int)), muse, SLOT(startEditor(Part*,int)));
      connect(canvas, SIGNAL(partChanged(Part*,unsigned,unsigned)),
         song, SLOT(cmdChangePart(Part*,unsigned,unsigned)));
      connect(canvas, SIGNAL(addMarker(const AL::Pos&)), SLOT(addMarker(const AL::Pos&)));
      connect(canvas, SIGNAL(removeMarker(const AL::Pos&)), SLOT(removeMarker(const AL::Pos&)));

      layout()->addWidget(split);

      connect(muse, SIGNAL(configChanged()), SLOT(updateConfiguration()));
      connect(song, SIGNAL(trackSelectionChanged(Track*)), SLOT(setSelectedTrack(Track*)));
      connect(song, SIGNAL(trackRemoved(Track*)), SLOT(removeTrack(Track*)));
      connect(song, SIGNAL(trackAdded(Track*,int)), SLOT(insertTrack(Track*)));
      connect(muse, SIGNAL(startLoadSong()), SLOT(startLoadSong()));
      }

//---------------------------------------------------------
//   setPos
//---------------------------------------------------------

void Arranger::setPos(int idx, const AL::Pos& pos)
      {
	song->setPos(idx, pos.snaped(muse->raster()));
      }

//---------------------------------------------------------
//   setTLViewPos
//---------------------------------------------------------

void Arranger::setTLViewPos(int /*x*/, int y)
      {
#if 1
      int dy = y + trackList->y();
      tlsv->scroll(0, -dy);
#else
      trackList->setGeometry(0, -y, trackList->width(), trackList->height());
#endif
      tlsvLayout->setOffset(0, -y);
      }

//---------------------------------------------------------
//   tlIndex
//---------------------------------------------------------

int Arranger::tlIndex(Track* t) const
      {
      const TrackList* stl = song->tracks();

      int idx = 0;
      for (ciTrack i = stl->begin(); i != stl->end(); ++i, ++idx) {
            if (*i == t)
                  break;
            ArrangerTrack* at = &(*i)->arrangerTrack;
            if (at->tw == 0)
                  continue;
            idx += (*i)->subtracks.size();
            }
      return idx;
      }

int Arranger::tlIndex(ArrangerTrack* t) const
      {
      TrackList* stl = song->tracks();

      int idx = 0;
      for (ciTrack i = stl->begin(); i != stl->end(); ++i, ++idx) {
            ArrangerTrack* at = &(*i)->arrangerTrack;
            if (at->tw == 0)
                  continue;
            ArrangerTrackList& atl = (*i)->subtracks;
            for (iArrangerTrack k = atl.begin(); k != atl.end(); ++k) {
                  ++idx;
                  if (*k == t)
                        return idx;
                  }
            }
      return -1;  // crash
      }

//---------------------------------------------------------
//   insertTrack
//---------------------------------------------------------

void Arranger::insertTrack1(Track* t)
      {
      int idx = tlIndex(t);
      //-------------------------
      //   track list widget
      //-------------------------

      TLWidget* tw = new TLWidget(t, &glist[t->type()]);
      tw->setFixedHeight(t->arrangerTrack.h);
      tl->insertWidget(idx, tw);
      tw->show();	// needed if song is reloaded

      connect(tw,   SIGNAL(plusClicked(TLWidget*)),   SLOT(appendSubtrack(TLWidget*)));
      connect(tw,   SIGNAL(moveTrack(Track*,Track*)), SLOT(moveTrack(Track*,Track*)));
      connect(this, SIGNAL(configChanged()), tw,      SLOT(configChanged()));
      connect(tw,   SIGNAL(drag(int, int)),           SLOT(drag(int,int)));
      connect(tw,   SIGNAL(startDrag(int)),           SLOT(startDrag(int)));
      connect(t,    SIGNAL(partsChanged()), canvas->widget(), SLOT(update()));

      ArrangerTrack* at = &(t->arrangerTrack);
      at->tw = tw;
      }

void Arranger::insertTrack(Track* t)
      {
      insertTrack1(t);
      t->arrangerTrack.tw->show();
      updateIndex();
      if (_curTrack == 0)
            setSelectedTrack(_curTrack);
      }

//---------------------------------------------------------
//   removeTrack
//---------------------------------------------------------

void Arranger::removeTrack(Track* t)
      {
      ArrangerTrack* at = &t->arrangerTrack;
      if (at->tw == 0)
            return;

      tl->removeWidget(at->tw);

      at->tw->close();
      at->tw = 0;

      for (iArrangerTrack i = t->subtracks.begin(); i != t->subtracks.end(); ++i) {
            ArrangerTrack* at = *i;
            tl->removeWidget(at->tw);
            at->tw->close();
            }
      t->subtracks.clear();

      if (t == _curTrack) {
            if (!song->tracks()->isEmpty())
                  song->selectTrack(song->tracks()->front());
            }
      updateIndex();
      }

//---------------------------------------------------------
//   drag
//---------------------------------------------------------

void Arranger::drag(int trackIdx, int delta)
      {
      int h = startH + delta;
      if (h < minTrackHeight)
            h = minTrackHeight;
      ArrangerTrack* at = atrack(trackIdx);
      at->tw->setFixedHeight(h);
      updateIndex();
      }

//---------------------------------------------------------
//   startDrag
//---------------------------------------------------------

void Arranger::startDrag(int trackIdx)
      {
      ArrangerTrack* at = atrack(trackIdx);
      startH = at->tw->height();
      }

//---------------------------------------------------------
//   TrackListWidget
//---------------------------------------------------------

TrackListWidget::TrackListWidget(QWidget* parent)
   : QWidget(parent)
	{
	setAttribute(Qt::WA_NoSystemBackground);
      setAttribute(Qt::WA_StaticContents);
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void TrackListWidget::paintEvent(QPaintEvent* ev)
	{
	QPainter p(this);
	p.eraseRect(ev->rect());
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void TrackListWidget::mousePressEvent(QMouseEvent* ev)
      {
      if (ev->button() == Qt::RightButton) {
            QMenu* menu = newAddTrackMenu(this);
            menu->exec(ev->globalPos());
            }
      }

//---------------------------------------------------------
//   wheelEvent
//---------------------------------------------------------

void TrackListWidget::wheelEvent(QWheelEvent* e)
      {
      emit mouseWheel(e);
      }

//---------------------------------------------------------
//   mouseWheel
//	get redirected mouse wheel events from TrackListWidget
//---------------------------------------------------------

void Arranger::mouseWheel(QWheelEvent* e)
      {
	if (e->orientation() != Qt::Vertical)
      	return;
      QScrollBar* sb = canvas->verticalScrollBar();
	int step = qMin(QApplication::wheelScrollLines() * sb->singleStep(), sb->pageStep());
	if ((e->modifiers() & Qt::ControlModifier) || (e->modifiers() & Qt::ShiftModifier))
            step = sb->pageStep();
      int offset = e->delta() * step / 120;
      if (sb->invertedControls())
            offset = -offset;
      if (qAbs(offset) < 1)
            return;
      sb->setValue(sb->value() + offset);
      e->accept();
      }

//---------------------------------------------------------
//   appendSubtrack
//    the user requests a new controller subtrack
//---------------------------------------------------------

void Arranger::appendSubtrack(TLWidget* trackWidget)
      {
      Track* t = trackWidget->track();

      ArrangerTrack* at = new ArrangerTrack;
      at->h    = minTrackHeight;
      at->ctrl = CTRL_NO_CTRL;
      t->subtracks.push_back(at);
      if(initSubtrack(t, at)==true) {
            updateIndex();
            }
      else {
            t->subtracks.remove(at);
            delete at;
            }
      }

//---------------------------------------------------------
//   initSubtrack
//---------------------------------------------------------

bool Arranger::initSubtrack(Track* t, ArrangerTrack* at)
      {
      TLSWidget* tw = new TLSWidget(t, at, canvas);
      tw->setFixedHeight(at->h);

      if(tw->setCtrl(at->ctrl) == true) {
            tl->insertWidget(tlIndex(at), tw);
            at->tw = tw;
            connect(tw, SIGNAL(minusClicked(TLSWidget*)), SLOT(removeSubtrack(TLSWidget*)));
            connect(tw, SIGNAL(controllerChanged(int)), canvas->widget(), SLOT(update()));
            connect(tw, SIGNAL(drag(int, int)), SLOT(drag(int,int)));
            connect(tw, SIGNAL(startDrag(int)), SLOT(startDrag(int)));
            tw->show();
            }
      else {
            delete tw;
            return false;
            }
      return true;
      }

//---------------------------------------------------------
//   removeSubtrack
//---------------------------------------------------------

void Arranger::removeSubtrack(TLSWidget* w)
      {
      Track* t = w->track();
      for (iArrangerTrack it = t->subtracks.begin(); it != t->subtracks.end(); ++it) {
            ArrangerTrack* at = *it;
            if (at->tw == w) {
                  tl->removeWidget(at->tw);
//                  at->tw->close();
                  delete at->tw;
                  t->subtracks.erase(it);
                  delete at;
                  break;
                  }
            }
      updateIndex();
      }

//---------------------------------------------------------
//   configTrackList
//---------------------------------------------------------

void Arranger::configTrackList()
      {
      if (configTrackEditor == 0) {
            configTrackEditor = new ConfigTrackList(this);
            connect(configTrackEditor, SIGNAL(trackConfigChanged()), SIGNAL(configChanged()));
            }
      configTrackEditor->show();
      }

//---------------------------------------------------------
//   atrack
//---------------------------------------------------------

ArrangerTrack* Arranger::atrack(int idx)
      {
      int k = 0;
      TrackList* stl = song->tracks();
      for (iTrack i = stl->begin(); i != stl->end(); ++i) {
            ArrangerTrack* at = &(*i)->arrangerTrack;
            if (idx == k)
                  return at;
            ++k;
            for (iArrangerTrack it = (*i)->subtracks.begin(); it != (*i)->subtracks.end(); ++it) {
                  ArrangerTrack* t = *it;
                  if (idx == k)
                        return t;
                  ++k;
                  }
            }
      return 0;
      }

//---------------------------------------------------------
//   updateIndex
//    update vertical scrollbar & index values
//---------------------------------------------------------

void Arranger::updateIndex()
      {
      int idx = 0;
      int h   = 2 * defaultTrackHeight;  // always show room for at least two
      					     // tracks at end of list

      TrackList* stl = song->tracks();
      for (iTrack i = stl->begin(); i != stl->end(); ++i) {
            ArrangerTrack* at = &(*i)->arrangerTrack;
            if (at->tw == 0)
                  continue;
            ((TLWidget*)at->tw)->setIdx(idx);
            h += at->tw->height();
            ++idx;
            for (iArrangerTrack it = (*i)->subtracks.begin(); it != (*i)->subtracks.end(); ++it) {
                  ArrangerTrack* t = *it;
                  if (t->tw == 0)
                        continue;
                  ((TLSWidget*)t->tw)->setIdx(idx);
                  h += t->tw->height();
                  ++idx;
                  }
            }
      setGMute();
      setGSolo();
      setGar();
      setGaw();
      canvas->setVSize(h);
      trackList->setFixedHeight(h + 32);
      canvas->widget()->update();
      QPoint p(canvas->getWPos());
      setTLViewPos(0, p.y());
      }

//---------------------------------------------------------
//   toggleTrackInfo
//---------------------------------------------------------

void Arranger::toggleTrackInfo(bool val)
      {
      trackInfoVisible = val;
      if (_curTrack == 0)
            return;
      if (trackInfoVisible) {
            Track::TrackType t = _curTrack->type();
            TrackInfo* w = trackInfos[t];
            if (w == 0) {
                  w = trackInfos[t] = createTrackInfo();
                  trackInfo->addWidget(w);
                  }
            w->init(_curTrack);
            trackInfo->setCurrentWidget(w);
            }
      infoDock->layout()->invalidate();
      infoDock->layout()->update();
      }

//---------------------------------------------------------
//   toggleMixerStrip
//---------------------------------------------------------

void Arranger::toggleMixerStrip(bool val)
      {
      mixerStripVisible = val;
      if (mixerStripVisible && _curTrack) {
            if (strip && _curTrack != strip->getTrack()) {
                  strip->close();
                  strip = 0;
                  }
            if (!strip) {
                  switch(_curTrack->type()) {
                        case Track::MIDI_IN:
                              strip = new MidiInPortStrip(0, (MidiInPort*)_curTrack, false);
                              break;
                        case Track::MIDI_OUT:
                              strip = new MidiOutPortStrip(0, (MidiOutPort*)_curTrack, false);
                              break;
                        case Track::MIDI_SYNTI:
                              strip = new MidiSyntiStrip(0, (MidiSynti*)_curTrack, false);
                              break;
                        case Track::MIDI:
                              strip = new MidiStrip(0, (MidiTrack*)_curTrack, false);
                              break;
                        default:
                              strip = new AudioStrip(0, (AudioTrack*)_curTrack, false);
                              break;
                        }
                  strip->setFixedWidth(STRIP_WIDTH);
                  mixerDock->setWidget(strip);
                  }
            }
      else {
            if (strip) {
                  strip->close();
                  strip = 0;
                  }
            }
      }

//---------------------------------------------------------
//   startLoadSong
//---------------------------------------------------------

void Arranger::startLoadSong()
      {
      if (strip)
            strip->close();
      strip = 0;

      TrackList* stl = song->tracks();
      for (iTrack i = stl->begin(); i != stl->end(); ++i) {
            ArrangerTrack* at = &(*i)->arrangerTrack;
            tl->removeWidget(at->tw);
            at->tw->close();
            at->tw = 0;
            for (iArrangerTrack it = (*i)->subtracks.begin(); it != (*i)->subtracks.end(); ++it) {
                  ArrangerTrack* at = *it;
                  tl->removeWidget(at->tw);
                  at->tw->close();
                  }
            (*i)->subtracks.clear();
            }
      _curTrack = 0;
      }

//---------------------------------------------------------
//   endLoadSong
//    create track list widgets
//---------------------------------------------------------

void Arranger::endLoadSong()
      {
      TrackList* stl = song->tracks();

      for (iTrack i = stl->begin(); i != stl->end(); ++i) {
            Track* t = *i;
            insertTrack1(t);
            for (iArrangerTrack i = t->subtracks.begin(); i != t->subtracks.end(); ++i) {
                  initSubtrack(t, *i);
                  }
            }
      updateIndex();
      setSelectedTrack(song->selectedTrack());
      infoDock->setVisible(trackInfoVisible);
      mixerDock->setVisible(mixerStripVisible);
      }

//---------------------------------------------------------
//   updateConfiguration
//---------------------------------------------------------

void Arranger::updateConfiguration()
      {
      if (config.canvasUseBgPixmap) {
            canvas->setCanvasBackground(QPixmap(config.canvasBgPixmap));
            }
      else
            canvas->setCanvasBackground(config.canvasBgColor);
//TD      canvas->setShowGrid(config.canvasShowGrid);
//TD      update();
      }

//---------------------------------------------------------
//   readStatus
//---------------------------------------------------------

void Arranger::readStatus(QDomNode node)
      {
      TrackList* tl = song->tracks();
      iTrack it = tl->begin();

      QPoint wpos;
      double xmag = 0.05;
      double ymag = 1.0;

      for (; !node.isNull(); node = node.nextSibling()) {
            QDomElement e = node.toElement();
            QString tag(e.tagName());
            QString s = e.text();
            int i = s.toInt();
            if (tag == "info")
                  trackInfoVisible = i;
            else if (tag == "strip")
                  mixerStripVisible = i;
            else if (tag == "TrackConf") {
                  }
            else if (tag == "hmag")
                  xmag = s.toDouble();
            else if (tag == "vmag")
                  ymag = s.toDouble();
            else if (tag == "hpos")
                  wpos.setX(i);
            else if (tag == "vpos")
                  wpos.setY(i);
            else if (tag == "namesize") {
                  Arranger::trackNameWidth = i;
                  }
            else if (tag == "raster") {
                  muse->initRaster(i);
                  canvas->setRaster(i);
                  }
            else if (tag == "splitter") {
                  split->readStatus(node);
                  QList<int> sizes = split->sizes();
                  split->setSizes(sizes);
                  }
            else
                  printf("Arranger: unknown tag %s\n", tag.toLatin1().data());
            }
      canvas->setMag(xmag, ymag);
      canvas->setWPos(wpos);
      }

//---------------------------------------------------------
//   writeStatus
//---------------------------------------------------------

void Arranger::writeStatus(Xml& xml)
      {
      xml.tag("arranger");

      for (int i = 0; i < Track::TRACK_TYPES; ++i) {
            TrGroupList* gl = &glist[i];
            xml.tag("TrackConf type=\"%d\"", i);
            for (iTrGroup ig = gl->begin(); ig != gl->end(); ++ig) {
                  TrElementList& el = *ig;
                  xml.tag("group");
                  for (iTrElement ie = el.begin(); ie != el.end(); ++ie)
                        xml.tagE("element id=\"%d\"", (*ie)->id);
                  xml.etag("group");
                  }
            xml.etag("TrackConf");
            }
      xml.intTag("info", trackInfoVisible);
      xml.intTag("strip", mixerStripVisible);
      xml.doubleTag("hmag", canvas->xmag());
      xml.doubleTag("vmag", canvas->ymag());
      xml.intTag("hpos", canvas->getWPos().x());
      xml.intTag("vpos", canvas->getWPos().y());
      xml.intTag("raster", muse->raster());
      split->writeStatus("splitter", xml);
      xml.etag("arranger");
      }

//---------------------------------------------------------
//   setGMute
//---------------------------------------------------------

void Arranger::setGMute()
      {
      TrackList* tl = song->tracks();
      bool mute = false;
      for (iTrack i = tl->begin(); i != tl->end(); ++i) {
            if ((*i)->mute()) {
                  mute = true;
                  break;
                  }
            }
      gmute->setChecked(mute);
      }

//---------------------------------------------------------
//   setGSolo
//---------------------------------------------------------

void Arranger::setGSolo()
      {
      TrackList* tl = song->tracks();
      bool solo = false;
      for (iTrack i = tl->begin(); i != tl->end(); ++i) {
            if ((*i)->solo()) {
                  solo = true;
                  break;
                  }
            }
      gsolo->setChecked(solo);
      }

//---------------------------------------------------------
//   setGar
//---------------------------------------------------------

void Arranger::setGar()
      {
      TrackList* tl = song->tracks();
      bool ar = false;
      for (iTrack i = tl->begin(); i != tl->end(); ++i) {
            if ((*i)->autoRead()) {
                  ar = true;
                  break;
                  }
            }
      if (ar == false) {
            MidiTrackList* cl = song->midis();
            for (iMidiTrack i = cl->begin(); i != cl->end(); ++i) {
                  if ((*i)->autoRead()) {
                        ar = true;
                        break;
                        }
                  }
            }
      gar->setChecked(ar);
      }

//---------------------------------------------------------
//   setGaw
//---------------------------------------------------------

void Arranger::setGaw()
      {
      TrackList*tl = song->tracks();
      bool aw = false;
      for (iTrack i = tl->begin(); i != tl->end(); ++i) {
            if ((*i)->autoWrite())
                  aw = true;
            }
      if (aw == false) {
            MidiTrackList* cl = song->midis();
            for (iMidiTrack i = cl->begin(); i != cl->end(); ++i) {
                  if ((*i)->autoWrite()) {
                        aw = true;
                        break;
                        }
                  }
            }
      gaw->setChecked(aw);
      }

//---------------------------------------------------------
//   offGMute
//---------------------------------------------------------

void Arranger::offGMute()
      {
      TrackList*tl = song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i)
            song->setMute(*i,false);
      gmute->setChecked(false);
      }

//---------------------------------------------------------
//   offGSolo
//---------------------------------------------------------

void Arranger::offGSolo()
      {
      TrackList*tl = song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i)
            song->setSolo(*i, false);
      gsolo->setChecked(false);
      }

//---------------------------------------------------------
//   offGar
//---------------------------------------------------------

void Arranger::offGar()
      {
      TrackList*tl = song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i)
            song->setAutoRead(*i, false);
      MidiTrackList* cl = song->midis();
      for (iMidiTrack i = cl->begin(); i != cl->end(); ++i)
            song->setAutoRead(*i, false);
      gar->setChecked(false);
      }

//---------------------------------------------------------
//   offGaw
//---------------------------------------------------------

void Arranger::offGaw()
      {
      TrackList*tl = song->tracks();
      for (iTrack i = tl->begin(); i != tl->end(); ++i)
            song->setAutoWrite(*i, false);
      MidiTrackList* cl = song->midis();
      for (iMidiTrack i = cl->begin(); i != cl->end(); ++i)
            song->setAutoWrite(*i, false);
      gaw->setChecked(false);
      }

//---------------------------------------------------------
//   setTool
//---------------------------------------------------------

void Arranger::setTool(int t)
      {
      tool = Tool(t);
      canvas->setTool(tool);
      }

//---------------------------------------------------------
//   setSelectedTrack
//---------------------------------------------------------

void Arranger::setSelectedTrack(Track* t)
      {
      _curTrack = t;
      toggleTrackInfo(trackInfoVisible);
      toggleMixerStrip(mixerStripVisible);
      }

//---------------------------------------------------------
//   moveTrack
//    move src before dst
//---------------------------------------------------------

void Arranger::moveTrack(Track* src, Track* dst)
      {
      audio->msgMoveTrack(src, dst);
      tl->clear();
      TrackList* stl = song->tracks();
      for (iTrack i = stl->begin(); i != stl->end(); ++i) {
            tl->addWidget((*i)->arrangerTrack.tw);
            for (iArrangerTrack it = (*i)->subtracks.begin(); it != (*i)->subtracks.end(); ++it)
                  tl->addWidget((*it)->tw);
            }
      tl->setGeometry(((QWidget*)tl->parent())->geometry());
      updateIndex();
      }

//---------------------------------------------------------
//!    \fn Arranger::keyPressEvent(QKeyEvent* e)
//!    \brief Called when a key is pressed
//!    @param e The key event
//---------------------------------------------------------

void Arranger::keyPressEvent(QKeyEvent* e)
      {
#if 0 //TODOB
      int key = e->key();
      bool keypress_handled = false;

      if (e->modifiers() & Qt::ShiftModifier)
            key += Qt::SHIFT;
      if (e->modifiers() & Qt::AltModifier)
            key += Qt::ALT;
      if (e->modifiers() & Qt::ControlModifier)
            key += Qt::CTRL;

      if (shortcutsKbdMovement.isValid()) {
            if (key == shortcuts[SHRT_SEL_LEFT].key || key == shortcuts[SHRT_SEL_LEFT_ADD].key) {
                  keypress_handled = true;
                  bool add = (key == shortcuts[SHRT_SEL_LEFT_ADD].key);
                  PartList* parts = shortcutsKbdMovement.track()->parts();
                  Part* nextLeft = 0;

                  for (iPart i = parts->begin(); i != parts->end(); i++) {
                        Part* tmp = i->second;
                        if (!nextLeft) {
                              if (tmp->tick() < shortcutsKbdMovement.getLpos())
                                    nextLeft = tmp;
                              }
                        else {
                              if (tmp->tick() > nextLeft->tick() && tmp->tick() < shortcutsKbdMovement.getLpos() )
                                    nextLeft = tmp;
                              }
                        }
                  if (nextLeft) {
                        song->selectPart(nextLeft, add);
                        shortcutsKbdMovement.setPart(nextLeft);
                        shortcutsKbdMovement.setPos(nextLeft->tick(), nextLeft->tick() + nextLeft->lenTick());
                        }
                  }
            else if (key == shortcuts[SHRT_SEL_RIGHT].key || key == shortcuts[SHRT_SEL_RIGHT_ADD].key) {
                  keypress_handled = true;
                  bool add = (key == shortcuts[SHRT_SEL_RIGHT_ADD].key);
                  PartList* parts = shortcutsKbdMovement.track()->parts();
                  Part* nextRight = 0;

                  for (iPart i = parts->begin(); i != parts->end(); i++) {
                        Part* tmp = i->second;
                        if (!nextRight) {
                              if (tmp->tick() > shortcutsKbdMovement.getLpos())
                                    nextRight = tmp;
                              }
                        else {
                              if (tmp->tick() < nextRight->tick() && tmp->tick() > shortcutsKbdMovement.getLpos() )
                                    nextRight = tmp;
                              }
                        }
                  if (nextRight) {
                        song->selectPart(nextRight, add);
                        shortcutsKbdMovement.setPart(nextRight);
                        shortcutsKbdMovement.setPos(nextRight->tick(), nextRight->tick() + nextRight->lenTick());
                        }
                  }

            else if (key == shortcuts[SHRT_SEL_ABOVE].key) {
                  // TODO
                  }
            else if (key == shortcuts[SHRT_SEL_BELOW].key) {
                  // TODO
                  }
            } // -- end movement
      else {
            if (key == shortcuts[SHRT_TOOL_POINTER].key) {
                    emit toolChanged(PointerTool);
                    return;
                    }
            else if (key == shortcuts[SHRT_TOOL_PENCIL].key) {
                    emit toolChanged(PencilTool);
                    return;
                    }
            else if (key == shortcuts[SHRT_TOOL_RUBBER].key) {
                    emit toolChanged(RubberTool);
                    return;
                    }
            else if (key == shortcuts[SHRT_TOOL_LINEDRAW].key) {
                    emit toolChanged(DrawTool);
                    return;
                    }
            }
      // If we haven't dealt with the keypress, pass it along
      if (!keypress_handled) {
            e->ignore();
            }
#endif
      e->ignore();
      }


/*!
    \fn Arranger::kbdMovementUpdate(Track* t, Part* p)
    \brief Slot connected to canvaswidget, called when a part is selected
    @param t Track the selected part belongs to (null if no part selected)
    @param p The selected Part (null if no part selected)
 */
void Arranger::kbdMovementUpdate(Track* t, Part* p)
      {
      if (t && p ) {
            // If other track selected:
            if (t != shortcutsKbdMovement.track()) {
                  TrackList* stl = song->tracks();
                  for (iTrack i = stl->begin(); i != stl->end(); ++i) {
                        if (*i == t) {
                              // Set focus and select current track
                              t->arrangerTrack.tw->setFocus();
                              song->selectTrack(t);
                              }
                        }
                  }
            shortcutsKbdMovement.setTrack(t);
            shortcutsKbdMovement.setPart(p);
            shortcutsKbdMovement.setPos(p->tick(), p->tick() + p->lenTick());
            }
      else { // Deselected
            shortcutsKbdMovement.reset();
            }
      }

//---------------------------------------------------------
//   addMarker
//---------------------------------------------------------

void Arranger::addMarker(const AL::Pos& pos)
	{
	song->addMarker(QString(), pos);
      canvas->widget()->update();
      }

//---------------------------------------------------------
//   removeMarker
//---------------------------------------------------------

void Arranger::removeMarker(const AL::Pos& pos)
	{
      AL::MarkerList* ml = song->marker();
      for (AL::iMarker i = ml->begin(); i != ml->end(); ++i) {
            AL::iMarker ni = i;
            ++ni;
		if (i->second <= pos && (ni == ml->end() || ni->second > pos)) {
                  song->removeMarker(&(i->second));
      		canvas->widget()->update();
                  return;
                  }
            }
      printf("marker not found\n");
      }

