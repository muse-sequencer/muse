//=========================================================
//  MusE
//  Linux Music Editor
//
//  control_mapper.h
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
#ifndef __CONTROL_MAPPER_H__
#define __CONTROL_MAPPER_H__

#include "ui_control_mapper_base.h"

#include "track.h"
#include "ctrl.h"
#include "midictrl.h"
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QWidget>

class QPaintEvent;
class QColor;
class QSize;
class QMouseEvent;
class QStyle;
class QSpinBox;
class QHBoxLayout;
class QPixmap;

namespace MusEGui {

class ColorListEditor : public QComboBox
{
  Q_OBJECT
  Q_PROPERTY(QColor color READ color WRITE setColor USER true)

  public:
    ColorListEditor(QWidget *widget = nullptr);

  public:
    QColor color() const;
    void setColor(QColor c);

  private:
    void populateList();
};

class MidiCtrlDelegateEditor : public QWidget
{
  Q_OBJECT
  //Q_PROPERTY(int color READ color WRITE setColor USER true)

  private:
    QComboBox* _typeCombo;
    QSpinBox*  _numHiSpinBox;
    QSpinBox*  _numLoSpinBox;
    QWidget*   _validIndicator;
    QHBoxLayout* _hlayout;

  protected:
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    //bool event(QEvent*);
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

  signals:
    void activated(int ctrlNum);

  public:
    MidiCtrlDelegateEditor(QWidget* parent = nullptr);
    virtual ~MidiCtrlDelegateEditor();
    int  controlNum() const;
    void setControlNum(int ctrlNum);
};

class ColorChooserEditor : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QColor color READ color WRITE setColor USER true)

  private:
    QColor _color;

  protected:
    void paintEvent(QPaintEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    //bool event(QEvent*);
    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    
  signals:
    void activated(const QColor& color);
    
  public:
    static QColor colorChooserList[];

    ColorChooserEditor(QWidget* parent = nullptr);
    virtual ~ColorChooserEditor();
    QColor color() const;
    void setColor(QColor c);
};

//-----------------------------------
//   MapperAssignDelegate
//-----------------------------------

class MapperAssignDelegate : public QStyledItemDelegate
{
    Q_OBJECT

  public:
    MapperAssignDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
              const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                  const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

  private slots:
    void commitAndCloseEditor();
};

//-----------------------------------
//   MapperControlDelegate
//-----------------------------------

class MapperControlDelegate : public QStyledItemDelegate
{
    Q_OBJECT

  private:
    QStyle::SubElement _currentSubElement; // Set in mouse press, checked in release to prevent unwanted editor opening.
    // Need this. For some reason when using CurrentChanged trigger, createEditor is called upon opening the dialog, yet nothing is selected.
    bool _firstPress; 

    QRect getItemRectangle(const QStyleOptionViewItem& option, const QModelIndex& index, QStyle::SubElement subElement, QWidget* editor = nullptr) const;
    bool subElementHitTest(const QPoint& point, const QStyleOptionViewItem& option, const QModelIndex& index, QStyle::SubElement* subElement, QWidget* editor = nullptr) const;
    
  public:
    MapperControlDelegate(QWidget *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
              const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option,
                  const QModelIndex &index) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

  protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index);
    bool eventFilter(QObject* editor, QEvent* event);
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const;
    
  private slots:
    void editorChanged();
    //void commitAndCloseEditor();
};


class ControlMapperDialog : public QDialog, public Ui::ControlMapperBase
{
    Q_OBJECT

private:
    MusECore::Track*              _track;
    MusECore::CtrlListList*       _actrls;
    MusECore::MidiControllerList* _mctrls;

    MapperAssignDelegate*  _assignDelegate;
    MapperControlDelegate* _controlsDelegate;
    
//     int _port, _chan, _ctrl;
    bool _is_learning;
    void doUpdate();
    void resetLearn();
    //void updateCtrlBoxes();

private slots:
    void heartbeat();
    void learnChanged(bool);
    //void portChanged(int);
    //void chanChanged();
    //void ctrlTypeChanged(int);
    //void ctrlHChanged();
    //void ctrlLChanged();
    void configChanged();
    void controlsItemChanged(QTreeWidgetItem*, int);

public:
    enum MapperAssignCols { A_NAME=0, A_PORT, A_CHAN, A_MCTL_TYPE, A_MCTL_H, A_MCTL_L, A_MCTL_PASS_THRU, A_COL_END };
    //enum MapperControlCols { C_NAME=0, C_COLOR, C_ASSIGN, C_MCTL_TYPE, C_MCTL_H, C_MCTL_L, C_COL_END };
    enum MapperControlCols { C_NAME=0, C_ASSIGN_PORT, C_ASSIGN_CHAN, C_MCTL_NUM, C_MCTL_H, C_MCTL_L, C_COL_END };
    enum UserRolesExt { UserRole2 = Qt::UserRole+1 };
    
    ControlMapperDialog(MusECore::Track* t, QWidget* parent = nullptr);
    ~ControlMapperDialog();
//     int port() const { return _port; }
//     int chan() const { return _chan; }
//     int ctrl() const { return _ctrl; }
    MusECore::Track* track() const { return _track; }
};

}

#endif // __CONTROL_MAPPER_H__
