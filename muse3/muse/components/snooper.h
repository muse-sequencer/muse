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
//#include <QTimer>
#include <QTreeWidgetItem>
#include <QMetaObject>
#include <QMetaProperty>
#include <QShowEvent>
#include <QHideEvent>
#include <QCloseEvent>

#include "ui_snooperbase.h"

namespace MusEGui {

//---------------------------------------------------------
//   SnooperTreeWidgetItem
//---------------------------------------------------------

class SnooperTreeWidgetItem : public QTreeWidgetItem
{
  public:
        enum Cols { Name = 0, Property, PropertyType, PropertyValue };
        enum ItemType { NormalItem = Type, ObjectItem = UserType, PropertiesItem = UserType + 1, PropertyItem = UserType + 2};
        enum ItemMode { NormalMode };

  private:
        QObject* _object;
        QMetaProperty _metaProperty;
        QMetaObject::Connection _metaConnection;

        void init();

  public:
        // Overrides for QTreeWidgetItem constructor...
        SnooperTreeWidgetItem(int type = NormalItem, QObject* obj = nullptr, const QMetaProperty& metaProp = QMetaProperty(),
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(type), _object(obj), _metaProperty(metaProp), _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(const QStringList& strings, int type = NormalItem,
                              QObject* obj = nullptr, const QMetaProperty& metaProp = QMetaProperty(),
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(strings, type), _object(obj), _metaProperty(metaProp),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidget* parent, int type = NormalItem, QObject* obj = nullptr,
                              const QMetaProperty& metaProp = QMetaProperty(),
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, type), _object(obj), _metaProperty(metaProp)
                              , _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidget* parent, const QStringList& strings, int type = NormalItem,
                              QObject* obj = nullptr, const QMetaProperty& metaProp = QMetaProperty(),
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, strings, type), _object(obj), _metaProperty(metaProp),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidget* parent, QTreeWidgetItem* preceding, int type = NormalItem,
                              QObject* obj = nullptr, const QMetaProperty& metaProp = QMetaProperty(),
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, preceding, type), _object(obj), _metaProperty(metaProp),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidgetItem* parent, int type = NormalItem, QObject* obj = nullptr,
                              const QMetaProperty& metaProp = QMetaProperty(),
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, type), _object(obj), _metaProperty(metaProp),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidgetItem* parent, const QStringList& strings, int type = NormalItem,
                              QObject* obj = nullptr, const QMetaProperty& metaProp = QMetaProperty(),
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, strings, type), _object(obj), _metaProperty(metaProp),
                              _metaConnection(conn) { init(); }

        SnooperTreeWidgetItem(QTreeWidgetItem* parent, QTreeWidgetItem* preceding, int type = NormalItem,
                              QObject* obj = nullptr, const QMetaProperty& metaProp = QMetaProperty(),
                              const QMetaObject::Connection& conn = QMetaObject::Connection())
                            : QTreeWidgetItem(parent, preceding, type), _object(obj), _metaProperty(metaProp),
                              _metaConnection(conn) { init(); }

        QObject* object() { return _object; }
        const QObject* cobject() const { return _object; }

        const QMetaObject::Connection& connection() const { return _metaConnection; }
        void setConnection(const QMetaObject::Connection& conn) { _metaConnection = conn; }
};

//---------------------------------------------------------
//   Snooper Dialog
//---------------------------------------------------------

class SnooperDialog : public QDialog, public Ui::SnooperDialogBase {
  
    Q_OBJECT

 protected:
      virtual void showEvent(QShowEvent*);
      virtual void hideEvent(QHideEvent*);
      virtual void closeEvent(QCloseEvent*);

  private:
      //QTimer* _updateTimer;

      bool _captureMouseClicks;
      bool _captureKeyPress;

      // Recursive!
      // Return true if anything of relevance was added ie. whether the branch should (not) be discarded.
      // If parentItem is given it adds to that item. Otherwise if null it adds as top level item.
      bool addBranch(QObject* object, SnooperTreeWidgetItem* parentItem);
      // Recursive!
      bool filterBranch(bool parentIsRelevant, QTreeWidgetItem* parentItem);
      // Recursive!
      bool destroyBranch(QObject *obj, QTreeWidgetItem* parentItem);
      // Recursive! Finds a non-hidden item.
      QTreeWidgetItem* findItem(const QObject *obj, QTreeWidgetItem* parentItem);
      const QTreeWidgetItem* cfindItem(const QObject *obj, const QTreeWidgetItem* parentItem) const;
      void disconnectAll();
      
   private slots:
     void objectDestroyed(QObject *obj = nullptr);

     void updateTree();
     void filterItems();

     void updateTreeClicked();
     void filterToggled(bool);
     void finishedLineEditing();
     void captureMouseClickToggled(bool);
     void captureKeyPressToggled(bool);

   public slots:
     void selectObject(const QObject*);
     
   public:
      SnooperDialog(QWidget* parent=0);
      virtual ~SnooperDialog();

      // Finds a non-hidden item.
      QTreeWidgetItem* findObject(const QObject* obj);
      const QTreeWidgetItem* cfindObject(const QObject* obj) const;

      bool captureMouseClicks() const { return _captureMouseClicks; }
      bool captureKeyPress() const { return _captureKeyPress; }
      };


} // namespace MusEGui

#endif  // __SNOOPER_H__

