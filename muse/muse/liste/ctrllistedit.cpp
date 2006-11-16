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

#include "ctrllistedit.h"
#include "ctrl.h"
#include "track.h"
#include "song.h"
#include "al/pos.h"
#include "awl/posedit.h"

//---------------------------------------------------------
//   CtrlListEditor
//---------------------------------------------------------

CtrlListEditor::CtrlListEditor(ListEdit* e, QWidget* parent)
   : ListWidget(parent)
      {
      listEdit = e;
      updateListDisabled = false;
      QWidget* cew = new QWidget;
      le.setupUi(cew);
      QVBoxLayout* layout = new QVBoxLayout;
      layout->addWidget(cew);
      setLayout(layout);
      le.minValue->setRange(-10000000.0, 100000000.0);
      le.maxValue->setRange(-10000000.0, 100000000.0);
      le.defaultValue->setRange(-10000000.0, 100000000.0);
      le.minValue->setSingleStep(1.0);
      le.maxValue->setSingleStep(1.0);
      le.defaultValue->setSingleStep(1.0);

      QFontMetrics fm(le.ctrlList->font());
      int zW = fm.width("0");
      le.ctrlList->setColumnWidth(TICK_COL, zW * 8);
      le.ctrlList->setColumnWidth(TIME_COL, zW * 14);
      MidiTimeDelegate* midiTimeDelegate = new MidiTimeDelegate(this);
      le.ctrlList->setItemDelegate(midiTimeDelegate);

      track = 0;
      connect(le.ctrlList, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
         SLOT(itemActivated(QTreeWidgetItem*,int)));
      connect(le.ctrlList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),
         SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));
      connect(le.ctrlList, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
         SLOT(itemChanged(QTreeWidgetItem*, int)));
      connect(le.ctrlList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
         SLOT(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
      connect(le.insertButton, SIGNAL(clicked()), SLOT(insertClicked()));
      connect(le.deleteButton, SIGNAL(clicked()), SLOT(deleteClicked()));
      connect(le.controllerName, SIGNAL(textEdited(const QString&)),
         SLOT(nameEdited(const QString&)));
      connect(le.minValue, SIGNAL(valueChanged(double)), SLOT(minValChanged(double)));
      connect(le.maxValue, SIGNAL(valueChanged(double)), SLOT(maxValChanged(double)));
      connect(le.defaultValue, SIGNAL(valueChanged(double)), SLOT(defaultValChanged(double)));
      EscapeFilter* ef = new EscapeFilter(this);
      installEventFilter(ef);
      }

//---------------------------------------------------------
//   eventFilter
//---------------------------------------------------------

bool EscapeFilter::eventFilter(QObject* obj, QEvent* event)
      {
      if (event->type() == QEvent::KeyPress) {
            if (((QKeyEvent*)event)->key() == Qt::Key_Escape) {
                  ((CtrlListEditor*)parent())->sendEscape();
                  return true;       
                  }     
            }
      return QObject::eventFilter(obj, event);      
      }

//---------------------------------------------------------
//   setup
//---------------------------------------------------------

void CtrlListEditor::setup(const ListType& lt)
      {
      if (track)
            disconnect(track, SIGNAL(controllerChanged(int)), this, SLOT(controllerChanged(int)));
      track = lt.track;
      connect(track, SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
      
      c = lt.ctrl;
      le.controllerName->setText(c->name());
      le.discreteCheckBox->setChecked(c->type() & Ctrl::DISCRETE);
      le.logarithmicCheckBox->setChecked(c->type() & Ctrl::LOG);
      le.floatCheckBox->setChecked(!(c->type() & Ctrl::INT));
      le.ctrlId->setValue(c->id());
      if (c->type() & Ctrl::INT) {
            le.minValue->setDecimals(0);
            le.minValue->setValue(c->minVal().i);
            le.maxValue->setDecimals(0);
            le.maxValue->setValue(c->maxVal().i);
            le.defaultValue->setDecimals(0);
            le.defaultValue->setValue(c->getDefault().i);
            }
      else {
            le.minValue->setDecimals(1);
            le.minValue->setValue(c->minVal().f);
            le.maxValue->setDecimals(1);
            le.maxValue->setValue(c->maxVal().f);
            le.defaultValue->setDecimals(1);
            le.defaultValue->setValue(c->getDefault().f);
            }
      updateList();
      }

//---------------------------------------------------------
//   updateList
//---------------------------------------------------------

void CtrlListEditor::updateList()
      {
      if (updateListDisabled)
            return;
      le.ctrlList->clear();
      int idx = 0;
      bool curItemSet = false;
      for (iCtrlVal i = c->begin(); i != c->end(); ++i, ++idx) {
            CVal v = i.value();
            QTreeWidgetItem* item = new QTreeWidgetItem;
            item->setData(TICK_COL, Qt::TextAlignmentRole, int(Qt::AlignRight | Qt::AlignVCenter));
            item->setData(TIME_COL, Qt::TextAlignmentRole, int(Qt::AlignRight | Qt::AlignVCenter));
            item->setData(VAL_COL,  Qt::TextAlignmentRole, int(Qt::AlignHCenter | Qt::AlignVCenter));

            item->setData(TICK_COL, Qt::DisplayRole, i.key());
            item->setData(TIME_COL, Qt::DisplayRole, i.key());
            if (c->type() & Ctrl::INT)
                  item->setData(VAL_COL, Qt::DisplayRole, v.i);
            else
                  item->setData(VAL_COL, Qt::DisplayRole, v.f);
            le.ctrlList->insertTopLevelItem(idx, item);
            if (!curItemSet && (i.key() >= listEdit->pos().tick())) {
                  le.ctrlList->setCurrentItem(item);
                  curItemSet = true;
                  }
            }
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void CtrlListEditor::controllerChanged(int id)
      {
      if (id != c->id())
            return;
      updateList();
      }

//---------------------------------------------------------
//   itemActivated
//---------------------------------------------------------

void CtrlListEditor::itemActivated(QTreeWidgetItem* item, int column)
      {
      le.ctrlList->openPersistentEditor(item, column);
      }

//---------------------------------------------------------
//   itemChanged
//---------------------------------------------------------

void CtrlListEditor::itemChanged(QTreeWidgetItem* item, int column)
      {
      CVal val;
      if (c->type() & Ctrl::INT) {
            val.i = item->data(VAL_COL, Qt::DisplayRole).toInt();
            bool updateData = false;
            if (val.i < c->minVal().i) {
                  val.i = c->minVal().i;
                  updateData = true;
                  }
            else if (val.i > c->maxVal().i) {
                  val.i = c->maxVal().i;
                  updateData = true;
                  }
            if (updateData)
                  item->setData(VAL_COL, Qt::DisplayRole, val.i);
            }
      else {
            val.f = item->data(VAL_COL, Qt::DisplayRole).toDouble();
            bool updateData = false;
            if (val.f < c->minVal().f) {
                  val.f = c->minVal().f;
                  updateData = true;
                  }
            else if (val.f > c->maxVal().f) {
                  val.f = c->maxVal().f;
                  updateData = true;
                  }
            if (updateData)
                  item->setData(VAL_COL, Qt::DisplayRole, val.f);
            }
      le.ctrlList->closePersistentEditor(item, TICK_COL);
      le.ctrlList->closePersistentEditor(item, TIME_COL);
      le.ctrlList->closePersistentEditor(item, VAL_COL);
      updateListDisabled = true;
      switch(column) {
            case TICK_COL:
                  {
                  int otick = item->data(TIME_COL, Qt::DisplayRole).toInt();
                  int tick = item->data(TICK_COL, Qt::DisplayRole).toInt();
                  item->setData(TIME_COL, Qt::DisplayRole, tick);
                  song->removeControllerVal(track, c->id(), otick);
                  song->addControllerVal(track, c, tick, val);
                  }
                  break;
            case TIME_COL:
                  {
                  int otick = item->data(TICK_COL, Qt::DisplayRole).toInt();
                  int tick = item->data(TIME_COL, Qt::DisplayRole).toInt();
                  item->setData(TICK_COL, Qt::DisplayRole, tick);
                  song->removeControllerVal(track, c->id(), otick);
                  song->addControllerVal(track, c, tick, val);
                  }
                  break;
            case VAL_COL:
                  song->addControllerVal(track, c, listEdit->pos(), val);
                  break;
            }
      updateListDisabled = false;
      }

//---------------------------------------------------------
//   sendEscape
//---------------------------------------------------------

void CtrlListEditor::sendEscape()
      {
      QTreeWidgetItem* cur = le.ctrlList->currentItem();
      if (cur == 0)
            return;
      le.ctrlList->closePersistentEditor(cur, TICK_COL);
      le.ctrlList->closePersistentEditor(cur, TIME_COL);
      le.ctrlList->closePersistentEditor(cur, VAL_COL);
      }

//---------------------------------------------------------
//   currentItemChanged
//---------------------------------------------------------

void CtrlListEditor::currentItemChanged(QTreeWidgetItem* cur, QTreeWidgetItem* prev)
      {
      if (prev) {
            le.ctrlList->closePersistentEditor(prev, TICK_COL);
            le.ctrlList->closePersistentEditor(prev, TIME_COL);
            le.ctrlList->closePersistentEditor(prev, VAL_COL);
            }
      if (cur)
            listEdit->pos().setTick(cur->data(TICK_COL, Qt::DisplayRole).toInt());
      le.deleteButton->setEnabled(cur);
      }

//---------------------------------------------------------
//   insertClicked
//    insert one tick before current value
//---------------------------------------------------------

void CtrlListEditor::insertClicked()
      {
      CVal val = c->minVal();
      QTreeWidgetItem* cur = le.ctrlList->currentItem();
      if (cur) {
            int tick = cur->data(TICK_COL, Qt::DisplayRole).toInt();
            if (tick == 0)    // cannot insert value at position < 0
                  return;
            listEdit->pos().setTick(tick - 1);
            if (c->type() & Ctrl::INT)
                  val.i = cur->data(VAL_COL, Qt::DisplayRole).toInt();
            else
                  val.f = cur->data(VAL_COL, Qt::DisplayRole).toDouble();
            }            
      song->addControllerVal(track, c, listEdit->pos(), val);
      }

//---------------------------------------------------------
//   deleteClicked
//---------------------------------------------------------

void CtrlListEditor::deleteClicked()
      {
      QTreeWidgetItem* cur = le.ctrlList->currentItem();
      if (cur == 0)
            return;
      int tick = cur->data(TICK_COL, Qt::DisplayRole).toInt();
      song->removeControllerVal(track, c->id(), tick);
      }

//---------------------------------------------------------
//   nameEdited
//---------------------------------------------------------

void CtrlListEditor::nameEdited(const QString& s)
      {
      track->changeCtrlName(c, s);
      }

//---------------------------------------------------------
//   minValChanged
//---------------------------------------------------------

void CtrlListEditor::minValChanged(double v)
      {
      CVal val;
      if (c->type() & Ctrl::INT)
            val.i = int(v);
      else
            val.f = v;
      c->setRange(val, c->maxVal());
      }

//---------------------------------------------------------
//   maxValChanged
//---------------------------------------------------------

void CtrlListEditor::maxValChanged(double v)
      {
      CVal val;
      if (c->type() & Ctrl::INT)
            val.i = int(v);
      else
            val.f = v;
      c->setRange(c->minVal(), val);
      }

//---------------------------------------------------------
//   defaultValChanged
//---------------------------------------------------------

void CtrlListEditor::defaultValChanged(double v)
      {
      CVal val;
      if (c->type() & Ctrl::INT)
            val.i = int(v);
      else
            val.f = v;
      c->setDefault(val);
      }

//---------------------------------------------------------
//   MidiTimeDelegate
//---------------------------------------------------------

MidiTimeDelegate::MidiTimeDelegate(QObject* parent)
   : QItemDelegate(parent)
      {
      }
      
//---------------------------------------------------------
//   createEditor
//---------------------------------------------------------

QWidget* MidiTimeDelegate::createEditor(QWidget* pw,
   const QStyleOptionViewItem& option, const QModelIndex& index) const
      {
      switch(index.column()) {
            case CtrlListEditor::TICK_COL:
                  break;
            case CtrlListEditor::TIME_COL:
                  return new Awl::PosEdit(pw);
            case CtrlListEditor::VAL_COL:
                  {
                  CtrlListEditor* ce = static_cast<CtrlListEditor*>(parent());
                  Ctrl* c = ce->ctrl();
                  if (c->type() & Ctrl::INT) {
                        QSpinBox* w = new QSpinBox(pw);
                        w->setRange(c->minVal().i, c->maxVal().i);
                        w->installEventFilter(const_cast<MidiTimeDelegate*>(this));
                        return w;
                        }
                  QDoubleSpinBox* w = new QDoubleSpinBox(pw);
                  w->setRange(c->minVal().f, c->maxVal().f);
                  w->installEventFilter(const_cast<MidiTimeDelegate*>(this));
                  return w;
                  }
            }
      return QItemDelegate::createEditor(pw, option, index);
      }

//---------------------------------------------------------
//   setEditorData
//---------------------------------------------------------

void MidiTimeDelegate::setEditorData(QWidget* editor, 
   const QModelIndex& index) const
      {
      switch(index.column()) {
            case CtrlListEditor::TICK_COL:
                  break;
            case CtrlListEditor::TIME_COL:
                  {
                  Awl::PosEdit* pe = static_cast<Awl::PosEdit*>(editor);
                  pe->setValue(AL::Pos(index.data().toInt()));
                  }
                  return;
            case CtrlListEditor::VAL_COL:
                  {
                  CtrlListEditor* ce = static_cast<CtrlListEditor*>(parent());
                  Ctrl* c = ce->ctrl();
                  if (c->type() & Ctrl::INT) {
                        QSpinBox* w = static_cast<QSpinBox*>(editor);
                        w->setValue(index.data().toInt());
                        }
                  else {
                        QDoubleSpinBox* w = static_cast<QDoubleSpinBox*>(editor);
                        w->setValue(index.data().toDouble());
                        }
                  }
                  return;
            }
      QItemDelegate::setEditorData(editor, index);
      }

//---------------------------------------------------------
//   setModelData
//---------------------------------------------------------

void MidiTimeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
         const QModelIndex& index) const
      {
      switch(index.column()) {
            case CtrlListEditor::TICK_COL:
                  break;
            case CtrlListEditor::TIME_COL:
                  {
                  Awl::PosEdit* pe = static_cast<Awl::PosEdit*>(editor);
                  model->setData(index, pe->pos().tick(), Qt::DisplayRole);
                  }
                  return;
            case CtrlListEditor::VAL_COL:
                  {
                  CtrlListEditor* ce = static_cast<CtrlListEditor*>(parent());
                  Ctrl* c = ce->ctrl();
                  if (c->type() & Ctrl::INT) {
                        QSpinBox* w = static_cast<QSpinBox*>(editor);
                        model->setData(index, w->value(), Qt::DisplayRole);
                        }
                  else {
                        QDoubleSpinBox* w = static_cast<QDoubleSpinBox*>(editor);
                        model->setData(index, w->value(), Qt::DisplayRole);
                        }
                  }
                  break;
            }
      QItemDelegate::setModelData(editor, model, index);
      }

//---------------------------------------------------------
//   paint
//---------------------------------------------------------

void MidiTimeDelegate::paint(QPainter* painter, 
   const QStyleOptionViewItem& option, const QModelIndex& index) const
      {
      if (index.column() != CtrlListEditor::TIME_COL) {
            QItemDelegate::paint(painter, option, index);
            return;
            }
      AL::Pos pos(index.data().toInt());
      int measure, beat, tick;
      pos.mbt(&measure, &beat, &tick);
      QString text;
      text.sprintf("%04d.%02d.%03u", measure+1, beat+1, tick);

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

//---------------------------------------------------------
//   itemDoubleClicked
//---------------------------------------------------------

void CtrlListEditor::itemDoubleClicked(QTreeWidgetItem* item, int column)
      {
      printf("double clicked\n");
      }


