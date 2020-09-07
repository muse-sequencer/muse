//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: lcombo.cpp,v 1.1.1.1.2.3 2009/07/01 22:14:56 spamatica Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
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

#include "lcombo.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QAbstractItemView>
#include <QString>
#include <QModelIndex>

namespace MusEGui {

//---------------------------------------------------------
//   LabelCombo
//---------------------------------------------------------

LabelCombo::LabelCombo(const QString& txt, QWidget* parent,
   const char* name) : QWidget(parent)
      {
      setObjectName(name);
      QHBoxLayout* layout = new QHBoxLayout(this);
      QLabel* label = new QLabel(txt, this);
      //label->setContentsMargins(0,0,0,0);                                               // REMOVE Tim. Or keep.
      box = new QComboBox(this);
      //box->setContentsMargins(0,0,0,0);                                                 //
      // Ignored was only solution, others were too tall. The label takes priority then.  //
      //box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);                 //
      box->setEditable(false);
      layout->addSpacing(2);
      layout->addWidget(label);
//      layout->addSpacing(2);
      layout->addWidget(box);
//      layout->addSpacing(2);
      layout->setContentsMargins(0, 0, 0, 0);
      connect(box, QOverload<int>::of(&QComboBox::activated), [=](int index) { box_activated(index); } );
      }

void LabelCombo::addItem(const QString& txt, const QVariant &userData)
{ 
  box->addItem(txt, userData);
}

void LabelCombo::insertItem(int index, const QString& txt, const QVariant &userData)
{
  box->insertItem(index, txt, userData); 
}

QAbstractItemView *LabelCombo::view() const
{
  return box->view();
}

void LabelCombo::setView(QAbstractItemView* v)
{
  box->setModel(v->model());
  box->setView(v);
}

void LabelCombo::setFocusPolicy (Qt::FocusPolicy fp)
{ 
  box->setFocusPolicy(fp);
}

void LabelCombo::box_activated(int idx)
{
  // HACK: Force the thing to show the right item. This hack is required because
  //        if we are trying to show a table view in a combo box it normally wants
  //        to show a single given column using a list view.
  const QAbstractItemView* iv = view();
  if(!iv)
    return;
  const QModelIndex mdl_idx = iv->currentIndex();
  if(!mdl_idx.isValid())
    return;
  const int row = mdl_idx.row();
  const int col = mdl_idx.column();
  blockSignals(true);
  if(box->modelColumn() != col)
    box->setModelColumn(col);
  if(box->currentIndex() != row)
    box->setCurrentIndex(row); 
  blockSignals(false);

  emit activated(idx);
  emit activated(mdl_idx);
}

void LabelCombo::clearFocus()
{
  box->clearFocus();
}

QVariant LabelCombo::itemData(int index, int role) const
{
  return box->itemData(index, role);
}

int LabelCombo::findData(const QVariant &data, int role, Qt::MatchFlags flags) const
{
  return box->findData(data, role, flags);
}

int LabelCombo::maxVisibleItems() const
{
  return box->maxVisibleItems();
}

void LabelCombo::setMaxVisibleItems(int maxItems)
{
  box->setMaxVisibleItems(maxItems);
}

QComboBox::SizeAdjustPolicy LabelCombo::sizeAdjustPolicy() const
{
  return box->sizeAdjustPolicy();
}

void LabelCombo::setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy policy)
{
  box->setSizeAdjustPolicy(policy);
}

int LabelCombo::currentIndex() const
{
  return box->currentIndex();
}

QModelIndex LabelCombo::currentModelIndex() const
{
  return view()->currentIndex();
}

void LabelCombo::setCurrentIndex(int i) 
{ 
  // HACK: Force the thing to show the right item. This hack is required because
  //        if we are trying to show a table view in a combo box it normally wants
  //        to show a single given column using a list view.
  int rc = box->model()->rowCount();
  if(rc == 0)
    return;
  int r = i % rc;
  int c = i / rc;
  if(c >= box->model()->columnCount())
    return;
  if(box->modelColumn() != c)
    box->setModelColumn(c);
  if(box->currentIndex() != r)  
    box->setCurrentIndex(r); 
} 

void LabelCombo::setCurrentModelIndex(const QModelIndex& mdl_idx)
{
  const int row = mdl_idx.row();
  const int col = mdl_idx.column();
  if(col >= box->model()->columnCount())
    return;
  if(box->modelColumn() != col)
    box->setModelColumn(col);
  if(box->currentIndex() != row)  
    box->setCurrentIndex(row); 
}

} // namespace MusEGui
