//=========================================================
//  MusE
//  Linux Music Editor
//
//  snooper.h
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

#ifndef __SNOOPER_H__
#define __SNOOPER_H__

#include <QWidget>
#include <QDialog>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QMetaObject>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>
#include <QMap>
#include <QSet>
#include <QColor>

#include "ui_snooperbase.h"

namespace MusEGui {

//---------------------------------------------------------
//   SnooperTreeWidgetItem
//---------------------------------------------------------

class SnooperTreeWidgetItem : public QTreeWidgetItem
{
  public:
        enum Cols { Name = 0, Property, PropertyType, PropertyValue, EventType };
        enum ItemType {
          NormalItem     = Type,
          ObjectItem     = UserType,
          PropertiesItem = ObjectItem + 1,
          PropertyItem   = PropertiesItem + 1
        };
        enum ItemMode { NormalMode };

  private:
        QObject* _object;
        bool _isWindowBranch;
        bool _isParentedTopLevelBranch;
        int _metaPropertyIndex;
        QMetaObject::Connection _metaConnection;
        //QMap<int /*type*/, int /*hit_count*/> _hitMap;
        QBrush _origBackground;
        int _flashCounter;
        bool _isFlashing;

        void init();

  public:
        // Overrides for QTreeWidgetItem constructor...
        SnooperTreeWidgetItem(int type = NormalItem, QObject* obj = nullptr, int metaPropertyIndex = 0,
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(type), _object(obj), _metaPropertyIndex(metaPropertyIndex),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(const QStringList& strings, int type = NormalItem,
                              QObject* obj = nullptr, int metaPropertyIndex = 0,
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(strings, type), _object(obj), _metaPropertyIndex(metaPropertyIndex),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidget* parent, int type = NormalItem, QObject* obj = nullptr,
                              int metaPropertyIndex = 0,
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, type), _object(obj), _metaPropertyIndex(metaPropertyIndex), 
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidget* parent, const QStringList& strings, int type = NormalItem,
                              QObject* obj = nullptr, int metaPropertyIndex = 0,
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, strings, type), _object(obj), _metaPropertyIndex(metaPropertyIndex),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidget* parent, QTreeWidgetItem* preceding, int type = NormalItem,
                              QObject* obj = nullptr, int metaPropertyIndex = 0,
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, preceding, type), _object(obj), _metaPropertyIndex(metaPropertyIndex),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidgetItem* parent, int type = NormalItem, QObject* obj = nullptr,
                              int metaPropertyIndex = 0,
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, type), _object(obj), _metaPropertyIndex(metaPropertyIndex),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidgetItem* parent, const QStringList& strings, int type = NormalItem,
                              QObject* obj = nullptr, int metaPropertyIndex = 0,
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, strings, type), _object(obj), _metaPropertyIndex(metaPropertyIndex),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidgetItem* parent, QTreeWidgetItem* preceding, int type = NormalItem,
                              QObject* obj = nullptr, int metaPropertyIndex = 0,
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, preceding, type), _object(obj), _metaPropertyIndex(metaPropertyIndex),
                              _metaConnection(conn) { init(); }

        QObject* object() { return _object; }
        const QObject* cobject() const { return _object; }
        
        bool isParentedTopLevelBranch() const { return _isParentedTopLevelBranch; }
        void setIsParentedTopLevelBranch(bool v) { _isParentedTopLevelBranch = v; }
        bool isWindowBranch() const { return _isWindowBranch; }
        void setIsWindowBranch(bool v) { _isWindowBranch = v; }

        const QMetaObject::Connection& connection() const { return _metaConnection; }
        void setConnection(const QMetaObject::Connection& conn) { _metaConnection = conn; }

        void startFlash(int interval, const QColor& color, const QEvent::Type& eventType = QEvent::None);
        bool isFlashing() const { return _isFlashing; }
        // Driven from timer/divider. Returns true if the end was reached.
        bool tickFlash();
        void resetFlash();
};

//---------------------------------------------------------
//   Snooper Dialog
//---------------------------------------------------------

class SnooperDialog : public QDialog, public Ui::SnooperDialogBase {
  
    Q_OBJECT

      // An extra property required to support stylesheets (not enough colours).
      Q_PROPERTY(QColor flashColor READ flashColor WRITE setFlashColor)

  public:
      typedef QMap<QEvent::Type /*event_type*/, int /*hit_count*/> HitMap;
      typedef QMap<QObject* /*object*/, HitMap> HitBuffer;

  protected:
      void showEvent(QShowEvent*) override;
      void hideEvent(QHideEvent*) override;
      void closeEvent(QCloseEvent*) override;
      bool eventFilter(QObject*, QEvent*) override;
      
  private:
      static QMap<int /*value*/, QString /*key*/> _eventTypeMap;
      // In milliseconds.
      static const int _updateTimerInterval;
      QTimer* _updateTimer;
      int _flashInterval;
      QColor _flashColor;
      // In milliseconds.
      static const int _autoHideTimerInterval;
      int _autoHideIntervalCounter;

      HitBuffer _eventBuffer;
      void putEventBuffer(QObject *obj, const QEvent::Type& eventType);
      // Also returns the first item processed (so it can be scrolled to).
      SnooperTreeWidgetItem* processEventBuffer();

      bool _captureMouseClicks;
      bool _captureKeyPress;

      QSet<SnooperTreeWidgetItem*> _flashingItems;

      // Recursive!
      // Return true if anything of relevance was added ie. whether the branch should (not) be discarded.
      // If parentItem is given it adds to that item. Otherwise if null it adds as top level item.
      bool addBranch(QObject* object, SnooperTreeWidgetItem* parentItem,
                     bool isParentedTopLevelBranch, bool isWindowBranch);
      // Recursive!
      bool filterBranch(bool parentIsRelevant, QTreeWidgetItem* parentItem);
      // Recursive!
      bool destroyBranch(QObject *obj, QTreeWidgetItem* parentItem, bool deleteBranchPending);
      // Recursive! Finds a non-hidden item.
      QTreeWidgetItem* findItem(const QObject *obj, QTreeWidgetItem* parentItem,
                                bool noHidden, bool parentedTopLevels);
      const QTreeWidgetItem* cfindItem(const QObject *obj, const QTreeWidgetItem* parentItem,
                                       bool noHidden, bool parentedTopLevels) const;
      SnooperTreeWidgetItem* selectObject(const QObject *obj, const QEvent::Type& eventType = QEvent::None);
      void disconnectAll();
      
   private slots:
     void objectDestroyed(QObject *obj = nullptr);

     void updateTimerTick();

     void updateTree();
     void filterItems();

     void updateTreeClicked();
     void filterToggled(bool);
     void finishedLineEditing();
     void captureMouseClickToggled(bool);
     void captureKeyPressToggled(bool);
     void useFlashTimerToggled(bool);
     void resetFlashTimerClicked();

   public:
      SnooperDialog(QWidget* parent=0);
      virtual ~SnooperDialog();

      static QString eventTypeString(const QEvent::Type& eventType);

      // Finds a non-hidden item.
      QTreeWidgetItem* findObject(const QObject* obj, bool noHidden, bool parentedTopLevels);
      const QTreeWidgetItem* cfindObject(const QObject* obj, bool noHidden, bool parentedTopLevels) const;

      bool captureMouseClicks() const { return _captureMouseClicks; }
      bool captureKeyPress() const { return _captureKeyPress; }

      void setFlashInterval(int val) { _flashInterval = (1000 * val) / _updateTimerInterval; }
      
      QColor flashColor() const { return _flashColor; }
      void setFlashColor(const QColor& c) { _flashColor = c; }
      };


} // namespace MusEGui

#endif  // __SNOOPER_H__

