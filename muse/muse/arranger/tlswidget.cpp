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
#include "track.h"
#include "tlswidget.h"
#include "tlwlayout.h"
#include "icons.h"
#include "arranger.h"
#include "widgets/simplebutton.h"
#include "muse.h"
#include "ctrl/configmidictrl.h"
#include "ctrl/ctrldialog.h"
#include "midictrl.h"
#include "widgets/utils.h"

static Ctrl veloList(CTRL_VELOCITY, "velocity", Ctrl::DISCRETE, 0.0, 127.0);    // dummy

//---------------------------------------------------------
//   TLSLayout
//---------------------------------------------------------

class TLSLayout : public QLayout {
      QList<QLayoutItem*> itemList;

   public:
      TLSLayout()  {}
      ~TLSLayout();

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
//   TLSLayout
//---------------------------------------------------------

TLSLayout::~TLSLayout()
      {
      QLayoutItem* child;
      while ((child = takeAt(0)) != 0)
            delete child;
      }

//---------------------------------------------------------
//   insertWidget
//---------------------------------------------------------

void TLSLayout::insertWidget(int index, QWidget* item)
      {
      if (item->parent() == 0)
            item->setParent((QWidget*)parent());
      itemList.insert(index, new QWidgetItem(item));
      update();
      }

//---------------------------------------------------------
//   addItem
//---------------------------------------------------------

void TLSLayout::addItem(QLayoutItem* item)
      {
      itemList.append(item);
      update();
      }

//---------------------------------------------------------
//   setGeometry
//---------------------------------------------------------

void TLSLayout::setGeometry(const QRect& rect)
      {
      static const int labelWidth = 50;
      int x1 = rect.x() + labelWidth;
      int y  = rect.y();
      int y2 = y + rect.height();

      QLayoutItem *item = itemList.at(0);
      QSize size(item->sizeHint());
      item->widget()->setGeometry(QRect(x1 - 18, y2 - 18-splitWidth, 18, 18));

      item = itemList.at(1);
      size = item->sizeHint();
      item->setGeometry(QRect(x1, rect.y(), size.width(), trackRowHeight));
      }

//---------------------------------------------------------
//   TLSWidget
//---------------------------------------------------------

TLSWidget::TLSWidget(Track* t, ArrangerTrack* atrack, TimeCanvas* timeC)
      {
      setAttribute(Qt::WA_NoBackground);
      setMouseTracking(true);
      _tc   = timeC;
      state = S_NORMAL;

      at         = atrack;
      _track     = t;
      _ctrlTrack = t;

      TLSLayout* l = new TLSLayout;
      setLayout(l);
      //
      //  track type
      //
      SimpleButton* minus = newMinusButton();
      minus->setToolTip(tr("Remove Subtrack"));
      l->addWidget(minus);

      ctrlList = new QToolButton;
      ctrlList->setText(tr("Ctrl"));

      connect(ctrlList, SIGNAL(clicked()), SLOT(showControllerList()));

      l->addWidget(ctrlList);
      ctrlList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

      connect(_track, SIGNAL(selectionChanged(bool)), SLOT(selectionChanged()));
      connect(_track, SIGNAL(controllerChanged(int)), SLOT(controllerListChanged(int)));
      connect(_track, SIGNAL(autoReadChanged(bool)), SLOT(autoReadChanged()));
      connect(muse, SIGNAL(configChanged()), SLOT(configChanged()));
      connect(minus, SIGNAL(clicked()), SLOT(labelMinusClicked()));
      configChanged();
      }

//---------------------------------------------------------
//   ctrl
//---------------------------------------------------------

Ctrl* TLSWidget::ctrl() const
      {
      return at->controller;
      }

//---------------------------------------------------------
//   height
//---------------------------------------------------------

int TLSWidget::cheight() const
      {
      return at->tw->height() - splitWidth;
      }

//---------------------------------------------------------
//   selectionChanged
//---------------------------------------------------------

void TLSWidget::selectionChanged()
      {
//      setFrameShadow(_track->selected() ? QFrame::Sunken : QFrame::Raised);
      }

//---------------------------------------------------------
//   configChanged
//---------------------------------------------------------

void TLSWidget::configChanged()
      {
      QColor c(_track->ccolor());
      QPalette p(palette());
      p.setColor(QPalette::Background, c.light(100));
      setPalette(p);
      }

//---------------------------------------------------------
//   showControllerList
//---------------------------------------------------------

bool TLSWidget::showControllerList()
      {
      Ctrl* c = ctrl();
      int id;
      if (c)
            id = c->id();
      else
            id = CTRL_NO_CTRL;
      for (;;) {
            CtrlDialog cd(_ctrlTrack, id);
            int rv = cd.exec();
            if (rv != 1)
                  return false;
            id = cd.curId();
            if (id == CTRL_NO_CTRL)
                  return false;
            if (id != CTRL_OTHER)
                  break;
            ConfigMidiCtrl* mce = new ConfigMidiCtrl((MidiTrack*)_track);
            mce->exec();
            delete mce;
            }
      setCtrl(id);
      return true;
      }

//---------------------------------------------------------
//   setCtrl
//---------------------------------------------------------

bool TLSWidget::setCtrl(int ctrl)
      {
      if (ctrl == CTRL_NO_CTRL || ctrl == CTRL_OTHER) {
            // this controller subtrack is new, ask user for 
            // controller:
            return showControllerList();
            }
      
      if (_ctrlTrack && _ctrlTrack != _track) {
            disconnect(_ctrlTrack, SIGNAL(controllerChanged(int)), this, SLOT(controllerListChanged(int)));
            }
      if (ctrl == CTRL_VELOCITY) {
            at->controller = &veloList;
            at->ctrl = CTRL_VELOCITY;
            ctrlList->setText(tr("Velocity"));
            _ctrlTrack = _track;
            emit controllerChanged(at->ctrl);
            }
      else {
            at->ctrl = ctrl;
            at->controller = _track->getController(ctrl);
            if (at->controller == 0 && _track->type() == Track::MIDI) {
                  MidiChannel* mc = ((MidiTrack*)_track)->channel();
                  at->controller = mc->getController(ctrl);
                  _ctrlTrack = mc;
                  connect(_ctrlTrack, SIGNAL(controllerChanged(int)), SLOT(controllerListChanged(int)));
                  }
            else
                  _ctrlTrack = _track;
            ctrlList->setText(at->controller->name());
            emit controllerChanged(ctrl);
            }
      return true;
      }

//---------------------------------------------------------
//   labelMinusClicked
//---------------------------------------------------------

void TLSWidget::labelMinusClicked()
      {
      emit minusClicked(this);
      }

//---------------------------------------------------------
//   mousePressEvent
//---------------------------------------------------------

void TLSWidget::mousePressEvent(QMouseEvent* ev)
      {
      if (ev->button() == Qt::RightButton) {
            QMenu* menu = new QMenu(this);
            QAction* a = menu->addAction(tr("Delete Controller"));
            a->setData(0);
            QAction* rv = menu->exec(ev->globalPos());
            if (rv == 0)
                  return;
            emit minusClicked(this);
            return;
            }
      song->selectTrack(_track);

      int y = ev->pos().y();
      int wh = height();
      starty = ev->globalPos().y();
      if (y > (wh - splitWidth)) {
            state  = S_DRAGBOTTOM;
            emit startDrag(trackIdx);
            }
      }

//---------------------------------------------------------
//   mouseReleaseEvent
//---------------------------------------------------------

void TLSWidget::mouseReleaseEvent(QMouseEvent*)
      {
      state  = S_NORMAL;
      }

//---------------------------------------------------------
//   mouseMoveEvent
//---------------------------------------------------------

void TLSWidget::mouseMoveEvent(QMouseEvent* ev)
      {
      QPoint pos(ev->pos());

      if (state == S_DRAGTOP)
            emit drag(trackIdx-1, ev->globalPos().y() - starty);
      else if (state == S_DRAGBOTTOM)
            emit drag(trackIdx, ev->globalPos().y() - starty);
      else {
            int y = pos.y();
            int wh = height();
            if (y > (wh - splitWidth))
                  setCursor(Qt::SizeVerCursor);
            else
                  setCursor(Qt::ArrowCursor);
            }
      }

//---------------------------------------------------------
//   controllerListChanged
//    controller list for controller id changed
//---------------------------------------------------------

void TLSWidget::controllerListChanged(int id)
      {
      if (ctrl()->id() == id)
            tc()->widget()->update();
      }

//---------------------------------------------------------
//   autoReadChanged
//---------------------------------------------------------

void TLSWidget::autoReadChanged()
      {
      tc()->widget()->update();
      }

//---------------------------------------------------------
//   paintEvent
//---------------------------------------------------------

void TLSWidget::paintEvent(QPaintEvent* ev)
      {
      QPainter p(this);
      QRect r(ev->rect());
      p.fillRect(r, _track->ccolor());
      paintHLine(p, r.x(), r.x() + r.width(), height() - splitWidth);
      }
