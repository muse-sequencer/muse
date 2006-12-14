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
            item->setData(TICK_COL, Qt::DisplayRole, e.tick());
            item->setData(TIME_COL, Qt::DisplayRole, e.tick());

            item->setData(2, Qt::DisplayRole, e.eventTypeName());
            item->setData(3, Qt::DisplayRole, e.dataA());
            item->setData(4, Qt::DisplayRole, e.dataB());
            le.eventList->insertTopLevelItem(idx, item);
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



