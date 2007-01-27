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

void PartListEditor::updateList()
      {
      EventList* el = part->events();
      int idx = 0;
      le.eventList->clear();
      for (iEvent i = el->begin(); i != el->end(); ++i, ++idx) {
            Event e = i->second;
            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setData(TICK_COL, Qt::TextAlignmentRole, int(Qt::AlignRight | Qt::AlignVCenter));
            item->setData(TIME_COL, Qt::TextAlignmentRole, int(Qt::AlignRight | Qt::AlignVCenter));
            item->setData(TICK_COL, Qt::DisplayRole, e.tick() + part->tick());
            item->setData(TIME_COL, Qt::DisplayRole, e.tick() + part->tick());
            item->setData(TYPE_COL, Qt::TextAlignmentRole, int(Qt::AlignRight | Qt::AlignVCenter));	    
            item->setData(TYPE_COL, Qt::DisplayRole, e.eventTypeName());
            item->setData(A_COL,  Qt::TextAlignmentRole, int(Qt::AlignHCenter | Qt::AlignVCenter));
            item->setData(A_COL, Qt::DisplayRole, e.dataA());
            item->setData(B_COL,  Qt::TextAlignmentRole, int(Qt::AlignHCenter | Qt::AlignVCenter));
            item->setData(B_COL, Qt::DisplayRole, e.dataB());
            item->setData(C_COL,  Qt::TextAlignmentRole, int(Qt::AlignHCenter | Qt::AlignVCenter));
            item->setData(C_COL, Qt::DisplayRole, e.dataC());
            item->setData(LEN_COL,  Qt::TextAlignmentRole, int(Qt::AlignHCenter | Qt::AlignVCenter));
            item->setData(LEN_COL, Qt::DisplayRole, (e.type()==Sysex?e.dataLen():e.lenTick()));
	    item->setText(DATA_COL, (e.type()==Sysex?InsertEventDialog::charArray2Str((const char*)e.data(), e.dataLen()):""));
            le.eventList->insertTopLevelItem(idx, item);
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

	InsertEventDialog dialog(time, part, this);
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
      int tick = cur->data(TICK_COL, Qt::DisplayRole).toInt();
      int evTick = (unsigned)IED_MAX(0, (int)tick - (int)part->tick());
      QString type = cur->text(TYPE_COL);
      if(type == "Note") {
	Event ev(Note);
	int pitch = cur->data(A_COL, Qt::DisplayRole).toInt();
	int velo = cur->data(B_COL, Qt::DisplayRole).toInt();
	int len = cur->data(LEN_COL, Qt::DisplayRole).toInt();
	ev.setTick(evTick);
	ev.setPitch(pitch);
	ev.setVelo(velo);
	ev.setLenTick(len);
	song->deleteEvent(ev, part);
      }
      else if(type == "Sysex") {
	Event ev(Sysex);
	QString dataStr = cur->text(DATA_COL);
	char* data = InsertEventDialog::Str2CharArray(dataStr);
	int len = cur->data(LEN_COL, Qt::DisplayRole).toInt();
	ev.setTick(evTick);
	ev.setData((const unsigned char*)data, len);
	song->deleteEvent(ev, part);
        }
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



