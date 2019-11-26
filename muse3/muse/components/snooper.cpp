//=========================================================
//  MusE
//  Linux Music Editor
//
//  snooper.cpp
//  (C) Copyright 2019 Tim E. Real (terminator356 on sourceforge)
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

#include <QApplication>

#include "snooper.h"

namespace MusEGui {

void SnooperTreeWidgetItem::init()
{
  switch(type())
  {
    case ObjectItem:
      if(_object)
      {
        const QMetaObject* mo = _object->metaObject();
        const QString cls_name = QString::fromLatin1(mo->className());
        const QString obj_name = _object->objectName();
        setText(Name, cls_name + QStringLiteral("::") + obj_name);
      }
    break;
    
    case PropertiesItem:
      setText(Name, QObject::tr("<Properties>"));
    break;

    case PropertyItem:
      if(_object)
      {
        setText(Name, QObject::tr("<Property>"));
        setText(Property, QString::fromLatin1(_metaProperty.name()));
        setText(PropertyType, QString::fromLatin1(_metaProperty.typeName()));
        setText(PropertyValue, _metaProperty.read(_object).toString());
      }
    break;

    default:
    break;
  }
}


SnooperDialog::SnooperDialog(QWidget* parent)
  : QDialog(parent, Qt::Window)
{
  setupUi(this);
  //setAttribute(Qt::WA_DeleteOnClose);
  setObjectName(QStringLiteral("snooper dialog"));
  connect(updateButton, &QPushButton::clicked, [this]() { updateTreeClicked(); } );
  connect(onlyAppCheckBox, &QCheckBox::toggled, [this](bool v) { filterToggled(v); } );
  connect(onlyWidgetCheckBox, &QCheckBox::toggled, [this](bool v) { filterToggled(v); } );
  connect(onlyPropsCheckBox, &QCheckBox::toggled, [this](bool v) { filterToggled(v); } );
  connect(separateParentedTopLevelsCheckBox, &QCheckBox::toggled, [this](bool v) { filterToggled(v); } );
  connect(classNameLineEdit, &QLineEdit::editingFinished, [this]() { finishedLineEditing(); } );
  connect(objectNameLineEdit, &QLineEdit::editingFinished, [this]() { finishedLineEditing(); } );
}

SnooperDialog::~SnooperDialog()
{
  fprintf(stderr, "SnooperDialog::dtor\n");
  disconnectAll();
}

void SnooperDialog::disconnectAll()
{
  fprintf(stderr, "SnooperDialog::disconnectAll():\n");
  const QObject* obj;
  QTreeWidgetItemIterator iObjTree(objectTree);
  while(*iObjTree)
  {
    const SnooperTreeWidgetItem* item = static_cast<const SnooperTreeWidgetItem*>(*iObjTree);
    obj = item->cobject();
    const QMetaObject* mo = obj->metaObject();
    const QString cls_name = QString::fromLatin1(mo->className());
    const QString obj_name = obj->objectName();
    const QMetaObject::Connection& conn = item->connection();
    // NOTE QMetaObject::Connection has a boolean operator.
    if(conn && !disconnect(conn))
      fprintf(stderr, "SnooperDialog::disconnectAll(): disconnected failed: obj:%p cls_name:%s obj_name:%s\n",
              obj, mo->className(), obj_name.toLatin1().constData());
    ++iObjTree;
  }
}

void SnooperDialog::showEvent(QShowEvent* e)
{
  e->ignore();
  if(!e->spontaneous())
  {
    fprintf(stderr, "SnooperDialog::showEvent(): not spontaneous\n");
    disconnectAll();
    objectTree->clear();
    updateTree();
  }
  QDialog::showEvent(e);
}

void SnooperDialog::closeEvent(QCloseEvent* e)
{
  e->ignore();
  fprintf(stderr, "SnooperDialog::closeEvent():\n");
  disconnectAll();
  objectTree->clear();
  QDialog::closeEvent(e);
}

void SnooperDialog::hideEvent(QHideEvent* e)
{
  e->ignore();
  if(!e->spontaneous())
  {
    fprintf(stderr, "SnooperDialog::hideEvent(): not spontaneous\n");
    disconnectAll();
    objectTree->clear();
  }
  QDialog::hideEvent(e);
}

// Recursive!
bool SnooperDialog::filterBranch(bool parentIsRelevant, QTreeWidgetItem* parentItem)
{
  const QTreeWidgetItem* root_item = objectTree->invisibleRootItem();

  bool is_relevant = false;
  bool this_parent_is_relevant = false;

  if(parentItem == root_item)
  {
    is_relevant = true;
  }
  else
  {
    const SnooperTreeWidgetItem* parent_snoop_item = static_cast<SnooperTreeWidgetItem*>(parentItem);
    const QObject* object = parent_snoop_item->cobject();
    const bool is_top_item = !parentItem->parent() || parentItem->parent() == root_item;
    const bool has_parent_obj = object->parent();
    const bool is_widget_type = object->isWidgetType();
  
    const bool topLevels = separateParentedTopLevelsCheckBox->isChecked();

    // Whether to separate parented top levels.
    if(has_parent_obj && is_widget_type)
    {
      const QWidget* widget = qobject_cast<const QWidget*>(object);
      if(widget->isWindow())
      {
        if((topLevels && !is_top_item) || (!topLevels && is_top_item))
        {
          if(!parentItem->isHidden())
            parentItem->setHidden(true);
          return false;
        }
      }
    }

    const QMetaObject* mo = object->metaObject();
    const QString cls_name = QString::fromLatin1(mo->className());
    const QString obj_name = object->objectName();
    const bool onlyAppClasses = onlyAppCheckBox->isChecked();
    const bool onlyWidgets = onlyWidgetCheckBox->isChecked();
    const bool onlyProps = onlyPropsCheckBox->isChecked();
    const QString search_class_name = classNameLineEdit->text();
    const QString search_obj_name = objectNameLineEdit->text();
    const int parent_item_type = parentItem->type();

    const bool search_is_relevant =
        (search_class_name.isEmpty() || cls_name.contains(search_class_name)) &&
        (search_obj_name.isEmpty() || obj_name.contains(search_obj_name));

    if((!onlyAppClasses || cls_name.startsWith(QStringLiteral("MusEGui::"))) &&
       (!onlyWidgets || object->isWidgetType()) &&
       (!onlyProps || parent_item_type == SnooperTreeWidgetItem::PropertiesItem || 
                      parent_item_type == SnooperTreeWidgetItem::PropertyItem) &&
       search_is_relevant)
    {
      is_relevant = true;
    }

    if(search_is_relevant)
      this_parent_is_relevant = true;
  }

  SnooperTreeWidgetItem* item;
  const int child_count = parentItem->childCount();
  for(int i = 0; i < child_count; ++ i)
  {
    item = static_cast<SnooperTreeWidgetItem*>(parentItem->child(i));
    if(filterBranch(this_parent_is_relevant, item))
      is_relevant = true;
  }

  const bool do_hide = !is_relevant && !parentIsRelevant;
  if(parentItem->isHidden() != do_hide)
    parentItem->setHidden(do_hide);

  return is_relevant;
}

// Recursive!
bool SnooperDialog::addBranch(QObject* object, SnooperTreeWidgetItem* parentItem)
{
  // Do NOT add anything related to THIS dialog. Hard to manage destroyed signals from itself.
  if(object == this)
    return false;

  SnooperTreeWidgetItem* item = nullptr;
  SnooperTreeWidgetItem* prop_parent_item = nullptr;
  SnooperTreeWidgetItem* prop_item = nullptr;
  const QMetaObject* mo = object->metaObject();
  const QString cls_name = QString::fromLatin1(mo->className());
  const QString obj_name = object->objectName();

  item = new SnooperTreeWidgetItem(SnooperTreeWidgetItem::ObjectItem, object);

  //fprintf(stderr, "SnooperDialog::addBranch(): adding connection: obj:%p cls_name:%s obj_name:%s\n",
  //        object, mo->className(), obj_name.toLatin1().constData());

  QMetaObject::Connection conn =
    connect(object, &QObject::destroyed, [this](QObject* o = nullptr) { objectDestroyed(o); } );

  item->setConnection(conn);

  const int prop_count = mo->propertyCount();
  const int prop_offset = mo->propertyOffset();
  if(prop_count > prop_offset)
  {
    prop_parent_item = new SnooperTreeWidgetItem(SnooperTreeWidgetItem::PropertiesItem, object);
    for(int i = prop_offset; i < prop_count; ++i)
    {
      prop_item = new SnooperTreeWidgetItem(SnooperTreeWidgetItem::PropertyItem, object, mo->property(i));
      prop_parent_item->addChild(prop_item);
    }
    item->addChild(prop_parent_item);
  }

  const QObjectList& ol = object->children();
  foreach(QObject* obj, ol)
    addBranch(obj, item);

  if(parentItem)
    parentItem->addChild(item);
  else
    objectTree->addTopLevelItem(item);

  return true;
}

void SnooperDialog::updateTree()
{
  disconnectAll();
  objectTree->clear();

//   const QObjectList& cl = qApp->children();
//   foreach(QObject* obj, cl)
//    addBranch(obj, nullptr);
  const QWidgetList wl = qApp->topLevelWidgets();
  foreach(QWidget* obj, wl)
    addBranch(obj, nullptr);

  filterItems();
  objectTree->resizeColumnToContents(SnooperTreeWidgetItem::Name);
}

bool SnooperDialog::destroyBranch(QObject *obj, QTreeWidgetItem* parentItem)
{
  if(parentItem != objectTree->invisibleRootItem())
  {
    if(static_cast<SnooperTreeWidgetItem*>(parentItem)->object() == obj)
    {
      // Delete the branch.
      delete parentItem;
      return false;
    }
  }

  // Do it in reverse!
  const int sz = parentItem->childCount();
  for(int i = sz - 1; i >= 0; --i)
    destroyBranch(obj, parentItem->child(i));

  return true;
}

void SnooperDialog::objectDestroyed(QObject *obj)
{
  //fprintf(stderr, "SnooperDialog::objectDestroyed(): obj:%p\n", obj);

  if(!isVisible())
    fprintf(stderr, "SnooperDialog::objectDestroyed(): Got objectDestroyed while Snooper is not visible! obj:%p\n", obj);

  // Enter the 'root branch'.
  destroyBranch(obj, objectTree->invisibleRootItem());
}

const QTreeWidgetItem* SnooperDialog::cfindItem(const QObject* obj) const
{
  const QTreeWidgetItem* root_item = objectTree->invisibleRootItem();
  const QTreeWidgetItem* item;
  const SnooperTreeWidgetItem* snoop_item;
  QTreeWidgetItemIterator iObjTree(objectTree);
  while(*iObjTree)
  {
    item = *iObjTree;
    if(item != root_item && !item->isHidden())
    {
      snoop_item = static_cast<const SnooperTreeWidgetItem*>(item);
      if(snoop_item->cobject() == obj)
        return snoop_item;
    }
    ++iObjTree;
  }
  return nullptr;
}

QTreeWidgetItem* SnooperDialog::findItem(const QObject* obj)
{
  const QTreeWidgetItem* root_item = objectTree->invisibleRootItem();
  QTreeWidgetItem* item;
  SnooperTreeWidgetItem* snoop_item;
  QTreeWidgetItemIterator iObjTree(objectTree);
  while(*iObjTree)
  {
    item = *iObjTree;
    if(item != root_item && !item->isHidden())
    {
      snoop_item = static_cast<SnooperTreeWidgetItem*>(item);
      if(snoop_item->cobject() == obj)
        return snoop_item;
    }
    ++iObjTree;
  }
  return nullptr;
}

void SnooperDialog::filterItems()
{
  // Enter the 'root branch'.
  const bool this_parent_is_relevant = true;
  filterBranch(this_parent_is_relevant, objectTree->invisibleRootItem());
}

void SnooperDialog::updateTreeClicked()
{
  updateTree();
}

void SnooperDialog::filterToggled(bool)
{
  filterItems();
  objectTree->resizeColumnToContents(SnooperTreeWidgetItem::Name);
}

void SnooperDialog::finishedLineEditing()
{
  filterItems();
  objectTree->resizeColumnToContents(SnooperTreeWidgetItem::Name);
}

void SnooperDialog::selectObject(const QObject* obj)
{
  QTreeWidgetItem* item = findItem(obj);
  if(!item)
    return;
  //item->setSelected(true);
  objectTree->setCurrentItem(item);
  objectTree->scrollToItem(item);
}

} // namespace MusEGui
