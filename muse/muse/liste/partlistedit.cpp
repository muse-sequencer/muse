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

#include "partlistedit.h"
#include "track.h"
#include "song.h"
#include "al/pos.h"
#include "awl/posedit.h"
#include "part.h"

//---------------------------------------------------------
//   PartListEditor
//---------------------------------------------------------

PartListEditor::PartListEditor(ListEdit* e, QWidget* parent)
   : ListWidget(parent)
      {
      listEdit = e;
      updateListDisabled = false;
      curEvent = NULL;
      QWidget* cew = new QWidget;
      le.setupUi(cew);
      QVBoxLayout* layout = new QVBoxLayout;
      layout->addWidget(cew);
      setLayout(layout);

      QFontMetrics fm(le.eventList->font());
      int zW = fm.width("0");
      le.eventList->setColumnWidth(TICK_COL, zW * 8);
      le.eventList->setColumnWidth(TIME_COL, zW * 14);
      EventDelegate* eventDelegate = new EventDelegate(this);
      le.eventList->setItemDelegate(eventDelegate);

      part = 0;

      connect(le.eventList, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
         SLOT(itemActivated(QTreeWidgetItem*,int)));
      connect(le.eventList, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
         SLOT(itemChanged(QTreeWidgetItem*, int)));
      connect(le.eventList,
	      SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
	      SLOT(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
      connect(le.insertButton, SIGNAL(clicked()), SLOT(insertClicked()));
      connect(le.deleteButton, SIGNAL(clicked()), SLOT(deleteClicked()));
     }

//---------------------------------------------------------
//   getTrack
//---------------------------------------------------------

Track* PartListEditor::getTrack() const
      {
      return part->track();
      }

//---------------------------------------------------------
//   setup
//---------------------------------------------------------

void PartListEditor::setup(const ListType& lt)
      {
      part = lt.part;
      le.partName->setText(part->name());
      updateList();
      }

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void PartListEditor::updateList() {
  if(updateListDisabled) {
    updateListDisabled = false;
    return;
  }
  EventList* el = part->events();
  int idx = 0;
  le.eventList->clear();
  for (iEvent i = el->begin(); i != el->end(); ++i, ++idx) {
    Event e = i->second;
    QTreeWidgetItem* item = new QTreeWidgetItem;
    item->setData(TICK_COL, Qt::TextAlignmentRole,
		  int(Qt::AlignRight | Qt::AlignVCenter));
    item->setData(TIME_COL, Qt::TextAlignmentRole,
		  int(Qt::AlignRight | Qt::AlignVCenter));
    item->setData(TICK_COL, Qt::DisplayRole, e.tick() + part->tick());
    item->setData(TIME_COL, Qt::DisplayRole, e.tick() + part->tick());
    item->setData(TYPE_COL, Qt::TextAlignmentRole,
		  int(Qt::AlignRight | Qt::AlignVCenter));	    
    item->setData(TYPE_COL, Qt::DisplayRole, e.eventTypeName());
    item->setData(A_COL,  Qt::TextAlignmentRole,
		  int(Qt::AlignHCenter | Qt::AlignVCenter));
    if(e.type()!=Sysex) item->setData(A_COL, Qt::DisplayRole, e.dataA());
    item->setData(B_COL,  Qt::TextAlignmentRole,
		  int(Qt::AlignHCenter | Qt::AlignVCenter));
    if(e.type()!=Sysex) item->setData(B_COL, Qt::DisplayRole, e.dataB());
    item->setData(C_COL,  Qt::TextAlignmentRole,
		  int(Qt::AlignHCenter | Qt::AlignVCenter));
    if(e.type()!=Sysex) item->setData(C_COL, Qt::DisplayRole, e.dataC());
    item->setData(LEN_COL,  Qt::TextAlignmentRole,
		  int(Qt::AlignHCenter | Qt::AlignVCenter));
    item->setData(LEN_COL, Qt::DisplayRole,
		  (e.type()==Sysex?e.dataLen():e.lenTick()));
    QString dataStr;
    if(e.type()==Sysex) 
      dataStr = InsertEventDialog::charArray2Str((const char*)e.data(),
						 e.dataLen());
    else dataStr = QString("");
    item->setText(DATA_COL, dataStr);
    le.eventList->insertTopLevelItem(idx, item);
  }
}

//---------------------------------------------------------
//   item2Event
//---------------------------------------------------------

Event* PartListEditor::item2Event(QTreeWidgetItem* item, int time_col) {
  if(item) {
    int tick;
    if(time_col == TICK_COL)
      tick = item->data(TICK_COL, Qt::DisplayRole).toInt();
    else tick = item->data(TIME_COL, Qt::DisplayRole).toInt();
    int evTick = (unsigned)IED_MAX(0, (int)tick - (int)part->tick());
    QString type = item->text(TYPE_COL);
    if(type == "Note") {
      Event* ev = new Event(Note);
      int pitch = item->data(A_COL, Qt::DisplayRole).toInt();
      int velo = item->data(B_COL, Qt::DisplayRole).toInt();
      int len = item->data(LEN_COL, Qt::DisplayRole).toInt();
      ev->setTick(evTick);
      ev->setPitch(pitch);
      ev->setVelo(velo);
      ev->setLenTick(len);
      return ev;
    }
    else if(type == "Sysex") {
      Event* ev = new Event(Sysex);
      QString dataStr = item->text(DATA_COL);
      char* data = InsertEventDialog::Str2CharArray(dataStr);
      int len = item->data(LEN_COL, Qt::DisplayRole).toInt();
      ev->setTick(evTick);
      ev->setData((const unsigned char*)data, len);
      return ev;
    }
    else return NULL;
  }
  else return NULL;
}

//---------------------------------------------------------
//   itemActivated
//---------------------------------------------------------

void PartListEditor::itemActivated(QTreeWidgetItem* item, int column) {
  AL::Pos time;
  int tick = item->data(TIME_COL, Qt::DisplayRole).toInt();
  time.setTick(tick);
  Event* ev = item2Event(item);
  EventList* el;

  if(column == TYPE_COL) {
    InsertEventDialog dialog(time, part, ev, this);
    if(dialog.exec() == QDialog::Accepted) {
      el = dialog.elResult();
      if(el) {
	if(ev) song->deleteEvent(*ev, part);
	for(iEvent ie = el->begin(); ie != el->end(); ie++) {
	  Event e = ie->second;
	  song->addEvent(e, part);
	}
      }
    }
  }
  else if((column==A_COL || column==B_COL || column==C_COL || column==LEN_COL)
	  && ev && ev->type()==Sysex) {
    //DO NOTHING
  }
  else if(column == DATA_COL) {
    if(ev && ev->type()==Sysex) {  
      InsertEventDialog dialog(time, part, ev, this);
      if(dialog.exec() == QDialog::Accepted) {
	el = dialog.elResult();
	if(el) {
	  if(ev) song->deleteEvent(*ev, part);
	  for(iEvent ie = el->begin(); ie != el->end(); ie++) {
	    Event e = ie->second;
	    song->addEvent(e, part);
	  }
	}
      }
    }
  }
  else le.eventList->openPersistentEditor(item, column);
}

//---------------------------------------------------------
//   itemChanged
//---------------------------------------------------------

void PartListEditor::itemChanged(QTreeWidgetItem* item, int column) {
  updateListDisabled = (column != TICK_COL && column != TIME_COL);
  if(item) {
    Event* newEvent = item2Event(item, column);
    song->changeEvent(*curEvent, *newEvent, part);
  }
}

//---------------------------------------------------------
//   currentItemChanged
//---------------------------------------------------------

void PartListEditor::currentItemChanged(QTreeWidgetItem* cur,
					QTreeWidgetItem* pre) {
  if(pre) {
    le.eventList->closePersistentEditor(pre, TICK_COL);
    le.eventList->closePersistentEditor(pre, TIME_COL);
    le.eventList->closePersistentEditor(pre, A_COL);
    le.eventList->closePersistentEditor(pre, B_COL);
    le.eventList->closePersistentEditor(pre, C_COL);
    le.eventList->closePersistentEditor(pre, LEN_COL); 
  }
  if(cur) {
    curEvent = item2Event(cur);
  }
}

//---------------------------------------------------------
//   insertClicked
//    insert one tick before current value
//---------------------------------------------------------

void PartListEditor::insertClicked()
      {
	QTreeWidgetItem* cur = le.eventList->currentItem();
	AL::Pos time;
	if(cur) {
	  int tick = cur->data(TIME_COL, Qt::DisplayRole).toInt();
	  time.setTick(tick);
	}
	  
	EventList* el;

	InsertEventDialog dialog(time, part, NULL, this);
	if(dialog.exec() == QDialog::Accepted) {
	  el = dialog.elResult();
	  if(el) {
	    for(iEvent ie = el->begin(); ie != el->end(); ie++) {
	      Event e = ie->second;
	      song->addEvent(e, part);
	    }
	  }
	}
      }

//---------------------------------------------------------
//   deleteClicked
//---------------------------------------------------------

void PartListEditor::deleteClicked()
      {
      QTreeWidgetItem* cur = le.eventList->currentItem();
      if (cur == 0)
	return;
      Event* ev = item2Event(cur);
      song->deleteEvent(*ev, part);
      }

//---------------------------------------------------------
//   EventDelegate
//---------------------------------------------------------

EventDelegate::EventDelegate(QObject* parent)
   : QItemDelegate(parent)
      {
      }

//---------------------------------------------------------
//   createEditor
//---------------------------------------------------------

QWidget* EventDelegate::createEditor(QWidget* pw,
   const QStyleOptionViewItem& option, const QModelIndex& index) const
      {
      switch(index.column()) {
            case PartListEditor::TICK_COL:
                  break;
            case PartListEditor::TIME_COL:
                  return new Awl::PosEdit(pw);
            case PartListEditor::TYPE_COL:
            case PartListEditor::A_COL:
            case PartListEditor::B_COL:
            case PartListEditor::C_COL:
            case PartListEditor::LEN_COL:
            case PartListEditor::DATA_COL:
                  break;
            }
      return QItemDelegate::createEditor(pw, option, index);
      }

//---------------------------------------------------------
//   setEditorData
//---------------------------------------------------------

void EventDelegate::setEditorData(QWidget* editor,
   const QModelIndex& index) const
      {
      switch(index.column()) {
            case PartListEditor::TICK_COL:
                  break;
            case PartListEditor::TIME_COL:
                  {
                  Awl::PosEdit* pe = static_cast<Awl::PosEdit*>(editor);
                  pe->setValue(AL::Pos(index.data().toInt()));
                  }
                  return;
            case PartListEditor::TYPE_COL:
            case PartListEditor::A_COL:
            case PartListEditor::B_COL:
            case PartListEditor::C_COL:
            case PartListEditor::LEN_COL:
            case PartListEditor::DATA_COL:
                  break;
            }
      QItemDelegate::setEditorData(editor, index);
      }

//---------------------------------------------------------
//   setModelData
//---------------------------------------------------------

void EventDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
         const QModelIndex& index) const
      {
      switch(index.column()) {
            case PartListEditor::TICK_COL:
                  break;
            case PartListEditor::TIME_COL:
                  {
                  Awl::PosEdit* pe = static_cast<Awl::PosEdit*>(editor);
                  model->setData(index, pe->pos().tick(), Qt::DisplayRole);
                  }
                  return;
            case PartListEditor::TYPE_COL:
            case PartListEditor::A_COL:
            case PartListEditor::B_COL:
            case PartListEditor::C_COL:
            case PartListEditor::LEN_COL:
            case PartListEditor::DATA_COL:
                  break;
            }
      QItemDelegate::setModelData(editor, model, index);
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void EventDelegate::paint(QPainter* painter,
   const QStyleOptionViewItem& option, const QModelIndex& index) const
      {
      QString text;
      PartListEditor* ce = static_cast<PartListEditor*>(parent());

      switch(index.column()) {
            case PartListEditor::TICK_COL:
                  {
                  Track* track = ce->getTrack();
                  AL::Pos pos(index.data().toInt(), track->timeType());
                  text = QString("%1").arg(pos.tick());
                  }
                  break;
            case PartListEditor::TIME_COL:
                  {
                  Track* track = ce->getTrack();
                  AL::Pos pos(index.data().toInt(), track->timeType());
                  int measure, beat, tick;
                  pos.mbt(&measure, &beat, &tick);
                  text.sprintf("%04d.%02d.%03u", measure+1, beat+1, tick);
                  }
                  break;
            case PartListEditor::TYPE_COL:
            case PartListEditor::A_COL:
            case PartListEditor::B_COL:
            case PartListEditor::C_COL:
            case PartListEditor::LEN_COL:
            case PartListEditor::DATA_COL:
                  QItemDelegate::paint(painter, option, index);
                  return;
            }

      QStyleOptionViewItemV2 opt = setOptions(index, option);
      const QStyleOptionViewItemV2 *v2 = qstyleoption_cast<const QStyleOptionViewItemV2 *>(&option);
      opt.features = v2 ? v2->features : QStyleOptionViewItemV2::ViewItemFeatures(QStyleOptionViewItemV2::None);

      painter->save();
      QVariant value;
      QRect displayRect;
      displayRect = option.rect; // textRectangle(painter, d->textLayoutBounds(opt), opt.font, text);

      QRect checkRect;
      Qt::CheckState checkState = Qt::Unchecked;
      value = index.data(Qt::CheckStateRole);
      if (value.isValid()) {
            checkState = static_cast<Qt::CheckState>(value.toInt());
            checkRect = check(opt, opt.rect, value);
            }

      drawBackground(painter, opt, index);
      drawCheck(painter, opt, checkRect, checkState);
      drawDisplay(painter, opt, displayRect, text);
      drawFocus(painter, opt, text.isEmpty() ? QRect() : displayRect);
      painter->restore();
      }



