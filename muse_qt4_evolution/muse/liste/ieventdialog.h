//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//  
//  Copyright (C) 2007 by Werner Schweer and others
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

#ifndef __IEVENTDIALOG_H__
#define __IEVENTDIALOG_H__

#include <QDialog>
#include <vector>
#include <QStringList>
#include <QLineEdit>
#include <QTextEdit>
#include <QRegExp>
#include <QDir>
#include "awl/posedit.h"
#include "midievent.h"
#include "al/pos.h"
#include "muse/event.h"
#include "muse/part.h"

class QDialog;
using Awl::PosEdit;
using AL::Pos;

#define NoteSTR "Note"
#define ProgramChangeSTR "Program change"
#define ControlChangeSTR "Control change"
#define SysexSTR "Sysex"

#define IED_MAX(x, y) (x > y? x : y)

class InsertEventDialog : public QDialog {
  Q_OBJECT

  QGridLayout* _mainLayout;
  std::vector<QWidget*> _typeWidget;
  int _selectedType;

  QString _lastDir;

  Part* _part;


  //event type
  QComboBox* _eventTypeComboBox;

  //time
  PosEdit* _timePosEdit;

  //Note
  QSpinBox* _pitchSpinBox;
  QSpinBox* _velocitySpinBox;
  QSpinBox* _veloOffSpinBox;
  QSpinBox* _lengthSpinBox;
  //QLabel* _noteLabel;

  //Sysex
  QSpinBox* _sysexCountSpinBox;
  QSpinBox* _curSysexSpinBox;
  QLabel* _lengthIntLabel;
  QTextEdit* _sysexTextEdit;
  QRegExp* _hexRegExp;
  int _sysexCursorPos;
  QPushButton* _loadButton;
  QPushButton* _saveButton;
  std::vector<QByteArray> _dataSysex;
  std::vector<QString> _dataSysexStr;

  enum {
    IED_Note,
    IED_ProgramChange,
    IED_ControlChange,
    IED_Sysex,
    IED_TypeCount };

  int sysexLength(); //return the length of the current sysex
  void setSysexTextEdit(); //set the display of sysexTextEdit

 public:
  InsertEventDialog(const Pos& time, Part* part, Event* ev = NULL,
		    QWidget* parent = 0, Qt::WindowFlags f = 0);
  ~InsertEventDialog();

  EventList* elResult();

  static QString charArray2Str(const char* s, int length); //add F0 and F7
  static QString ByteArray2Str(const QByteArray& ba); //add F0 and F7
  static QByteArray Str2ByteArray(const QString& s); //skip F0 and F7
  static char* Str2CharArray(const QString& s);

 private slots:
  void updateSysexTextEdit();
  void updateSysexCursor();
  void updateSysexCount(int c);
  void updateCurSysexSpinBox(int c);
  void updateSysexLoad();
  void updateSysexSave();
  void updateType(int type);
};

#endif
