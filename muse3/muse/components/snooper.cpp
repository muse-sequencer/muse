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
#include <QStyle>
#include <QMetaProperty>

#include "snooper.h"

// For debugging output: Uncomment the fprintf section.
#define DEBUG_SNOOPER(dev, format, args...) // fprintf(dev, format, ##args);

namespace MusEGui {

// 200ms timer interval.
const int SnooperDialog::_updateTimerInterval = 200;
// 1 seconds.
const int SnooperDialog::_autoHideTimerInterval = 1000;

QMap<int /*value*/, QString /*key*/> SnooperDialog::_eventTypeMap;

void SnooperTreeWidgetItem::init()
{
  _isParentedTopLevelBranch = false;
  _isWindowBranch = false;
  _flashCounter = 0;
  _isFlashing = false;
  _origBackground = background(Name);

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
        const QMetaObject* mo = _object->metaObject();
        const int prop_count = mo->propertyCount();
        if(_metaPropertyIndex < prop_count)
        {
          const QMetaProperty prop = mo->property(_metaPropertyIndex);
          const int prop_offset = mo->propertyOffset();
          const QString prop_str = (_metaPropertyIndex < prop_offset) ?
                        QObject::tr("<Base Property>") : QObject::tr("<Property>");
          setText(Name, prop_str);
          setText(Property, QString::fromLatin1(prop.name()));
          setText(PropertyType, QString::fromLatin1(prop.typeName()));
          setText(PropertyValue, prop.read(_object).toString());
        }
      }
    break;

    default:
    break;
  }
}

void SnooperTreeWidgetItem::startFlash(int interval, const QColor& color, const QEvent::Type& eventType)
{
  _flashCounter = interval;
  _isFlashing = true;

  setBackground(Name, color);
  if(eventType != QEvent::None)
  {
    const QString key = SnooperDialog::eventTypeString(eventType);
    setText(EventType, QString("<%1>: ").arg(eventType) + key);
  }
}

void SnooperTreeWidgetItem::resetFlash()
{
  _isFlashing = false;
  setBackground(Name, _origBackground);
  setText(EventType, QString());
}

bool SnooperTreeWidgetItem::tickFlash()
{
  if(_flashCounter <= 0)
    return false;
  if(--_flashCounter > 0)
    return false;
  resetFlash();
  return true;
}

SnooperDialog::SnooperDialog(QWidget* parent)
  : QDialog(parent, Qt::Window)
{
  setupUi(this);
  //setAttribute(Qt::WA_DeleteOnClose);
  setObjectName(QStringLiteral("snooper dialog"));

  _captureMouseClicks = captureMouseClickCheckBox->isChecked();
  _captureKeyPress = captureKeyPressCheckBox->isChecked();
  _autoHideIntervalCounter = 0;
  _flashColor = QColor(255, 170, 128);

  const QMetaObject mo = QEvent::staticMetaObject;
  const int type_idx = mo.indexOfEnumerator("Type");
  if(type_idx >= 0)
  {
    const QMetaEnum meta_enum = mo.enumerator(type_idx);
    const int key_count = meta_enum.keyCount();
    for(int k = 0; k < key_count; ++k)
      _eventTypeMap.insert(meta_enum.value(k), meta_enum.key(k));
  }

  connect(updateButton, &QPushButton::clicked, [this]() { updateTreeClicked(); } );
  connect(autoHideCheckBox, &QCheckBox::toggled, [this](bool v) { filterToggled(v); } );
  connect(onlyAppCheckBox, &QCheckBox::toggled, [this](bool v) { filterToggled(v); } );
  connect(onlyWidgetCheckBox, &QCheckBox::toggled, [this](bool v) { filterToggled(v); } );
  connect(onlyPropsCheckBox, &QCheckBox::toggled, [this](bool v) { filterToggled(v); } );
  connect(separateParentedTopLevelsCheckBox, &QCheckBox::toggled, [this](bool v) { filterToggled(v); } );
  connect(classNameLineEdit, &QLineEdit::editingFinished, [this]() { finishedLineEditing(); } );
  connect(objectNameLineEdit, &QLineEdit::editingFinished, [this]() { finishedLineEditing(); } );
  connect(captureMouseClickCheckBox, &QCheckBox::toggled, [this](bool v) { captureMouseClickToggled(v); } );
  connect(captureKeyPressCheckBox, &QCheckBox::toggled, [this](bool v) { captureKeyPressToggled(v); } );

  setFlashInterval(flashTimerValueSpinBox->value());
  flashTimerValueSpinBox->setEnabled(useFlashCheckBox->isChecked());
  resetFlashButton->setEnabled(!useFlashCheckBox->isChecked());
  // Special for this: Need qt helper overload for these lambdas.
  connect(flashTimerValueSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int v)
    { setFlashInterval(v); } );
  connect(useFlashCheckBox, &QCheckBox::toggled, [this](bool v) { useFlashTimerToggled(v); } );
  connect(resetFlashButton, &QToolButton::clicked, [this]() { resetFlashTimerClicked(); } );

  _updateTimer = new QTimer(this);
  connect(_updateTimer, &QTimer::timeout, [this]() { updateTimerTick(); } );
  _updateTimer->start(_updateTimerInterval);
}

SnooperDialog::~SnooperDialog()
{
  DEBUG_SNOOPER(stderr, "SnooperDialog::dtor\n");
  _updateTimer->stop();
  disconnectAll();
}

void SnooperDialog::disconnectAll()
{
  DEBUG_SNOOPER(stderr, "SnooperDialog::disconnectAll():\n");
  qApp->removeEventFilter(this);
  QObject* obj;
  QTreeWidgetItemIterator iObjTree(objectTree);
  while(*iObjTree)
  {
    SnooperTreeWidgetItem* item = static_cast<SnooperTreeWidgetItem*>(*iObjTree);
    obj = item->object();
    const QMetaObject::Connection& conn = item->connection();
    // NOTE QMetaObject::Connection has a boolean operator.
    if(conn && !disconnect(conn))
      fprintf(stderr, "SnooperDialog::disconnectAll(): disconnected failed: obj:%p cls_name:%s obj_name:%s\n",
              obj, obj->metaObject()->className(), obj->objectName().toLatin1().constData());
    ++iObjTree;
  }
}

bool SnooperDialog::eventFilter(QObject *obj, QEvent *event)
{
  // Pass the event on to the parent class.
  const bool ret = QDialog::eventFilter(obj, event);

  const QEvent::Type event_type = event->type();
  if(obj != this && isVisible() && !isHidden() &&
     ((event_type == QEvent::MouseButtonPress && captureMouseClicks()) ||
      (event_type == QEvent::KeyPress && captureKeyPress())))
  {
    putEventBuffer(obj, event->type());
    //selectObject(obj, event);

//     // Restart only if already at end.
//     if(_autoHideIntervalCounter <= 0)
//       _autoHideIntervalCounter = _autoHideTimerInterval / _updateTimerInterval;
  }
  return ret;
}

void SnooperDialog::putEventBuffer(QObject *obj, const QEvent::Type& eventType)
{
  HitBuffer::iterator i = _eventBuffer.find(obj);
  if(i == _eventBuffer.end())
  {
    HitMap hm;
    hm.insert(eventType, 1);
    _eventBuffer.insert(obj, hm);
  }
  else
  {
    HitMap& hm = *i;
    HitMap::iterator ihm = hm.find(eventType);
    if(ihm == hm.end())
      hm.insert(eventType, 1);
    else
      ++ihm.value();
  }
}

void SnooperDialog::showEvent(QShowEvent* e)
{
  e->ignore();
  if(!e->spontaneous())
  {
    DEBUG_SNOOPER(stderr, "SnooperDialog::showEvent(): not spontaneous\n");
    disconnectAll();
    _flashingItems.clear();
    objectTree->clear();
    updateTree();
    if(!_updateTimer->isActive())
      _updateTimer->start(_updateTimerInterval);
  }
  QDialog::showEvent(e);
}

void SnooperDialog::closeEvent(QCloseEvent* e)
{
  e->ignore();
  DEBUG_SNOOPER(stderr, "SnooperDialog::closeEvent():\n");
  _updateTimer->stop();
  disconnectAll();
  _flashingItems.clear();
  objectTree->clear();
  QDialog::closeEvent(e);
}

void SnooperDialog::hideEvent(QHideEvent* e)
{
  e->ignore();
  if(!e->spontaneous())
  {
    DEBUG_SNOOPER(stderr, "SnooperDialog::hideEvent(): not spontaneous\n");
    _updateTimer->stop();
    disconnectAll();
    _flashingItems.clear();
    objectTree->clear();
  }
  QDialog::hideEvent(e);
}

// Recursive!
bool SnooperDialog::filterBranch(bool parentIsRelevant, QTreeWidgetItem* parentItem)
{
  const QTreeWidgetItem* root_item = objectTree->invisibleRootItem();
  const bool is_parent_root_item = parentItem == root_item;

  bool is_relevant = false;
  bool this_parent_is_relevant = false;

  if(is_parent_root_item)
  {
    is_relevant = true;
  }
  else
  {
    const SnooperTreeWidgetItem* parent_snoop_item = static_cast<SnooperTreeWidgetItem*>(parentItem);
    const QObject* object = parent_snoop_item->cobject();
    const bool topLevels = separateParentedTopLevelsCheckBox->isChecked();

    // Whether to separate parented top levels.
    if(parent_snoop_item->isWindowBranch() && parent_snoop_item->isParentedTopLevelBranch() != topLevels)
    {
      if(!parentItem->isHidden())
        parentItem->setHidden(true);
      return false;
    }
    
    const QMetaObject* mo = object->metaObject();
    const QString cls_name = QString::fromLatin1(mo->className());
    const QString obj_name = object->objectName();
    const bool onlyAppClasses = onlyAppCheckBox->isChecked();
    const bool onlyWidgets = onlyWidgetCheckBox->isChecked();
    const bool onlyProps = onlyPropsCheckBox->isChecked();
    const QString search_class_name = classNameLineEdit->text();
    const QString search_obj_name = objectNameLineEdit->text();
    const int parent_item_type = parent_snoop_item->type();

    const bool search_is_relevant = parentIsRelevant ||
        ((search_class_name.isEmpty() || cls_name.contains(search_class_name)) &&
        (search_obj_name.isEmpty() || obj_name.contains(search_obj_name)));

    //if(search_is_relevant && !search_class_name.isEmpty() && !search_obj_name.isEmpty())
    if(search_is_relevant && (!search_class_name.isEmpty() || !search_obj_name.isEmpty()))
      this_parent_is_relevant = true;

    // Auto-hide takes precedence over any other filtering.
    if(autoHideCheckBox->isChecked())
    {
      if(parent_snoop_item->isFlashing())
        is_relevant = true;
    }
    else
    if((!onlyAppClasses || cls_name.startsWith(QStringLiteral("MusEGui::"))) &&
       (!onlyWidgets || object->isWidgetType()) &&
       (!onlyProps || parent_item_type == SnooperTreeWidgetItem::PropertiesItem || 
                      parent_item_type == SnooperTreeWidgetItem::PropertyItem) &&
       search_is_relevant)
    {
      is_relevant = true;
    }
  }

  SnooperTreeWidgetItem* item;
  const int child_count = parentItem->childCount();
  for(int i = 0; i < child_count; ++ i)
  {
    item = static_cast<SnooperTreeWidgetItem*>(parentItem->child(i));
    if(filterBranch(parentIsRelevant || this_parent_is_relevant, item))
      is_relevant = true;
  }

  const bool do_hide = !is_relevant && !parentIsRelevant && !this_parent_is_relevant && !is_parent_root_item;
  if(parentItem->isHidden() != do_hide)
    parentItem->setHidden(do_hide);

  return is_relevant;
}

// Recursive!
bool SnooperDialog::addBranch(QObject* object, SnooperTreeWidgetItem* parentItem,
                              bool isParentedTopLevelBranch, bool isWindowBranch)
{
  // Do NOT add anything related to THIS dialog. Hard to manage destroyed signals from itself.
  if(object == this)
    return false;

  const QTreeWidgetItem* root_item = objectTree->invisibleRootItem();
  SnooperTreeWidgetItem* item = nullptr;
  SnooperTreeWidgetItem* prop_parent_item = nullptr;
  SnooperTreeWidgetItem* prop_item = nullptr;
  const QMetaObject* mo = object->metaObject();
  const QString cls_name = QString::fromLatin1(mo->className());
  const QString obj_name = object->objectName();
  const bool is_top_item = !parentItem || parentItem == root_item;
  const bool has_parent_obj = object->parent();
  const bool is_widget_type = object->isWidgetType();

  // Whether to separate parented top levels.
  if(has_parent_obj && is_widget_type)
  {
    const QWidget* widget = qobject_cast<const QWidget*>(object);
    if(widget->isWindow())
      isWindowBranch = true;
    if(is_top_item)
      isParentedTopLevelBranch = true;
  }

  item = new SnooperTreeWidgetItem(SnooperTreeWidgetItem::ObjectItem, object);
  item->setIsParentedTopLevelBranch(isParentedTopLevelBranch);
  item->setIsWindowBranch(isWindowBranch);

  DEBUG_SNOOPER(stderr,
    "SnooperDialog::addBranch(): adding connection: obj:%p cls_name:%s obj_name:%s isParentedTopLevelBranch:%d isWindowBranch:%d\n",
    object, mo->className(), obj_name.toLatin1().constData(), isParentedTopLevelBranch, isWindowBranch);

  QMetaObject::Connection conn =
    connect(object, &QObject::destroyed, [this](QObject* o = nullptr) { objectDestroyed(o); } );
  item->setConnection(conn);

  const bool show_base_props = true;  
  const int prop_count = mo->propertyCount();
  const int prop_offset = show_base_props ? 0 : mo->propertyOffset();
  if(prop_offset < prop_count)
  {
    prop_parent_item = new SnooperTreeWidgetItem(SnooperTreeWidgetItem::PropertiesItem, object);
    prop_parent_item->setIsParentedTopLevelBranch(isParentedTopLevelBranch);
    prop_parent_item->setIsWindowBranch(isWindowBranch);
    for(int i = prop_offset; i < prop_count; ++i)
    {
      prop_item = new SnooperTreeWidgetItem(SnooperTreeWidgetItem::PropertyItem, object, i);
      prop_item->setIsParentedTopLevelBranch(isParentedTopLevelBranch);
      prop_item->setIsWindowBranch(isWindowBranch);
      prop_parent_item->addChild(prop_item);
    }
    item->addChild(prop_parent_item);
  }

  const QObjectList& ol = object->children();
  foreach(QObject* obj, ol)
    addBranch(obj, item, isParentedTopLevelBranch, isWindowBranch);

  if(parentItem)
    parentItem->addChild(item);
  else
    objectTree->addTopLevelItem(item);

  return true;
}

void SnooperDialog::updateTree()
{
  _updateTimer->stop();
  disconnectAll();
  _flashingItems.clear();
  objectTree->clear();

//   const QObjectList& cl = qApp->children();
//   foreach(QObject* obj, cl)
//    addBranch(obj, nullptr);
  const QWidgetList wl = qApp->topLevelWidgets();
  foreach(QWidget* obj, wl)
    addBranch(obj, nullptr, false, false);

  qApp->installEventFilter(this);

  filterItems();
  objectTree->resizeColumnToContents(SnooperTreeWidgetItem::Name);
  _updateTimer->start(_updateTimerInterval);
}

// Recursive!
bool SnooperDialog::destroyBranch(QObject *obj, QTreeWidgetItem* parentItem, bool deleteBranchPending)
{
  bool delete_this_branch = false;
  if(parentItem != objectTree->invisibleRootItem())
  {
    SnooperTreeWidgetItem* snoop_item = static_cast<SnooperTreeWidgetItem*>(parentItem);
    if(snoop_item->object() == obj)
    {
      // Mark this branch for deletion, and this item ONLY if it initiated the deletion.
      if(!deleteBranchPending)
      {
        deleteBranchPending = true;
        delete_this_branch = true;
      }
    }

    // Remove the item from the flashing list, if it's there.
    if(deleteBranchPending)
      _flashingItems.remove(snoop_item);
  }

  // Do it in reverse!
  const int sz = parentItem->childCount();
  for(int i = sz - 1; i >= 0; --i)
    destroyBranch(obj, parentItem->child(i), deleteBranchPending);

  if(delete_this_branch)
    delete parentItem;
    
  return true;
}

void SnooperDialog::objectDestroyed(QObject *obj)
{
  DEBUG_SNOOPER(stderr, "SnooperDialog::objectDestroyed(): obj:%p\n", obj);

  if(!isVisible())
    fprintf(stderr, "SnooperDialog::objectDestroyed(): Got objectDestroyed while Snooper is not visible! obj:%p\n", obj);

  // Enter the 'root branch'.
  destroyBranch(obj, objectTree->invisibleRootItem(), false);
}

// Recursive!
QTreeWidgetItem* SnooperDialog::findItem(const QObject *obj, QTreeWidgetItem* parentItem,
                                         bool noHidden, bool parentedTopLevels)
{
  if(noHidden && parentItem->isHidden())
    return nullptr;

  if(parentItem != objectTree->invisibleRootItem())
  {
    const SnooperTreeWidgetItem* snoop_item = static_cast<const SnooperTreeWidgetItem*>(parentItem);
    if(snoop_item->cobject() == obj &&
      (!snoop_item->isWindowBranch() || snoop_item->isParentedTopLevelBranch() == parentedTopLevels))
      return parentItem;
  }
  QTreeWidgetItem *item;
  const int sz = parentItem->childCount();
  for(int i = 0; i < sz; ++i)
  {
    item = findItem(obj, parentItem->child(i), noHidden, parentedTopLevels);
    if(item)
      return item;
  }
  return nullptr;
}

// Recursive!
const QTreeWidgetItem* SnooperDialog::cfindItem(const QObject *obj, const QTreeWidgetItem* parentItem,
                                                bool noHidden, bool parentedTopLevels) const
{
  if(noHidden && parentItem->isHidden())
    return nullptr;

  if(parentItem != objectTree->invisibleRootItem())
  {
    const SnooperTreeWidgetItem* snoop_item = static_cast<const SnooperTreeWidgetItem*>(parentItem);
    if(snoop_item->cobject() == obj &&
       //snoop_item->isParentedTopLevelBranch() == parentedTopLevels)
       (!snoop_item->isWindowBranch() || snoop_item->isParentedTopLevelBranch() == parentedTopLevels))
      return parentItem;
  }
  const QTreeWidgetItem *item;
  const int sz = parentItem->childCount();
  for(int i = 0; i < sz; ++i)
  {
    item = cfindItem(obj, parentItem->child(i), noHidden, parentedTopLevels);
    if(item)
      return item;
  }
  return nullptr;
}

QTreeWidgetItem* SnooperDialog::findObject(const QObject* obj,
                                           bool noHidden, bool parentedTopLevels)
{
  // Enter the 'root branch'.
  return findItem(obj, objectTree->invisibleRootItem(), noHidden, parentedTopLevels);
}

const QTreeWidgetItem* SnooperDialog::cfindObject(const QObject* obj,
                                                  bool noHidden, bool parentedTopLevels) const
{
  // Enter the 'root branch'.
  return cfindItem(obj, objectTree->invisibleRootItem(), noHidden, parentedTopLevels);
}

void SnooperDialog::filterItems()
{
  // Enter the 'root branch'.
  // Top relevant is false here to keep the routine happy.
  const bool this_parent_is_relevant = false;
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

void SnooperDialog::useFlashTimerToggled(bool v)
{
  flashTimerValueSpinBox->setEnabled(v);
  resetFlashButton->setEnabled(!v);
}

void SnooperDialog::finishedLineEditing()
{
  filterItems();
  objectTree->resizeColumnToContents(SnooperTreeWidgetItem::Name);
}

void SnooperDialog::captureMouseClickToggled(bool v)
{
  _captureMouseClicks = v;
}

void SnooperDialog::captureKeyPressToggled(bool v)
{
  _captureKeyPress = v;
}

void SnooperDialog::resetFlashTimerClicked()
{
  if(_flashingItems.isEmpty())
    return;
  for(QSet<SnooperTreeWidgetItem *>::iterator i = _flashingItems.begin(); i != _flashingItems.end(); ++i)
    (*i)->resetFlash();
  _flashingItems.clear();
}

SnooperTreeWidgetItem* SnooperDialog::selectObject(const QObject *obj, const QEvent::Type& eventType)
{
//   if((eventType != QEvent::MouseButtonPress || !captureMouseClicks()) &&
//      (eventType != QEvent::KeyPress || !captureKeyPress()))
//     return;

  QTreeWidgetItem* item = findObject(obj, !autoHideCheckBox->isChecked(),
                                     separateParentedTopLevelsCheckBox->isChecked());
  if(!item)
  {
    // Careful, crash might happen here. Object might not exist?
    //fprintf(stderr, "SnooperDialog::selectObject() Did not find class name:%s object name:%s\n",
    //        obj->metaObject()->className(), obj->objectName().toLatin1().constData());
    //fprintf(stderr, "SnooperDialog::selectObject() Did not find object:%p\n", obj);
    return nullptr;
  }

  SnooperTreeWidgetItem* snoop_item = static_cast<SnooperTreeWidgetItem*>(item);
  //_autoHideIntervalCounter = _autoHideTimerInterval / _updateTimerInterval;
  snoop_item->startFlash(_flashInterval, _flashColor, eventType);
  _flashingItems.insert(snoop_item);
  return snoop_item;
}

SnooperTreeWidgetItem* SnooperDialog::processEventBuffer()
{
  if(_eventBuffer.isEmpty())
    return nullptr;

  // Suppose we should warn if things are getting busy.
  if(_eventBuffer.size() >= 32768)
    fprintf(stderr, "SnooperDialog::processEventBuffer(): Warning: Event buffer size >= 32768. Quite busy?\n");

  SnooperTreeWidgetItem* first_item = nullptr;
  SnooperTreeWidgetItem* item;
  QObject *obj;
  for(HitBuffer::iterator i = _eventBuffer.begin(); i != _eventBuffer.end(); ++i)
  {
    obj = i.key();
    HitMap& hm = *i;
    for(HitMap::iterator k = hm.begin(); k != hm.end(); ++k)
    {
      const QEvent::Type& event_type = k.key();
      item = selectObject(obj, event_type);
      if(item && !first_item)
        first_item = item;
    }
  }
  // Done with buffer. Clear it.
  _eventBuffer.clear();
  return first_item;
}

void SnooperDialog::updateTimerTick()
{
  // Avoid repeated filtering. Do it periodically.
  //if(_autoHideIntervalCounter > 0)
  {
    if(_autoHideIntervalCounter <= 0 || --_autoHideIntervalCounter <= 0)
    {
      SnooperTreeWidgetItem* item = processEventBuffer();
      if(autoHideCheckBox->isChecked())
      {
        //fprintf(stderr, "SnooperDialog::updateTimerTick(): Resetting auto-hide timer\n");
        _autoHideIntervalCounter = _autoHideTimerInterval / _updateTimerInterval;
        filterItems();
      }
      //item->setSelected(true);
      //objectTree->setCurrentItem(item);
      objectTree->scrollToItem(item);
      objectTree->resizeColumnToContents(SnooperTreeWidgetItem::Name);
    }
    // Don't tick the flashers until the auto-hide countdown is done.
    //return;
  }

  if(_flashingItems.isEmpty() || !useFlashCheckBox->isChecked())
    return;
  SnooperTreeWidgetItem* item;
  for(QSet<SnooperTreeWidgetItem *>::iterator i = _flashingItems.begin(); i != _flashingItems.end(); )
  {
    item = *i;
    if(item->tickFlash())
      i = _flashingItems.erase(i);
    else
      ++i;
  }
}

// Static
QString SnooperDialog::eventTypeString(const QEvent::Type& eventType)
{
  if(eventType != QEvent::None)
  {
    QString key;
    QMap<int /*value*/, QString /*key*/>::const_iterator iet = _eventTypeMap.constFind(eventType);
    if(iet != _eventTypeMap.constEnd())
      return *iet;
  }
  return QString();
}

} // namespace MusEGui
