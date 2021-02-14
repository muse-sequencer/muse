//=========================================================
//  MusE
//  Linux Music Editor
//
//  control_mapper.cpp
//  Copyright (C) 2012 by Tim E. Real (terminator356 at users.sourceforge.net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#include <QTimer>
#include <QItemEditorCreatorBase>
#include <QStandardItemEditorCreator>
#include <QItemEditorFactory>
#include <QStringList>
#include <QPainter>
#include <QSize>
#include <QMouseEvent>
#include <QColor>
#include <QBrush>
#include <QPointF>
#include <QMouseEvent>
#include <QTableView>
#include <QColorDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QHBoxLayout>

#include <stdio.h>
#include <stdlib.h>

#include "globals.h"
#include "globaldefs.h"
#include "mididev.h"
#include "midiport.h"
#include "midictrl.h"
#include "audio.h"
#include "synth.h"
#include "minstrument.h"
#include "app.h"
#include "icons.h"

#include "control_mapper.h"

namespace MusEGui {

//typedef class ColorListEditor ColorEditor;
typedef class ColorChooserEditor ColorEditor;
  
// Fifteen colors, 16 elements, the last element opens a full QColorDialog.
#define __COLOR_CHOOSER_NUM_ELEMENTS__     16
#define __COLOR_CHOOSER_NUM_COLUMNS__       4
#define __COLOR_CHOOSER_ELEMENT_WIDTH__    16
#define __COLOR_CHOOSER_ELEMENT_HEIGHT__   16
#define __COLOR_CHOOSER_BORDER_WIDTH__      1
QColor ColorChooserEditor::colorChooserList[] = {
  Qt::red,
  Qt::yellow,
  Qt::green,
  Qt::cyan,
  Qt::blue,
  Qt::magenta,

  Qt::darkRed,
  Qt::darkYellow,
  Qt::darkGreen,
  Qt::darkCyan,
  Qt::darkBlue,
  Qt::darkMagenta,
  
  Qt::darkGray,
  Qt::white,
  Qt::black
};

//-----------------------------------
//   ColorChooserEditor
//-----------------------------------

ColorChooserEditor::ColorChooserEditor(QWidget *parent) : QWidget(parent)
{
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setFixedWidth(__COLOR_CHOOSER_ELEMENT_WIDTH__ * __COLOR_CHOOSER_NUM_COLUMNS__ +
                __COLOR_CHOOSER_BORDER_WIDTH__ * 2);
  setFixedHeight(__COLOR_CHOOSER_ELEMENT_HEIGHT__ * (__COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__) +
                __COLOR_CHOOSER_BORDER_WIDTH__ * 2);
  //setMouseTracking(true);
  setAutoFillBackground(true);
  setFocusPolicy(Qt::StrongFocus);  // Required or else it closes on click.
}

ColorChooserEditor::~ColorChooserEditor()
{
  fprintf(stderr, "~ColorChooserEditor\n");  // REMOVE Tim.
}

QSize ColorChooserEditor::sizeHint() const
{
  return minimumSizeHint();
}

QSize ColorChooserEditor::minimumSizeHint() const
{
  return QSize(__COLOR_CHOOSER_ELEMENT_WIDTH__ * __COLOR_CHOOSER_NUM_COLUMNS__ +
               __COLOR_CHOOSER_BORDER_WIDTH__ * 2,
               __COLOR_CHOOSER_ELEMENT_HEIGHT__ * (__COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__) +
               __COLOR_CHOOSER_BORDER_WIDTH__ * 2);
}

QColor ColorChooserEditor::color() const
{
  //return qvariant_cast<QColor>(itemData(currentIndex(), Qt::DecorationRole));
  return _color;
}

void ColorChooserEditor::setColor(QColor color)
{
  //setCurrentIndex(findData(color, int(Qt::DecorationRole)));
  _color = color;
}

void ColorChooserEditor::paintEvent(QPaintEvent* /*event*/)
{
  QPainter p(this);

  for(int i = 0; i < __COLOR_CHOOSER_NUM_ELEMENTS__; ++i)
  {
    int px = (i % __COLOR_CHOOSER_NUM_COLUMNS__) * __COLOR_CHOOSER_ELEMENT_WIDTH__;
    int py = (i / __COLOR_CHOOSER_NUM_COLUMNS__) * __COLOR_CHOOSER_ELEMENT_HEIGHT__;

    // The last element is the QColorDialog opener.
    if(i == __COLOR_CHOOSER_NUM_ELEMENTS__ - 1)
      p.drawPixmap(px + __COLOR_CHOOSER_BORDER_WIDTH__, py + __COLOR_CHOOSER_BORDER_WIDTH__, *settings_appearance_settingsIcon);  // This icon is 16x16
    else
    {
      p.fillRect(px + __COLOR_CHOOSER_BORDER_WIDTH__ + 1,
                 py + __COLOR_CHOOSER_BORDER_WIDTH__ + 1,
                 __COLOR_CHOOSER_ELEMENT_WIDTH__ - 2,
                 __COLOR_CHOOSER_ELEMENT_HEIGHT__ - 2,
                 colorChooserList[i]);
      if(_color == colorChooserList[i])
      {
        QPen pen(Qt::DashLine);
        p.setPen(pen);
        p.drawRect(px + __COLOR_CHOOSER_BORDER_WIDTH__,
                   py + __COLOR_CHOOSER_BORDER_WIDTH__,
                   __COLOR_CHOOSER_ELEMENT_WIDTH__ - 1,
                   __COLOR_CHOOSER_ELEMENT_HEIGHT__ - 1);
      }
    }
  }

  // Draw a one-pixel border.
  QPen pen(Qt::black);
  pen.setWidth(__COLOR_CHOOSER_BORDER_WIDTH__);
  p.setPen(pen);
  p.drawRect(0, 0, width() - 1, height() - 1);
}

void ColorChooserEditor::mousePressEvent(QMouseEvent* event)
{
  fprintf(stderr, "ColorChooserEditor::mousePressEvent\n");  // REMOVE Tim.
  event->accept();
}

void ColorChooserEditor::mouseReleaseEvent(QMouseEvent* event)
{
  fprintf(stderr, "ColorChooserEditor::mouseReleaseEvent\n");  // REMOVE Tim.

  int x = event->x();
  int y = event->y();

  int px = x - __COLOR_CHOOSER_BORDER_WIDTH__;
  int py = y - __COLOR_CHOOSER_BORDER_WIDTH__;
  if(px < 0)
    px = 0;
  if(py < 0)
    py = 0;
  
  int column = px / __COLOR_CHOOSER_ELEMENT_WIDTH__;
  if(column >= __COLOR_CHOOSER_NUM_COLUMNS__)
    column = __COLOR_CHOOSER_NUM_COLUMNS__ - 1;
  int row    = py / __COLOR_CHOOSER_ELEMENT_HEIGHT__;
  if(row >= __COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__)
    row = __COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__ - 1;

  int element = row * __COLOR_CHOOSER_NUM_COLUMNS__ + column;

  if(element >= __COLOR_CHOOSER_NUM_ELEMENTS__)  // Just in case
    element = __COLOR_CHOOSER_NUM_ELEMENTS__;

  // The last element is the QColorDialog opener.
  if(element == __COLOR_CHOOSER_NUM_ELEMENTS__ - 1)
  {
    QColor c = QColorDialog::getColor(_color, this, QString());
    if(!c.isValid())
      return;
    _color = c;
  }
  else
    _color = colorChooserList[element];

  emit activated(_color);
  
  event->accept();
}

// bool ColorChooserEditor::event(QEvent* event)
// {
//   if(event->type() == QEvent::MouseButtonPress)
//   {
//     QMouseEvent* me = static_cast<QMouseEvent*>(event);
//     fprintf(stderr, "ColorChooserEditor::event: Press X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
//     event->accept();
//     return true;
//   }
//   else
//   if(event->type() == QEvent::MouseButtonRelease)
//   {
//     QMouseEvent* me = static_cast<QMouseEvent*>(event);
//     fprintf(stderr, "ColorChooserEditor::event: Release X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
//     //event->accept();
//     //return true;
//   }
//   else
//   if(event->type() == QEvent::Close)
//   {
//     fprintf(stderr, "ColorChooserEditor::event: Close\n");  // REMOVE Tim.
//   }
//   else
//     fprintf(stderr, "ColorChooserEditor::event: type:%d\n", event->type());  // REMOVE Tim.
//   
//   return QWidget::event(event);
// }


//-----------------------------------
//   MidiCtrlDelegateEditor
//-----------------------------------

MidiCtrlDelegateEditor::MidiCtrlDelegateEditor(QWidget *parent) : QWidget(parent)
{
///   setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
///   setFixedWidth(__COLOR_CHOOSER_ELEMENT_WIDTH__ * __COLOR_CHOOSER_NUM_COLUMNS__ +
///                 __COLOR_CHOOSER_BORDER_WIDTH__ * 2);
///   setFixedHeight(__COLOR_CHOOSER_ELEMENT_HEIGHT__ * (__COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__) +
///                 __COLOR_CHOOSER_BORDER_WIDTH__ * 2);
  //setMouseTracking(true);
  setAutoFillBackground(true);
  setFocusPolicy(Qt::StrongFocus);  // Required or else it closes on click.

  _hlayout = new QHBoxLayout(this);
  _typeCombo = new QComboBox(this);
  _numHiSpinBox = new QSpinBox(this);
  _numLoSpinBox = new QSpinBox(this);
  _validIndicator = new QWidget(this);

  _numHiSpinBox->setMinimum(0);
  _numLoSpinBox->setMinimum(0);
  _numHiSpinBox->setMaximum(127);
  _numLoSpinBox->setMaximum(127);

  _validIndicator->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  _validIndicator->setFixedWidth(16);
  _validIndicator->setFixedHeight(16);
  
  _typeCombo->addItem("---", -1);
  _typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller7), MusECore::MidiController::Controller7);
  _typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller14), MusECore::MidiController::Controller14);
  _typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN), MusECore::MidiController::RPN);
  _typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN), MusECore::MidiController::NRPN);
  _typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN14), MusECore::MidiController::RPN14);
  _typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN14), MusECore::MidiController::NRPN14);
  _typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Pitch), MusECore::MidiController::Pitch);
  _typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Program), MusECore::MidiController::Program);
  // TODO Per-pitch controls not supported yet. Need a way to select pitch.
  //_typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::PolyAftertouch), MusECore::MidiController::PolyAftertouch);
  _typeCombo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Aftertouch), MusECore::MidiController::Aftertouch);
  //connect(_typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(editorChanged()));

  _hlayout->addWidget(_typeCombo);
  _hlayout->addWidget(_numHiSpinBox);
  _hlayout->addWidget(_numLoSpinBox);
  _hlayout->addWidget(_validIndicator);
}

MidiCtrlDelegateEditor::~MidiCtrlDelegateEditor()
{
  fprintf(stderr, "~MidiCtrlDelegateEditor\n");  // REMOVE Tim.
}

QSize MidiCtrlDelegateEditor::sizeHint() const
{
  //return minimumSizeHint();
  return QWidget::sizeHint();
}

QSize MidiCtrlDelegateEditor::minimumSizeHint() const
{
//   return QSize(__COLOR_CHOOSER_ELEMENT_WIDTH__ * __COLOR_CHOOSER_NUM_COLUMNS__ +
//                __COLOR_CHOOSER_BORDER_WIDTH__ * 2,
//                __COLOR_CHOOSER_ELEMENT_HEIGHT__ * (__COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__) +
//                __COLOR_CHOOSER_BORDER_WIDTH__ * 2);
  return QWidget::minimumSizeHint();
}

int MidiCtrlDelegateEditor::controlNum() const
{
  int idx = _typeCombo->currentIndex();
  if(idx != -1)
    return MusECore::midiCtrlTerms2Number(qvariant_cast<MusECore::MidiController::ControllerType>(_typeCombo->itemData(idx)),
                                          ((_numHiSpinBox->value() & 0x7f) << 8) | (_numLoSpinBox->value() & 0x7f));
  return -1;
}

void MidiCtrlDelegateEditor::setControlNum(int ctrlNum)
{
  int type = -1;
  int hnum = 0;
  int lnum = 0;
  if(ctrlNum != -1)
  {
    type = MusECore::midiControllerType(ctrlNum);
    hnum = (ctrlNum >> 8) & 0x7f;
    lnum = ctrlNum & 0x7f;
  }

  int idx = _typeCombo->findData(type);
  if(idx != -1)
  {
    _typeCombo->blockSignals(true);     // Prevent currentIndexChanged or activated from being called
    _typeCombo->setCurrentIndex(idx);
    _typeCombo->blockSignals(false);
  }

  _numHiSpinBox->blockSignals(true);
  _numHiSpinBox->setValue(hnum);
  _numHiSpinBox->blockSignals(false);
  _numLoSpinBox->blockSignals(true);
  _numLoSpinBox->setValue(lnum);
  _numLoSpinBox->blockSignals(false);
}

void MidiCtrlDelegateEditor::paintEvent(QPaintEvent* event)
{
  // TODO if required

  QWidget::paintEvent(event);

//   QPainter p(this);
// 
//   for(int i = 0; i < __COLOR_CHOOSER_NUM_ELEMENTS__; ++i)
//   {
//     int px = (i % __COLOR_CHOOSER_NUM_COLUMNS__) * __COLOR_CHOOSER_ELEMENT_WIDTH__;
//     int py = (i / __COLOR_CHOOSER_NUM_COLUMNS__) * __COLOR_CHOOSER_ELEMENT_HEIGHT__;
// 
//     // The last element is the QColorDialog opener.
//     if(i == __COLOR_CHOOSER_NUM_ELEMENTS__ - 1)
//       p.drawPixmap(px + __COLOR_CHOOSER_BORDER_WIDTH__, py + __COLOR_CHOOSER_BORDER_WIDTH__, *settings_appearance_settingsIcon);  // This icon is 16x16
//     else
//     {
//       p.fillRect(px + __COLOR_CHOOSER_BORDER_WIDTH__ + 1,
//                  py + __COLOR_CHOOSER_BORDER_WIDTH__ + 1,
//                  __COLOR_CHOOSER_ELEMENT_WIDTH__ - 2,
//                  __COLOR_CHOOSER_ELEMENT_HEIGHT__ - 2,
//                  colorChooserList[i]);
//       if(_color == colorChooserList[i])
//       {
//         QPen pen(Qt::DashLine);
//         p.setPen(pen);
//         p.drawRect(px + __COLOR_CHOOSER_BORDER_WIDTH__,
//                    py + __COLOR_CHOOSER_BORDER_WIDTH__,
//                    __COLOR_CHOOSER_ELEMENT_WIDTH__ - 1,
//                    __COLOR_CHOOSER_ELEMENT_HEIGHT__ - 1);
//       }
//     }
//   }
// 
//   // Draw a one-pixel border.
//   QPen pen(Qt::black);
//   pen.setWidth(__COLOR_CHOOSER_BORDER_WIDTH__);
//   p.setPen(pen);
//   p.drawRect(0, 0, width() - 1, height() - 1);
}

void MidiCtrlDelegateEditor::mousePressEvent(QMouseEvent* event)
{
  fprintf(stderr, "MidiCtrlDelegateEditor::mousePressEvent\n");  // REMOVE Tim.
  //event->accept();
}

void MidiCtrlDelegateEditor::mouseReleaseEvent(QMouseEvent* event)
{
  fprintf(stderr, "MidiCtrlDelegateEditor::mouseReleaseEvent\n");  // REMOVE Tim.

//   int x = event->x();
//   int y = event->y();
// 
//   int px = x - __COLOR_CHOOSER_BORDER_WIDTH__;
//   int py = y - __COLOR_CHOOSER_BORDER_WIDTH__;
//   if(px < 0)
//     px = 0;
//   if(py < 0)
//     py = 0;
// 
//   int column = px / __COLOR_CHOOSER_ELEMENT_WIDTH__;
//   if(column >= __COLOR_CHOOSER_NUM_COLUMNS__)
//     column = __COLOR_CHOOSER_NUM_COLUMNS__ - 1;
//   int row    = py / __COLOR_CHOOSER_ELEMENT_HEIGHT__;
//   if(row >= __COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__)
//     row = __COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__ - 1;
// 
//   int element = row * __COLOR_CHOOSER_NUM_COLUMNS__ + column;
// 
//   if(element >= __COLOR_CHOOSER_NUM_ELEMENTS__)  // Just in case
//     element = __COLOR_CHOOSER_NUM_ELEMENTS__;
// 
//   // The last element is the QColorDialog opener.
//   if(element == __COLOR_CHOOSER_NUM_ELEMENTS__ - 1)
//   {
//     QColor c = QColorDialog::getColor(_color, this, QString());
//     if(!c.isValid())
//       return;
//     _color = c;
//   }
//   else
//     _color = colorChooserList[element];
// 
//   emit activated(_color);
// 
//   event->accept();
}

// bool MidiCtrlDelegateEditor::event(QEvent* event)
// {
//   if(event->type() == QEvent::MouseButtonPress)
//   {
//     QMouseEvent* me = static_cast<QMouseEvent*>(event);
//     fprintf(stderr, "MidiCtrlDelegateEditor::event: Press X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
//     event->accept();
//     return true;
//   }
//   else
//   if(event->type() == QEvent::MouseButtonRelease)
//   {
//     QMouseEvent* me = static_cast<QMouseEvent*>(event);
//     fprintf(stderr, "MidiCtrlDelegateEditor::event: Release X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
//     //event->accept();
//     //return true;
//   }
//   else
//   if(event->type() == QEvent::Close)
//   {
//     fprintf(stderr, "MidiCtrlDelegateEditor::event: Close\n");  // REMOVE Tim.
//   }
//   else
//     fprintf(stderr, "MidiCtrlDelegateEditor::event: type:%d\n", event->type());  // REMOVE Tim.
//
//   return QWidget::event(event);
// }


//-----------------------------------
//   ColorListEditor
//-----------------------------------

ColorListEditor::ColorListEditor(QWidget *parent) : QComboBox(parent)
{
  populateList();
}

QColor ColorListEditor::color() const
{
  return qvariant_cast<QColor>(itemData(currentIndex(), Qt::DecorationRole));
}

void ColorListEditor::setColor(QColor color)
{
  setCurrentIndex(findData(color, int(Qt::DecorationRole)));
}

void ColorListEditor::populateList()
{
    QStringList colorNames = QColor::colorNames();

    for (int i = 0; i < colorNames.size(); ++i) {
        QColor color(colorNames[i]);

        //insertItem(i, colorNames[i]);
        //setItemData(i, color, Qt::DecorationRole);
        addItem(QString());
        model()->setData(model()->index(i, 0), color, Qt::DecorationRole);
    }
}

//-----------------------------------
//   MapperAssignDelegate
//-----------------------------------

void MapperAssignDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
//     if (index.data().canConvert<StarRating>()) {
//         StarRating starRating = qvariant_cast<StarRating>(index.data());
// 
//         if (option.state & QStyle::State_Selected)
//             painter->fillRect(option.rect, option.palette.highlight());
// 
//         starRating.paint(painter, option.rect, option.palette,
//                         StarRating::ReadOnly);
//     } else 
        QStyledItemDelegate::paint(painter, option, index);
}

QWidget* MapperAssignDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
//     if (index.data().canConvert<StarRating>()) {
//         StarEditor *editor = new StarEditor(parent);
//         connect(editor, SIGNAL(editingFinished()),
//                 this, SLOT(commitAndCloseEditor()));
//         return editor;
//     } else


  //if(index.column() == 
  return QStyledItemDelegate::createEditor(parent, option, index);
}

void MapperAssignDelegate::commitAndCloseEditor()
{
//     StarEditor *editor = qobject_cast<StarEditor *>(sender());
//     emit commitData(editor);
//     emit closeEditor(editor);
}

void MapperAssignDelegate::setEditorData(QWidget *editor,
                                  const QModelIndex &index) const
{
//      if (index.data().canConvert<StarRating>()) {
//          StarRating starRating = qvariant_cast<StarRating>(index.data());
//          StarEditor *starEditor = qobject_cast<StarEditor *>(editor);
//          starEditor->setStarRating(starRating);
//      } else 
         QStyledItemDelegate::setEditorData(editor, index);
}

void MapperAssignDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                 const QModelIndex &index) const
{
//      if (index.data().canConvert<StarRating>()) {
//          StarEditor *starEditor = qobject_cast<StarEditor *>(editor);
//          model->setData(index, QVariant::fromValue(starEditor->starRating()));
//      } else 
         QStyledItemDelegate::setModelData(editor, model, index);
}

QSize MapperAssignDelegate::sizeHint(const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
//     if (index.data().canConvert<StarRating>()) {
//         StarRating starRating = qvariant_cast<StarRating>(index.data());
//         return starRating.sizeHint();
//     } else 
        return QStyledItemDelegate::sizeHint(option, index);
}

//-----------------------------------
//   MapperControlDelegate
//-----------------------------------

MapperControlDelegate::MapperControlDelegate(QWidget *parent) : QStyledItemDelegate(parent)
{
  _firstPress = true;
}

//-----------------------------------
//   getItemRectangle
//   editor is optional and provides info 
//-----------------------------------

QRect MapperControlDelegate::getItemRectangle(const QStyleOptionViewItem& option, const QModelIndex& index, QStyle::SubElement subElement, QWidget* editor) const
{
    // Taken from QStyledItemDelegate source. 
    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    const QWidget* widget = NULL;
    const QStyleOptionViewItemV3* v3 = qstyleoption_cast<const QStyleOptionViewItemV3*>(&option);
    if(v3)
      widget = v3->widget;
    // Let the editor take up all available space if the editor is not a QLineEdit or it is in a QTableView.
    #if !defined(QT_NO_TABLEVIEW) && !defined(QT_NO_LINEEDIT)
    if(editor && qobject_cast<QLineEdit*>(editor) && !qobject_cast<const QTableView*>(widget))
      opt.showDecorationSelected = editor->style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, 0, editor);
    else
    #endif
      opt.showDecorationSelected = true;
    const QStyle *style = widget ? widget->style() : QApplication::style();
//     if(editor->layoutDirection() == Qt::RightToLeft)
//     {
//       const int delta = qSmartMinSize(editor).width() - r.width();       // qSmartMinSize ???
//       if (delta > 0)
//       {
//         //we need to widen the geometry
//         r.adjust(-delta, 0, 0, 0);
//       }
//     }

  return style->subElementRect(subElement, &opt, widget);
}

//-----------------------------------
//   subElementHitTest
//   editor is optional and provides info
//-----------------------------------

bool MapperControlDelegate::subElementHitTest(const QPoint& point, const QStyleOptionViewItem& option, const QModelIndex& index, QStyle::SubElement* subElement, QWidget* editor) const
{
  QRect checkBoxRect = getItemRectangle(option, index, QStyle::SE_ItemViewItemCheckIndicator, editor);
  if(checkBoxRect.isValid() && checkBoxRect.contains(point))
  {
    if(subElement)
      (*subElement) = QStyle::SE_ItemViewItemCheckIndicator;
    return true;
  }

  QRect decorationRect = getItemRectangle(option, index, QStyle::SE_ItemViewItemDecoration, editor);
  if(decorationRect.isValid() && decorationRect.contains(point))
  {
    if(subElement)
      (*subElement) = QStyle::SE_ItemViewItemDecoration;
    return true;
  }

  QRect textRect = getItemRectangle(option, index, QStyle::SE_ItemViewItemText, editor);
  if(textRect.isValid() && textRect.contains(point))
  {
    if(subElement)
      (*subElement) = QStyle::SE_ItemViewItemText;
    return true;
  }

  return false;
}

//-----------------------------------
//   updateEditorGeometry
//-----------------------------------

void MapperControlDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  // REMOVE Tim.
  fprintf(stderr, "ColorChooserEditor::updateEditorGeometry editor x:%d y:%d w:%d h:%d rect x:%d y:%d w:%d h:%d\n",
          editor->x(), editor->y(), editor->width(), editor->height(),
          option.rect.x(), option.rect.y(), option.rect.width(), option.rect.height());

  // For the color editor, move it down to the start of the next item so it doesn't cover the current item row.
  // Width and height are not used - the color editor fixates it's own width and height.
  if(index.column() == ControlMapperDialog::C_NAME)
  {
    QRect r = getItemRectangle(option, index, QStyle::SE_ItemViewItemText, editor);  // Get the text rectangle.
    if(r.isValid())
    {
      editor->move(r.x(), option.rect.y() + option.rect.height());
      return;
    }
  }
  
  QStyledItemDelegate::updateEditorGeometry(editor, option, index);
}

void MapperControlDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index) const
{
//   fprintf(stderr, "MapperControlDelegate::paint row:%d col:%d, rect x:%d y:%d w:%d h:%d showDecorationSelected:%d\n",
//           index.row(), index.column(),
//           option.rect.x(), option.rect.y(), option.rect.width(), option.rect.height(),
//           option.showDecorationSelected);  // REMOVE Tim.
  
  //QStyleOptionViewItemV4 opt = option;
  //initStyleOption(&opt, index);
  //opt.showDecorationSelected = false;
  
  // TODO: Don't forget these if necessary.
  //painter->save();
  //painter->restore();
  
//     if (index.data().canConvert<StarRating>()) {
//         StarRating starRating = qvariant_cast<StarRating>(index.data());
//
//         if (option.state & QStyle::State_Selected)
//             painter->fillRect(option.rect, option.palette.highlight());
//
//         starRating.paint(painter, option.rect, option.palette,
//                         StarRating::ReadOnly);
//     } else

//   if(index.column() == ControlMapperDialog::C_NAME)
//   {
//     // TODO: Disable all this Style stuff if using a style sheet.
// 
//     //QRect disclosure_r = getItemRectangle(option, index, QStyle::SE_TreeViewDisclosureItem);  // Get the text rectangle.
//     //if(disclosure_r.isValid())
//     //{
//     //}
//       
//     QRect checkbox_r = getItemRectangle(option, index, QStyle::SE_ItemViewItemCheckIndicator);  // Get the text rectangle.
//     if(checkbox_r.isValid())
//     {
//       if(option.state & QStyle::State_Selected)
//         painter->fillRect(checkbox_r & option.rect, option.palette.highlight());
//       QStyleOptionViewItemV4 opt = option;
//       initStyleOption(&opt, index);         // Required ?
//       opt.rect = checkbox_r & option.rect;
//       QApplication::style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &opt, painter);
//       //QApplication::style()->drawControl();
//     }
// 
//     //QApplication::style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, painter);
// 
//     //QApplication::style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &option, painter);
//     
//     QRect deco_r = getItemRectangle(option, index, QStyle::SE_ItemViewItemDecoration);  // Get the text rectangle.
//     if(deco_r.isValid())
//       painter->fillRect(deco_r & option.rect, index.data(Qt::DecorationRole).value<QColor>());
//     
//     QRect text_r = getItemRectangle(option, index, QStyle::SE_ItemViewItemText);  // Get the text rectangle.
//     if(text_r.isValid())
//     {
//       if(option.state & QStyle::State_Selected)
//         painter->fillRect(text_r & option.rect, option.palette.highlight());
//       QApplication::style()->drawItemText(painter, text_r & option.rect, option.displayAlignment, option.palette, true, index.data(Qt::DisplayRole).toString());
//     }
//     
//     return;
//   }
  
  QStyledItemDelegate::paint(painter, option, index);
  //QStyledItemDelegate::paint(painter, opt, index);
}

QWidget* MapperControlDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
//     if (index.data().canConvert<StarRating>()) {
//         StarEditor *editor = new StarEditor(parent);
//         connect(editor, SIGNAL(editingFinished()),
//                 this, SLOT(commitAndCloseEditor()));
//         return editor;
//     } else

  int opt_state = option.state;
  fprintf(stderr, "MapperControlDelegate::createEditor option state:%d\n", opt_state);  // REMOVE Tim.

  // HACK: For some reason when using CurrentChanged trigger, createEditor is called upon opening the dialog, yet nothing is selected.
  // It suddenly started doing that after working just fine. Can't find what may have changed.
  //if(!(option.state & QStyle::State_Selected))   // Nope. option.state is always the same, never seems to change.
  //  return NULL;
  //if(_firstPress)
  //  return NULL;
  
  switch(index.column())
  {
//     case ControlMapperDialog::C_SHOW:
//       //return QStyledItemDelegate::createEditor(parent, option, index);
//       // This is a checkbox column. No editable info.
//       //fprintf(stderr, "ERROR: MapperControlDelegate::createEditor called for SHOW column\n");
//       return 0;

    //case ControlMapperDialog::C_NAME:
      //fprintf(stderr, "ERROR: MapperControlDelegate::createEditor called for NAME column\n");
      // This seems to be a way we can prevent editing of a cell here in this tree widget.
      // Table widget has individual item cell edting enable but here in tree widget it's per row.
      //return 0;

    //case ControlMapperDialog::C_COLOR:
    case ControlMapperDialog::C_NAME:
    {
      ColorEditor* color_list = new ColorEditor(parent);
      //connect(color_list, SIGNAL(activated(int)), this, SLOT(colorEditorChanged()));
      connect(color_list, SIGNAL(activated(const QColor&)), this, SLOT(editorChanged()));
      return color_list;
    }

    case ControlMapperDialog::C_ASSIGN_PORT:
    {
      QComboBox* combo = new QComboBox(parent);

//       combo->addItem(tr("<None>"), -1);
//       combo->addItem(tr("Control7"), MusECore::MidiController::Controller7);
//       combo->addItem(tr("Control14"), MusECore::MidiController::Controller14);
//       combo->addItem(tr("RPN"), MusECore::MidiController::RPN);
//       combo->addItem(tr("NPRN"), MusECore::MidiController::NRPN);
//       combo->addItem(tr("RPN14"), MusECore::MidiController::RPN14);
//       combo->addItem(tr("NRPN14"), MusECore::MidiController::NRPN14);
//       combo->addItem(tr("Pitch"), MusECore::MidiController::Pitch);
//       combo->addItem(tr("Program"), MusECore::MidiController::Program);
//       //combo->addItem(tr("PolyAftertouch"), MusECore::MidiController::PolyAftertouch); // Not supported yet. Need a way to select pitch.
//       combo->addItem(tr("Aftertouch"), MusECore::MidiController::Aftertouch);
//       //combo->setCurrentIndex(0);

//       combo->addItem(tr("<None>"), -1);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller7), MusECore::MidiController::Controller7);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller14), MusECore::MidiController::Controller14);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN), MusECore::MidiController::RPN);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN), MusECore::MidiController::NRPN);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN14), MusECore::MidiController::RPN14);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN14), MusECore::MidiController::NRPN14);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Pitch), MusECore::MidiController::Pitch);
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Program), MusECore::MidiController::Program);
//       //combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::PolyAftertouch), MusECore::MidiController::PolyAftertouch); // Not supported yet. Need a way to select pitch.
//       combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Aftertouch), MusECore::MidiController::Aftertouch);

      combo->addItem("---", -1);
      int port = index.data(Qt::UserRole).toInt();
      QString port_name;
      for(int i = 0; i < MIDI_PORTS; ++i)
      {
        MusECore::MidiDevice* md = MusEGlobal::midiPorts[i].device();
        //if(!md)  // In the case of this combo box, don't bother listing empty ports.
        //  continue;
        //if(!(md->rwFlags() & 1 || md->isSynti()) && (i != outPort))
        if(!(md && (md->rwFlags() & 2)) && (i != port))   // Only readable ports, or current one.
          continue;
        //name.sprintf("%d:%s", i+1, MusEGlobal::midiPorts[i].portname().toLatin1().constData());
        QString name = QString("%1:%2").arg(i+1).arg(MusEGlobal::midiPorts[i].portname());
        combo->addItem(name, i);
      }
      connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(editorChanged()));
      return combo;
    }

    case ControlMapperDialog::C_ASSIGN_CHAN:
    {
//       QSpinBox* spin_box = new QSpinBox(parent);
//       spin_box->setMinimum(0);
//       spin_box->setMaximum(127);
//       return spin_box;

      QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
      QSpinBox* spin_box = qobject_cast<QSpinBox*>(widget);
      if(spin_box)
      {
        spin_box->setMinimum(0);
        spin_box->setMaximum(MIDI_CHANNELS - 1);
      }
      return widget;
    }
    
    case ControlMapperDialog::C_MCTL_NUM:
    {
      QComboBox* combo = new QComboBox(parent);

//       combo->addItem(tr("<None>"), -1);
//       combo->addItem(tr("Control7"), MusECore::MidiController::Controller7);
//       combo->addItem(tr("Control14"), MusECore::MidiController::Controller14);
//       combo->addItem(tr("RPN"), MusECore::MidiController::RPN);
//       combo->addItem(tr("NPRN"), MusECore::MidiController::NRPN);
//       combo->addItem(tr("RPN14"), MusECore::MidiController::RPN14);
//       combo->addItem(tr("NRPN14"), MusECore::MidiController::NRPN14);
//       combo->addItem(tr("Pitch"), MusECore::MidiController::Pitch);
//       combo->addItem(tr("Program"), MusECore::MidiController::Program);
//       //combo->addItem(tr("PolyAftertouch"), MusECore::MidiController::PolyAftertouch); // Not supported yet. Need a way to select pitch.
//       combo->addItem(tr("Aftertouch"), MusECore::MidiController::Aftertouch);
//       //combo->setCurrentIndex(0);

      //combo->addItem(tr("<None>"), -1);
      combo->addItem("---", -1);
      combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller7), MusECore::MidiController::Controller7);
      combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Controller14), MusECore::MidiController::Controller14);
      combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN), MusECore::MidiController::RPN);
      combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN), MusECore::MidiController::NRPN);
      combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::RPN14), MusECore::MidiController::RPN14);
      combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::NRPN14), MusECore::MidiController::NRPN14);
      combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Pitch), MusECore::MidiController::Pitch);
      combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Program), MusECore::MidiController::Program);
      // TODO Per-pitch controls not supported yet. Need a way to select pitch.
      //combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::PolyAftertouch), MusECore::MidiController::PolyAftertouch);
      combo->addItem(MusECore::int2ctrlType(MusECore::MidiController::Aftertouch), MusECore::MidiController::Aftertouch);
      connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(editorChanged()));
      return combo;
    }

//     case ControlMapperDialog::C_MCTL_H:
//     {
// //       QSpinBox* spin_box = new QSpinBox(parent);
// //       spin_box->setMinimum(0);
// //       spin_box->setMaximum(127);
// //       return spin_box;
//       
//       QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
//       QSpinBox* spin_box = qobject_cast<QSpinBox*>(widget);
//       if(spin_box)
//       {
//         spin_box->setMinimum(0);
//         spin_box->setMaximum(127);
//       }
//       return widget;
//     }

///     case ControlMapperDialog::C_MCTL_H:
//     case ControlMapperDialog::C_MCTL_L:
//     {
// //       QSpinBox* spin_box = new QSpinBox(parent);
// //       spin_box->setMinimum(0);
// //       spin_box->setMaximum(127);
// //       return spin_box;
//       
//       QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
//       QSpinBox* spin_box = qobject_cast<QSpinBox*>(widget);
//       if(spin_box)
//       {
//         spin_box->setMinimum(0);
//         spin_box->setMaximum(127);
//       }
//       return widget;
//     }
  }
  
  return QStyledItemDelegate::createEditor(parent, option, index);
}

void MapperControlDelegate::editorChanged()
{
//     StarEditor *editor = qobject_cast<StarEditor *>(sender());
//     emit commitData(editor);
//     emit closeEditor(editor);

  fprintf(stderr, "MapperControlDelegate::editorChanged\n");  // REMOVE Tim.
  
  // Wow, I thought using sender was frowned upon ("breaks modularity"). But hey, it's necessary sometimes. TODO Improve this?
  //ColorEditor* editor = qobject_cast<ColorEditor*>(sender());
  QWidget* editor = qobject_cast<QWidget*>(sender());
  if(editor)
  {
    emit commitData(editor);
    emit closeEditor(editor);
  }
}

// void MapperControlDelegate::commitAndCloseEditor()
// {
// //     StarEditor *editor = qobject_cast<StarEditor *>(sender());
// //     emit commitData(editor);
// //     emit closeEditor(editor);
// }

void MapperControlDelegate::setEditorData(QWidget *editor,
                                  const QModelIndex &index) const
{
  fprintf(stderr, "MapperControlDelegate::setEditorData\n");  // REMOVE Tim.
//      if (index.data().canConvert<StarRating>()) {
//          StarRating starRating = qvariant_cast<StarRating>(index.data());
//          StarEditor *starEditor = qobject_cast<StarEditor *>(editor);
//          starEditor->setStarRating(starRating);
//      } else
  
   //if(index.column() == ControlMapperDialog::C_COLOR)


  switch(index.column())
  {
    case ControlMapperDialog::C_NAME:
    {
      ColorEditor* color_editor = qobject_cast<ColorEditor*>(editor);
      if(color_editor)
        color_editor->setColor(index.data(Qt::DecorationRole).value<QColor>());
      return;
    }

    case ControlMapperDialog::C_ASSIGN_PORT:
    case ControlMapperDialog::C_MCTL_NUM:
    {
      QComboBox* combo = qobject_cast<QComboBox*>(editor);
      if(combo)
      {
        int data = index.data(Qt::UserRole).toInt();
        int idx = combo->findData(data);
        if(idx != -1)
        {
          combo->blockSignals(true);     // Prevent currentIndexChanged or activated from being called
          combo->setCurrentIndex(idx);
          combo->blockSignals(false);
        }
      }
      return;
    }

    default:
      QStyledItemDelegate::setEditorData(editor, index);
  }
   
//    if(index.column() == ControlMapperDialog::C_NAME)
//    {
//      ColorEditor* color_editor = qobject_cast<ColorEditor*>(editor);
//      if(color_editor)
//        color_editor->setColor(index.data(Qt::DecorationRole).value<QColor>());
//    }
//    else
//    if(index.column() == ControlMapperDialog::C_ASSIGN_PORT)
//    {
//      QComboBox* combo = qobject_cast<QComboBox*>(editor);
//      if(combo)
//      {
//        int data = index.data(Qt::UserRole).toInt();
//        int idx = combo->findData(data);
//        if(idx != -1)
//          combo->setCurrentIndex(idx);
//      }
//    }
//    else
//    if(index.column() == ControlMapperDialog::C_MCTL_TYPE)
//    {
//      QComboBox* combo = qobject_cast<QComboBox*>(editor);
//      if(combo)
//      {
//        int data = index.data(Qt::UserRole).toInt();
//        int idx = combo->findData(data);
//        if(idx != -1)
//          combo->setCurrentIndex(idx);
//      }
//    }
//    else
//      QStyledItemDelegate::setEditorData(editor, index);
}

void MapperControlDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                 const QModelIndex &index) const
{
  fprintf(stderr, "MapperControlDelegate::setModelData\n");  // REMOVE Tim.
//      if (index.data().canConvert<StarRating>()) {
//          StarEditor *starEditor = qobject_cast<StarEditor *>(editor);
//          model->setData(index, QVariant::fromValue(starEditor->starRating()));
//      } else

   //if(index.column() == ControlMapperDialog::C_COLOR)

  switch(index.column())
  {
    case ControlMapperDialog::C_NAME:
    {
      ColorEditor* color_editor = qobject_cast<ColorEditor*>(editor);
      if(color_editor)
        model->setData(index, color_editor->color(), Qt::DecorationRole);
      return;
    }

    case ControlMapperDialog::C_ASSIGN_PORT:
    case ControlMapperDialog::C_MCTL_NUM:
    {
      QComboBox* combo = qobject_cast<QComboBox*>(editor);
      if(combo)
      {
        int idx = combo->currentIndex();
        if(idx != -1)
        {
          model->setData(index, combo->itemData(idx), Qt::UserRole);    // Do this one before the text so that the tree view's itemChanged handler gets it first!
          model->blockSignals(true);
          model->setData(index, combo->itemText(idx), Qt::DisplayRole); // This will cause another handler call. Prevent it by blocking.
          model->blockSignals(false);
        }
      }
      return;
    }

    default:
       QStyledItemDelegate::setModelData(editor, model, index);
  }

//    if(index.column() == ControlMapperDialog::C_NAME)
//    {
//      ColorEditor* color_editor = qobject_cast<ColorEditor*>(editor);
//      if(color_editor)
//        model->setData(index, color_editor->color(), Qt::DecorationRole);
//    }
//    else
//    if(index.column() == ControlMapperDialog::C_ASSIGN_PORT)
//    {
//      QComboBox* combo = qobject_cast<QComboBox*>(editor);
//      if(combo)
//      {
//        int idx = combo->currentIndex();
//        if(idx != -1)
//        {
//          model->setData(index, combo->itemText(idx), Qt::DisplayRole);
//          model->setData(index, combo->itemData(idx), Qt::UserRole);
//        }
//      }
//    }
//    else
//    if(index.column() == ControlMapperDialog::C_MCTL_TYPE)
//    {
//      QComboBox* combo = qobject_cast<QComboBox*>(editor);
//      if(combo)
//      {
//        int idx = combo->currentIndex();
//        if(idx != -1)
//        {
//          model->setData(index, combo->itemText(idx), Qt::DisplayRole);
//          model->setData(index, combo->itemData(idx), Qt::UserRole);
//        }
//      }
//    }
//    else
//      QStyledItemDelegate::setModelData(editor, model, index);
}

QSize MapperControlDelegate::sizeHint(const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
//     if (index.data().canConvert<StarRating>()) {
//         StarRating starRating = qvariant_cast<StarRating>(index.data());
//         return starRating.sizeHint();
//     } else

//   if(index.column() == ControlMapperDialog::C_COLOR)
//     return QSize(__COLOR_CHOOSER_ELEMENT_WIDTH__ * __COLOR_CHOOSER_NUM_COLUMNS__,
//                  __COLOR_CHOOSER_ELEMENT_HEIGHT__ * (__COLOR_CHOOSER_NUM_ELEMENTS__ / __COLOR_CHOOSER_NUM_COLUMNS__));
//     
  return QStyledItemDelegate::sizeHint(option, index);
}

bool MapperControlDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  if(event->type() == QEvent::MouseMove)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(event);
    fprintf(stderr, "MapperControlDelegate::editorEvent: Move X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
    // If any buttons down, ignore.
    if(me->buttons() != Qt::NoButton)
    {
      event->accept();
      return true;
    }
  }
  else
  if(event->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(event);
    fprintf(stderr, "MapperControlDelegate::editorEvent: Press X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.

    _firstPress = false;  // HACK
    
    QStyle::SubElement sub_element;
    if(subElementHitTest(me->pos(), option, index, &sub_element))
      _currentSubElement = sub_element;
    //event->accept();
    //return true;
  }
  else
  if(event->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(event);
    fprintf(stderr, "MapperControlDelegate::editorEvent: Release X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.

    // If the element under the mouse is not the one when pressed, eat up these events because
    //  they trigger the editor or action of the element under the mouse at the release position.
    QStyle::SubElement sub_element = _currentSubElement;
    if(!subElementHitTest(me->pos(), option, index, &sub_element) || sub_element != _currentSubElement)
    //QRect r = getItemRectangle(option, index, QStyle::SE_ItemViewItemDecoration);
    //if(!subElementHitTest(me->pos(), option, index, &sub_element) ||
    //  (sub_element != QStyle::SE_ItemViewItemCheckIndicator && sub_element != QStyle::SE_ItemViewItemDecoration))
    //if(r.isValid())
    {
      event->accept();
      return true;
    }
  }
  else
  if(event->type() == QEvent::Close)
  {
    fprintf(stderr, "MapperControlDelegate::editorEvent: Close\n");  // REMOVE Tim.
  }
  else
    fprintf(stderr, "MapperControlDelegate::editorEvent: event type:%d\n", event->type());  // REMOVE Tim.


//   switch(index.column())
//   {
//     case ControlMapperDialog::C_SHOW:
//       // This is checkbox column. No editable info.
//       //event->accept();
//       //return true;
//       //return false;
//       return QStyledItemDelegate::editorEvent(event, model, option, index);
// 
//     case ControlMapperDialog::C_NAME:
//       // This is non-editable name.
//       event->accept();
//       return true;
// 
//     case ControlMapperDialog::C_COLOR:
//     {
//       if(event->type() == QEvent::MouseButtonRelease)
//       {
//         QMouseEvent* me = static_cast<QMouseEvent*>(event);
//         fprintf(stderr, " X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
// 
//       }
// 
//       event->accept();
//       return true;
//     }
// 
//     case ControlMapperDialog::C_ASSIGN:
//       // This is editable assigned input controller.
//       return false;
// 
//     case ControlMapperDialog::C_MCTL_TYPE:
//       // This is editable midi control type.
//       return false;
// 
//     case ControlMapperDialog::C_MCTL_H:
//       // This is editable midi control num high.
//       return false;
// 
//     case ControlMapperDialog::C_MCTL_L:
//       // This is editable midi control num low.
//       return false;
//   }
// 
//   return false;

  return QStyledItemDelegate::editorEvent(event, model, option, index);
}


bool MapperControlDelegate::eventFilter(QObject* editor, QEvent* event)
{
  if(event->type() == QEvent::MouseButtonPress)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(event);
    fprintf(stderr, "MapperControlDelegate::eventFilter: Press X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
    //event->accept();
    //return true;
  }
  else
  if(event->type() == QEvent::MouseButtonRelease)
  {
    QMouseEvent* me = static_cast<QMouseEvent*>(event);
    fprintf(stderr, "MapperControlDelegate::eventFilter: Release X:%d Y:%d gX:%d gY:%d\n", me->x(), me->y(), me->globalX(), me->globalY());  // REMOVE Tim.
    //event->accept();
    //return true;
  }
  else
  if(event->type() == QEvent::Close)
  {
    fprintf(stderr, "MapperControlDelegate::eventFilter: Close\n");  // REMOVE Tim.
  }
  else
    fprintf(stderr, "MapperControlDelegate::eventFilter: event type:%d\n", event->type());  // REMOVE Tim.

  return QStyledItemDelegate::eventFilter(editor, event);
}

 
// -----------------------------------
//   ControlMapperDialog
// -----------------------------------

ControlMapperDialog::ControlMapperDialog(MusECore::Track* t, QWidget* parent)
  : QDialog(parent), _track(t)
{
  setupUi(this);

  if(_track->isMidiTrack())
  {
    fprintf(stderr, "ERROR: ControlMapperDialog ctor: track type is midi - unsupported yet. Aborting!\n");
    abort();
  }
  MusECore::AudioTrack* atrack = static_cast<MusECore::AudioTrack*>(_track);
  _actrls = atrack->controller();
  _mctrls = NULL;  
  if(_track->type() == MusECore::Track::AUDIO_SOFTSYNTH)
  {
    MusECore::SynthI* si = static_cast<MusECore::SynthI*>(_track);
    _mctrls = ((MusECore::MidiInstrument*)si)->controller();
  }

  MusECore::MidiAudioCtrlMap* map = _actrls->midiControls();
  
//   QItemEditorFactory* factory = new QItemEditorFactory;
//   //QItemEditorFactory* factory = QItemEditorFactory::defaultFactory();
//   QItemEditorCreatorBase* colorListCreator = new QStandardItemEditorCreator<ColorListEditor>();
//   factory->registerEditor(QVariant::Color, colorListCreator);
//   //QItemEditorFactory::defaultFactory()->registerEditor(QVariant::Color, colorListCreator);
//   //QItemEditorFactory::setDefaultFactory(factory);


  controlList->setColumnCount(C_COL_END);
  //QStringList control_header_names = QStringList() << tr("Show") << tr("Name") << tr("Color") << tr("Assign") << tr("Midi ctl type") << tr("Ctl H") << tr("Ctl L");
  //QStringList control_header_names = QStringList() << tr("Name") << tr("Color") << tr("Assign") << tr("Midi ctl type") << tr("Ctl H") << tr("Ctl L");
  ///QStringList control_header_names = QStringList() << tr("Name") << tr("Assign port") << tr("Chan") << tr("Midi ctl type") << tr("Ctl H") << tr("Ctl L");
  QStringList control_header_names = QStringList() << tr("Name") << tr("Assign port") << tr("Chan") << tr("Midi control");
  controlList->setHeaderLabels(control_header_names);

  //controlList->setIconSize(QSize(16, 16));
  
  //MapperControlDelegate controls_delegate;
  //controlList->setItemDelegate(&controls_delegate);

  _assignDelegate =   new MapperAssignDelegate(this);
  _controlsDelegate = new MapperControlDelegate(this);
  ///_assignDelegate->setItemEditorFactory(factory);
  ///_controlsDelegate->setItemEditorFactory(factory);

  controlList->setItemDelegate(_controlsDelegate);

  MusECore::AudioMidiCtrlStructMap macm;
  int port, chan, ctl_num;
  bool is_output_port;

  QList<QTreeWidgetItem*> new_items;
  for(MusECore::ciCtrlList icl = _actrls->begin(); icl != _actrls->end(); ++icl)
  {
    MusECore::CtrlList* cl = icl->second;
    if(cl->dontShow())
      continue;
    QTreeWidgetItem* control_item = new QTreeWidgetItem;
    new_items.append(control_item);
    
    control_item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    control_item->setData(C_NAME, Qt::UserRole, cl->id());

    //control_item->setData(C_SHOW, Qt::EditRole, cl->isVisible());
    //control_item->setData(C_SHOW, Qt::CheckStateRole, cl->isVisible());
    // To make a checkbox appear at all, must call setCheckState.
    //control_item->setCheckState(C_SHOW, cl->isVisible() ? Qt::Checked : Qt::Unchecked);
    control_item->setData(C_NAME, Qt::CheckStateRole, cl->isVisible() ? Qt::Checked : Qt::Unchecked);
    control_item->setData(C_NAME, Qt::DisplayRole, cl->name()); // Not editable.
    //control_item->setData(C_COLOR, Qt::EditRole, cl->color());
    //control_item->setData(C_COLOR, Qt::DecorationRole, cl->color());
    control_item->setData(C_NAME, Qt::DecorationRole, cl->color());

    macm.clear();
    map->find_audio_ctrl_structs(cl->id(), &macm);

    ///int ctl_type = -1; // "<None>"
    ctl_num = -1;
    port = -1;
    chan = 0;
    is_output_port = false;
    QString port_name = "---";
    QString ctl_type_name = "---";
    for(MusECore::ciAudioMidiCtrlStructMap imacm = macm.begin(); imacm != macm.end(); ++imacm)
    {
      // TODO: Only one connection to this audio control supported for now.
      map->hash_values((*imacm)->first, &port, &chan, &ctl_num, &is_output_port);
      MusECore::MidiController::ControllerType t = MusECore::midiControllerType(ctl_num);
      if(t == MusECore::MidiController::PolyAftertouch)  // TODO Not supported yet. Need a way to select pitch.
        continue;
      ///ctl_type = t;
      ctl_type_name = MusECore::int2ctrlType(t) + ":" + MusECore::midiCtrlNumString(ctl_num);

      if(port != -1)
        port_name = QString("%1:%2").arg(port + 1).arg(MusEGlobal::midiPorts[port].portname());
      break;
    }
    control_item->setData(C_ASSIGN_PORT, Qt::DisplayRole, port_name);
    control_item->setData(C_ASSIGN_PORT, Qt::UserRole,    port);
    control_item->setData(C_ASSIGN_CHAN, Qt::EditRole,    chan);
    control_item->setData(C_MCTL_NUM,   Qt::DisplayRole, ctl_type_name);
    ///control_item->setData(C_MCTL_NUM,   Qt::UserRole,    ctl_type);
    ///control_item->setData(C_MCTL_H,      Qt::EditRole,    (ctl_num >> 8) & 0x7f);
    ///control_item->setData(C_MCTL_L,      Qt::EditRole,    ctl_num & 0x7f);
    control_item->setData(C_MCTL_NUM, Qt::UserRole, ctl_num);
    control_item->setData(C_MCTL_NUM, UserRole2, (void*)_mctrls);
  }
  controlList->addTopLevelItems(new_items);
  
  for(int i = 0; i < C_COL_END; ++i)
    controlList->resizeColumnToContents(i);

  controlList->setEditTriggers(QAbstractItemView::AllEditTriggers);
  
  connect(controlList, SIGNAL(itemChanged(QTreeWidgetItem*, int)), SLOT(controlsItemChanged(QTreeWidgetItem*, int)));

  
/*
 
  controlTypeComboBox->addItem(tr("Control7"), MusECore::MidiController::Controller7);
  controlTypeComboBox->addItem(tr("Control14"), MusECore::MidiController::Controller14);
  controlTypeComboBox->addItem(tr("RPN"), MusECore::MidiController::RPN);
  controlTypeComboBox->addItem(tr("NPRN"), MusECore::MidiController::NRPN);
  controlTypeComboBox->addItem(tr("RPN14"), MusECore::MidiController::RPN14);
  controlTypeComboBox->addItem(tr("NRPN14"), MusECore::MidiController::NRPN14);
  controlTypeComboBox->addItem(tr("Pitch"), MusECore::MidiController::Pitch);
  controlTypeComboBox->addItem(tr("Program"), MusECore::MidiController::Program);
  //controlTypeComboBox->addItem(tr("PolyAftertouch"), MusECore::MidiController::PolyAftertouch); // Not supported yet. Need a way to select pitch.
  controlTypeComboBox->addItem(tr("Aftertouch"), MusECore::MidiController::Aftertouch);
  controlTypeComboBox->setCurrentIndex(0);

  _port = port;
  _chan = chan;
  _ctrl = ctrl;
  _is_learning = false;

  update();

  connect(learnPushButton, SIGNAL(clicked(bool)), SLOT(learnChanged(bool)));
  connect(portComboBox, SIGNAL(currentIndexChanged(int)), SLOT(portChanged(int)));
  connect(channelSpinBox, SIGNAL(valueChanged(int)), SLOT(chanChanged()));
  connect(controlTypeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(ctrlTypeChanged(int)));
  connect(ctrlHiSpinBox, SIGNAL(valueChanged(int)), SLOT(ctrlHChanged()));
  connect(ctrlLoSpinBox, SIGNAL(valueChanged(int)), SLOT(ctrlLChanged()));
  connect(MusEGlobal::muse, SIGNAL(configChanged()), SLOT(configChanged()));
  connect(MusEGlobal::heartBeatTimer, SIGNAL(timeout()), SLOT(heartbeat()));*/
}

ControlMapperDialog::~ControlMapperDialog()
{
//   if(_controlsDelegate)
//     delete _controlsDelegate;
//   if(_assignDelegate)
//     delete _assignDelegate;
}

void ControlMapperDialog::heartbeat()
{
//   if(_is_learning)
//   {
//     if(MusEGlobal::midiLearnPort != -1)
//     {
//       int port_item = portComboBox->findData(MusEGlobal::midiLearnPort);
//       if(port_item != -1 && port_item != portComboBox->currentIndex())
//       {
//         _port = MusEGlobal::midiLearnPort;
//         portComboBox->blockSignals(true);
//         portComboBox->setCurrentIndex(port_item);
//         portComboBox->blockSignals(false);
//       }
//     }
// 
//     if(MusEGlobal::midiLearnChan != -1 && (MusEGlobal::midiLearnChan + 1) != channelSpinBox->value())
//     {
//       _chan = MusEGlobal::midiLearnChan;
//       channelSpinBox->blockSignals(true);
//       channelSpinBox->setValue(_chan + 1);
//       channelSpinBox->blockSignals(false);
//     }
// 
//     if(MusEGlobal::midiLearnCtrl != -1)
//     {
//       MusECore::MidiController::ControllerType type = MusECore::midiControllerType(MusEGlobal::midiLearnCtrl);
//       int idx = controlTypeComboBox->findData(type);
//       if(idx != -1 && idx != controlTypeComboBox->currentIndex())
//       {
//         controlTypeComboBox->blockSignals(true);
//         controlTypeComboBox->setCurrentIndex(idx);
//         controlTypeComboBox->blockSignals(false);
//       }
// 
//       int hv = (MusEGlobal::midiLearnCtrl >> 8) & 0xff;
//       int lv = MusEGlobal::midiLearnCtrl & 0xff;
// 
//       switch(type)
//       {
//         case MusECore::MidiController::Program:
//         case MusECore::MidiController::Pitch:
//         case MusECore::MidiController::PolyAftertouch: // Unsupported yet. Need a way to select pitch.
//         case MusECore::MidiController::Aftertouch:
//           ctrlHiSpinBox->setEnabled(false);
//           ctrlLoSpinBox->setEnabled(false);
//           ctrlHiSpinBox->blockSignals(true);
//           ctrlLoSpinBox->blockSignals(true);
//           ctrlHiSpinBox->setValue(0);
//           ctrlLoSpinBox->setValue(0);
//           ctrlHiSpinBox->blockSignals(false);
//           ctrlLoSpinBox->blockSignals(false);
//           break;
//         case MusECore::MidiController::Controller7:
//           ctrlHiSpinBox->setEnabled(false);
//           ctrlLoSpinBox->setEnabled(true);
// 
//           ctrlHiSpinBox->blockSignals(true);
//           ctrlHiSpinBox->setValue(0);
//           ctrlHiSpinBox->blockSignals(false);
// 
//           if(lv != ctrlLoSpinBox->value())
//           {
//             ctrlLoSpinBox->blockSignals(true);
//             ctrlLoSpinBox->setValue(lv);
//             ctrlLoSpinBox->blockSignals(false);
//           }
//           break;
//         case MusECore::MidiController::Controller14:
//         case MusECore::MidiController::RPN:
//         case MusECore::MidiController::RPN14:
//         case MusECore::MidiController::NRPN:
//         case MusECore::MidiController::NRPN14:
//           ctrlHiSpinBox->setEnabled(true);
//           ctrlLoSpinBox->setEnabled(true);
//           if(hv != ctrlHiSpinBox->value())
//           {
//             ctrlHiSpinBox->blockSignals(true);
//             ctrlHiSpinBox->setValue(hv);
//             ctrlHiSpinBox->blockSignals(false);
//           }
//           if(lv != ctrlLoSpinBox->value())
//           {
//             ctrlLoSpinBox->blockSignals(true);
//             ctrlLoSpinBox->setValue(lv);
//             ctrlLoSpinBox->blockSignals(false);
//           }
//           break;
//         default:
//           printf("FIXME: MidiAudioControl::heartbeat: Unknown control type: %d\n", type);
//           break;
//       }
// 
//       _ctrl = MusECore::midiCtrlTerms2Number(type, (ctrlHiSpinBox->value() << 8) + ctrlLoSpinBox->value());
//     }
//   }
}

void ControlMapperDialog::learnChanged(bool v)
{
  _is_learning = v;
  if(_is_learning)
    MusEGlobal::audio->msgStartMidiLearn();  // Resets the learn values to -1.
}

void ControlMapperDialog::resetLearn()
{
  _is_learning = false;
  learnPushButton->blockSignals(true);
  learnPushButton->setChecked(false);
  learnPushButton->blockSignals(false);
  MusEGlobal::audio->msgStartMidiLearn();  // Resets the learn values to -1.
}

// void ControlMapperDialog::portChanged(int idx)
// {
//   if(idx == -1)
//     return;
//   int port_num = portComboBox->itemData(idx).toInt();
//   if(port_num < 0 || port_num >= MIDI_PORTS)
//     return;
// 
//   _port = port_num;
//   resetLearn();
// }
// 
// void MidiAudioControl::chanChanged()
// {
//   _chan = channelSpinBox->value() - 1;
//   resetLearn();
// }
// 
// void MidiAudioControl::updateCtrlBoxes()
// {
//   if(controlTypeComboBox->currentIndex() == -1)
//     return;
//   MusECore::MidiController::ControllerType t = (MusECore::MidiController::ControllerType)controlTypeComboBox->itemData(controlTypeComboBox->currentIndex()).toInt();
// 
//   switch(t)
//   {
//     case MusECore::MidiController::Program:
//     case MusECore::MidiController::Pitch:
//     case MusECore::MidiController::PolyAftertouch: // Unsupported yet. Need a way to select pitch.
//     case MusECore::MidiController::Aftertouch:
//       ctrlHiSpinBox->setEnabled(false);
//       ctrlLoSpinBox->setEnabled(false);
//       ctrlHiSpinBox->blockSignals(true);
//       ctrlLoSpinBox->blockSignals(true);
//       ctrlHiSpinBox->setValue(0);
//       ctrlLoSpinBox->setValue(0);
//       ctrlHiSpinBox->blockSignals(false);
//       ctrlLoSpinBox->blockSignals(false);
//       break;
//     case MusECore::MidiController::Controller7:
//       ctrlHiSpinBox->setEnabled(false);
//       ctrlLoSpinBox->setEnabled(true);
//       ctrlHiSpinBox->blockSignals(true);
//       ctrlHiSpinBox->setValue(0);
//       ctrlHiSpinBox->blockSignals(false);
//       break;
//     case MusECore::MidiController::Controller14:
//     case MusECore::MidiController::RPN:
//     case MusECore::MidiController::RPN14:
//     case MusECore::MidiController::NRPN:
//     case MusECore::MidiController::NRPN14:
//       ctrlHiSpinBox->setEnabled(true);
//       ctrlLoSpinBox->setEnabled(true);
//       break;
//     default:
//       printf("FIXME: MidiAudioControl::updateCtrlBoxes: Unknown control type: %d\n", t);
//       break;
//   }
// }
// 
// void MidiAudioControl::ctrlTypeChanged(int idx)
// {
//   if(idx == -1)
//     return;
// 
//   updateCtrlBoxes();
// 
//   _ctrl = (ctrlHiSpinBox->value() << 8) + ctrlLoSpinBox->value();
//   _ctrl = MusECore::midiCtrlTerms2Number((MusECore::MidiController::ControllerType)controlTypeComboBox->itemData(idx).toInt(), _ctrl);
// 
//   resetLearn();
// }
// 
// void MidiAudioControl::ctrlHChanged()
// {
//   if(controlTypeComboBox->currentIndex() == -1)
//     return;
//   _ctrl = (ctrlHiSpinBox->value() << 8) + ctrlLoSpinBox->value();
//   _ctrl = MusECore::midiCtrlTerms2Number((MusECore::MidiController::ControllerType)controlTypeComboBox->itemData(controlTypeComboBox->currentIndex()).toInt(), _ctrl);
// 
//   resetLearn();
// }
// 
// void MidiAudioControl::ctrlLChanged()
// {
//   if(controlTypeComboBox->currentIndex() == -1)
//     return;
//   _ctrl = (ctrlHiSpinBox->value() << 8) + ctrlLoSpinBox->value();
//   _ctrl = MusECore::midiCtrlTerms2Number((MusECore::MidiController::ControllerType)controlTypeComboBox->itemData(controlTypeComboBox->currentIndex()).toInt(), _ctrl);
// 
//   resetLearn();
// }

void ControlMapperDialog::configChanged()
{
  doUpdate();
}

void ControlMapperDialog::doUpdate()
{
//   portComboBox->blockSignals(true);
//   portComboBox->clear();
// 
//   int item_idx = 0;
//   for (int i = 0; i < MIDI_PORTS; ++i) {
//         MusECore::MidiDevice* md = MusEGlobal::midiPorts[i].device();
//         if(!md)  // In the case of this combo box, don't bother listing empty ports.
//           continue;
//         //if(!(md->rwFlags() & 1 || md->isSynti()) && (i != outPort))
//         if(!(md->rwFlags() & 2) && (i != _port))   // Only readable ports, or current one.
//           continue;
//         QString name;
//         name.sprintf("%d:%s", i+1, MusEGlobal::midiPorts[i].portname().toLatin1().constData());
//         portComboBox->insertItem(item_idx, name, i);
//         if(_port == -1)
//           _port = i;      // Initialize
//         if(i == _port)
//           portComboBox->setCurrentIndex(item_idx);
//         item_idx++;
//         }
//   portComboBox->blockSignals(false);
// 
//   channelSpinBox->blockSignals(true);
//   channelSpinBox->setValue(_chan + 1);
//   channelSpinBox->blockSignals(false);
// 
//   int type = MusECore::midiControllerType(_ctrl);
//   int idx = controlTypeComboBox->findData(type);
//   if(idx != -1 && idx != controlTypeComboBox->currentIndex())
//   {
//     controlTypeComboBox->blockSignals(true);
//     controlTypeComboBox->setCurrentIndex(idx);
//     controlTypeComboBox->blockSignals(false);
//   }
// 
//   int hv = (_ctrl >> 8) & 0xff;
//   int lv = _ctrl & 0xff;
// 
//   switch(type)
//   {
//     case MusECore::MidiController::Program:
//     case MusECore::MidiController::Pitch:
//     case MusECore::MidiController::PolyAftertouch:  // Unsupported yet. Need a way to select pitch.
//     case MusECore::MidiController::Aftertouch:
//       ctrlHiSpinBox->setEnabled(false);
//       ctrlLoSpinBox->setEnabled(false);
//       ctrlHiSpinBox->blockSignals(true);
//       ctrlLoSpinBox->blockSignals(true);
//       ctrlHiSpinBox->setValue(0);
//       ctrlLoSpinBox->setValue(0);
//       ctrlHiSpinBox->blockSignals(false);
//       ctrlLoSpinBox->blockSignals(false);
//       break;
//     case MusECore::MidiController::Controller7:
//       ctrlHiSpinBox->setEnabled(false);
//       ctrlLoSpinBox->setEnabled(true);
// 
//       ctrlHiSpinBox->blockSignals(true);
//       ctrlHiSpinBox->setValue(0);
//       ctrlHiSpinBox->blockSignals(false);
// 
//       if(lv != ctrlLoSpinBox->value())
//       {
//         ctrlLoSpinBox->blockSignals(true);
//         ctrlLoSpinBox->setValue(lv);
//         ctrlLoSpinBox->blockSignals(false);
//       }
//       break;
//     case MusECore::MidiController::Controller14:
//     case MusECore::MidiController::RPN:
//     case MusECore::MidiController::RPN14:
//     case MusECore::MidiController::NRPN:
//     case MusECore::MidiController::NRPN14:
//       ctrlHiSpinBox->setEnabled(true);
//       ctrlLoSpinBox->setEnabled(true);
//       if(hv != ctrlHiSpinBox->value())
//       {
//         ctrlHiSpinBox->blockSignals(true);
//         ctrlHiSpinBox->setValue(hv);
//         ctrlHiSpinBox->blockSignals(false);
//       }
//       if(lv != ctrlLoSpinBox->value())
//       {
//         ctrlLoSpinBox->blockSignals(true);
//         ctrlLoSpinBox->setValue(lv);
//         ctrlLoSpinBox->blockSignals(false);
//       }
//       break;
//     default:
//       printf("FIXME: MidiAudioControl::updateCtrlBoxes: Unknown control type: %d\n", type);
//       break;
//   }
}

void ControlMapperDialog::controlsItemChanged(QTreeWidgetItem* item, int col)
{
  // REMOVE Tim.
  fprintf(stderr, "ControlMapperDialog::controlsItemChanged col:%d checkstate:%d edit:%s display:%s deco:%x\n",
          col,
          item->data(col, Qt::CheckStateRole).toInt(),
          item->data(col, Qt::EditRole).toString().toLatin1().constData(),
          //item->data(col, Qt::DisplayRole).toInt(),
          item->data(col, Qt::DisplayRole).toString().toLatin1().constData(),
          item->data(col, Qt::DecorationRole).value<QColor>().rgb());

  
//   int id = item->data(C_NAME, Qt::UserRole).toInt();
//   MusECore::iCtrlList icl = _actrls->find(id);
//   if(icl == _actrls->end())
//     fprintf(stderr, "ControlMapperDialog::controlsItemChanged audio control id not found:%d\n", id);

  switch(col)
  {
    case C_NAME:
    {
      MusECore::iCtrlList icl = _actrls->find(item->data(C_NAME, Qt::UserRole).toInt());
      if(icl != _actrls->end())
      {
        icl->second->setColor(item->data(col, Qt::DecorationRole).value<QColor>().rgb());
        int check_state = item->data(col, Qt::CheckStateRole).toInt();
        icl->second->setVisible(check_state == Qt::Checked ? true : false);
      }
    }
    break;
    
    case C_ASSIGN_PORT:
    {
      int id = item->data(C_NAME, Qt::UserRole).toInt();
      MusECore::iCtrlList icl = _actrls->find(id);
      if(icl != _actrls->end())
      {
        int new_port = item->data(C_ASSIGN_PORT, Qt::UserRole).toInt();
        MusECore::MidiAudioCtrlMap* map = _actrls->midiControls();
        int port, chan, ctl_num;
        bool is_output_port = false;
        MusECore::AudioMidiCtrlStructMap macm;
        map->find_audio_ctrl_structs(id, &macm);
        // Is there already something assigned to this audio control?
        if(macm.size())  
        {
          for(MusECore::ciAudioMidiCtrlStructMap imacm = macm.begin(); imacm != macm.end(); ++imacm)
          {
            map->hash_values((*imacm)->first, &port, &chan, &ctl_num, &is_output_port);
            if(port == new_port)
              continue;
            map->erase_ctrl_struct(port, chan, ctl_num, is_output_port, id);
            map->add_ctrl_struct(new_port, chan, ctl_num, is_output_port, MusECore::MidiAudioCtrlStruct(id));
            // TODO: Only one connection to this audio control supported for now.
            break;
          }
        }
        // No assignments found. Create a new assignment if port is valid.
        else if(new_port != -1)
        {
          ///ctl_num = ((item->data(C_MCTL_H, Qt::UserRole).toInt() & 0x7f) << 8) | (item->data(C_MCTL_L, Qt::UserRole).toInt() & 0x7f);
          ctl_num = _mctrls->findFreeController();
          chan = item->data(C_ASSIGN_CHAN, Qt::UserRole).toInt();
          if(chan < 0)
            chan = 0;
          else if(chan >= MIDI_CHANNELS)
              chan = MIDI_CHANNELS - 1;
          map->add_ctrl_struct(new_port, chan, ctl_num, is_output_port, MusECore::MidiAudioCtrlStruct(id));

          controlList->blockSignals(true);
          //item->setData(C_ASSIGN_CHAN, Qt::EditRole,    chan);
          item->setData(C_MCTL_NUM,   Qt::DisplayRole, MusECore::int2ctrlType(MusECore::MidiController::Controller7));
          item->setData(C_MCTL_NUM,   Qt::UserRole,    MusECore::MidiController::Controller7);
          //item->setData(C_MCTL_H,      Qt::EditRole,    (ctl_num >> 8) & 0x7f);
          //item->setData(C_MCTL_L,      Qt::EditRole,    ctl_num & 0x7f);
          controlList->blockSignals(false);
          
          // TODO midi controllers...
        }
        
      }
    }
    break;
      
    case C_ASSIGN_CHAN:
      break;
      
    case C_MCTL_NUM:
    {
      int id = item->data(C_NAME, Qt::UserRole).toInt();
      MusECore::iCtrlList icl = _actrls->find(id);
      if(icl != _actrls->end())
      {
        //int new_type = item->data(C_MCTL_NUM, Qt::UserRole).toInt();
        int new_ctl_num = item->data(C_MCTL_NUM, Qt::UserRole).toInt();
        MusECore::MidiAudioCtrlMap* map = _actrls->midiControls();
        int port, chan, ctl_num;
        bool is_output_port = false;
        MusECore::AudioMidiCtrlStructMap macm;
        map->find_audio_ctrl_structs(id, &macm);
        // Is there already something assigned to this audio control?
        if(macm.size())
        {
          for(MusECore::ciAudioMidiCtrlStructMap imacm = macm.begin(); imacm != macm.end(); ++imacm)
          {
            map->hash_values((*imacm)->first, &port, &chan, &ctl_num, &is_output_port);
            ///MusECore::MidiController::ControllerType type = MusECore::midiControllerType(ctl_num);
            // TODO Per-pitch controls not supported yet. Need a way to select pitch.
            ///if(type == new_type || type == MusECore::MidiController::PolyAftertouch)  
            ///  continue;
            ///map->erase_ctrl_struct(port, chan, ctl_num, is_output_port, id);
            // TODO: Only one connection to this audio control supported for now.
            ///if(new_type == -1)
            if(new_ctl_num == -1)
            {
              map->erase_ctrl_struct(port, chan, ctl_num, is_output_port, id);

              controlList->blockSignals(true);
              item->setData(C_ASSIGN_PORT, Qt::UserRole, -1);
              item->setData(C_ASSIGN_PORT, Qt::DisplayRole, QString("---"));
              //item->setData(C_ASSIGN_CHAN, Qt::EditRole,    chan);
              //item->setData(C_MCTL_TYPE,   Qt::DisplayRole, MusECore::int2ctrlType(MusECore::MidiController::Controller7));
              //item->setData(C_MCTL_TYPE,   Qt::UserRole,    MusECore::MidiController::Controller7);
              //item->setData(C_MCTL_H,      Qt::EditRole,    (ctl_num >> 8) & 0x7f);
              //item->setData(C_MCTL_L,      Qt::EditRole,    ctl_num & 0x7f);
              controlList->blockSignals(false);
              break; // Done. We just wanted to erase.
            }

            // TODO Erase and create midi controllers.
            
            ///map->add_ctrl_struct(port, chan, MusECore::midiCtrlTerms2Number((MusECore::MidiController::ControllerType)new_type, ctl_num & 0xffff), MusECore::MidiAudioCtrlStruct(id));
            map->add_ctrl_struct(port, chan, new_ctl_num, is_output_port, MusECore::MidiAudioCtrlStruct(id));
            break;
          }
        }
        // No assignments found. Create a new assignment if port is valid.
        ///else if(new_type != -1)
        else if(new_ctl_num != -1)
        {
          port = item->data(C_ASSIGN_PORT, Qt::UserRole).toInt();  // Can be -1 indicating not assigned from any input.
          if(port < -1)
            port = -1;
          else if(port >= MIDI_PORTS)
            port = MIDI_PORTS - 1;
          chan = item->data(C_ASSIGN_CHAN, Qt::UserRole).toInt();
          ///ctl_num = ((item->data(C_MCTL_H, Qt::UserRole).toInt() & 0x7f) << 8) | (item->data(C_MCTL_L, Qt::UserRole).toInt() & 0x7f);
          if(chan < 0)
            chan = 0;
          else if(chan >= MIDI_CHANNELS)
              chan = MIDI_CHANNELS - 1;
          ///map->add_ctrl_struct(port, chan, MusECore::midiCtrlTerms2Number((MusECore::MidiController::ControllerType)new_type, ctl_num & 0xffff), MusECore::MidiAudioCtrlStruct(id));
          map->add_ctrl_struct(port, chan, new_ctl_num, is_output_port, MusECore::MidiAudioCtrlStruct(id));

          //controlList->blockSignals(true);
          //item->setData(C_ASSIGN_PORT, Qt::UserRole,    port);
          //item->setData(C_ASSIGN_CHAN, Qt::EditRole,    chan);
          //item->setData(C_MCTL_TYPE,   Qt::DisplayRole, MusECore::int2ctrlType(MusECore::MidiController::Controller7));
          //item->setData(C_MCTL_TYPE,   Qt::UserRole,    MusECore::MidiController::Controller7);
          //item->setData(C_MCTL_H,      Qt::EditRole,    (ctl_num >> 8) & 0x7f);
          //item->setData(C_MCTL_L,      Qt::EditRole,    ctl_num & 0x7f);
          //controlList->blockSignals(false);

          // TODO midi controllers...
        }

      }
    }
    break;

    ///case C_MCTL_H:
    ///  break;

    ///case C_MCTL_L:
    ///  break;

    default:
      break;
  }

}

}  // namespace MusEGui
