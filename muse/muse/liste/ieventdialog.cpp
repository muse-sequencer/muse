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

#include <QColor>
#include <QDialogButtonBox>
#include "ieventdialog.h"

InsertEventDialog::InsertEventDialog(const Pos& time,
				     QWidget* parent, Qt::WindowFlags f)
  : QDialog(parent, f) {
  setWindowTitle("Insert Event Dialog");

  _selectedType = -1;


  _lastDir = QDir::currentPath();

  _mainLayout = new QGridLayout(parent);
  //event type
  QLabel* eventTypeLabel = new QLabel("Event Type", parent);
  _mainLayout->addWidget(eventTypeLabel, 0, 0);
  _eventTypeComboBox = new QComboBox(parent);
  for(unsigned i = 0; i < IED_TypeCount; i++) {
    _typeWidget.push_back(new QWidget(parent));
    QGridLayout* tLayout = new QGridLayout(_typeWidget[i]);
    if(i == IED_Note) {
      _eventTypeComboBox->addItem(NoteSTR);
      //TODO
      QLabel* noteEventTODO =
	new QLabel("TODO : Note event", _typeWidget[i]);
      tLayout->addWidget(noteEventTODO, 0, 0, 1, 2);
    }
    else if(i == IED_ProgramChange) {
      _eventTypeComboBox->addItem(ProgramChangeSTR);
      //TODO
      QLabel* programChangeEventTODO =
	new QLabel("TODO : Program change event", _typeWidget[i]);
      tLayout->addWidget(programChangeEventTODO, 0, 0, 1, 2);
    }
    else if(i == IED_ControlChange) {
      _eventTypeComboBox->addItem(ControlChangeSTR);
      //TODO
      QLabel* controlChangeEventTODO =
	new QLabel("TODO : Control change event", _typeWidget[i]);
      tLayout->addWidget(controlChangeEventTODO, 0, 0, 1, 2);
    }
    else if(i == IED_Sysex) {
      _eventTypeComboBox->addItem(SysexSTR);
      //number of sysex
      QLabel* numOfSysexLabel = new QLabel("Number of Sysex", _typeWidget[i]);
      tLayout->addWidget(numOfSysexLabel, 0, 0);
      _sysexCountSpinBox = new QSpinBox(_typeWidget[i]);
      _sysexCountSpinBox->setMaximum(1024);
      _sysexCountSpinBox->setMinimum(1);
      _sysexCountSpinBox->setValue(1);
      tLayout->addWidget(_sysexCountSpinBox, 0, 1);
      //current sysex
      QLabel* curSysexLabel = new QLabel("Current Sysex", _typeWidget[i]);
      tLayout->addWidget(curSysexLabel, 1, 0);
      _curSysexSpinBox = new QSpinBox(_typeWidget[i]);
      _curSysexSpinBox->setMaximum(0);
      _curSysexSpinBox->setMinimum(0);
      _curSysexSpinBox->setValue(0);
      tLayout->addWidget(_curSysexSpinBox, 1, 1);
      //select from the instrument
      QPushButton* selectButton = new QPushButton("Select from instrument",
						  _typeWidget[i]);
      tLayout->addWidget(selectButton, 2, 0, 1, 2);
      //load
      _loadButton = new QPushButton("Load...", _typeWidget[i]);
      tLayout->addWidget(_loadButton, 3, 0);
      //save
      _saveButton = new QPushButton("Save...", _typeWidget[i]);
      tLayout->addWidget(_saveButton, 3, 1);
      //length
      QLabel* lengthLabel = new QLabel("Length", _typeWidget[i]);
      tLayout->addWidget(lengthLabel, 4, 0);
      _lengthIntLabel = new QLabel("0", _typeWidget[i]);
      tLayout->addWidget(_lengthIntLabel, 4, 1);
      //text edit
      _sysexTextEdit = new QTextEdit(_typeWidget[i]);
      _sysexCursorPos = 0;
      QByteArray ba;
      _dataSysex.push_back(QByteArray(ba));
      _lengthIntLabel->setText(QString::number(sysexLength()));
      _dataSysexStr.push_back(QString("F0  F7"));
      tLayout->addWidget(_sysexTextEdit, 5, 0, 3, 2);
      setSysexTextEdit();
      QString HEX = "(?!F7)([A-F]|\\d){1,2}";
      QString SRE = QString("^F0(\\s+)(") + HEX + QString("(\\s+))*F7$");
      _hexRegExp = new QRegExp(SRE, Qt::CaseInsensitive);
    }
    tLayout->setMargin(0);
    _typeWidget[i]->setLayout(tLayout);
    _mainLayout->addWidget(_typeWidget[i], 2, 0, 1, 2);
  }
  _mainLayout->addWidget(_eventTypeComboBox, 0, 1);
  //time
  QLabel* timeLabel = new QLabel("Time", parent);
  _mainLayout->addWidget(timeLabel, 1, 0);
  _timePosEdit = new PosEdit(parent);
  _timePosEdit->setValue(time);
  _mainLayout->addWidget(_timePosEdit, 1, 1);
  //Ok, cancel
  QDialogButtonBox* OkCancelBox =
    new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
			 Qt::Horizontal, parent);
  _mainLayout->addWidget(OkCancelBox, 3, 0, 1, 2);

  updateType(_eventTypeComboBox->currentIndex());
  
  setLayout(_mainLayout);

  //connection
  connect(_sysexTextEdit, SIGNAL(textChanged()), this,
	  SLOT(updateSysexTextEdit()));
  connect(_sysexTextEdit, SIGNAL(cursorPositionChanged()), this,
	  SLOT(updateSysexCursor()));
  connect(_eventTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
	  SLOT(updateType(int)));
  connect(_sysexCountSpinBox, SIGNAL(valueChanged(int)), this,
	  SLOT(updateSysexCount(int)));
  connect(_curSysexSpinBox, SIGNAL(valueChanged(int)), this,
	  SLOT(updateCurSysexSpinBox(int)));
  connect(_loadButton, SIGNAL(clicked()), this, SLOT(updateSysexLoad()));
  connect(_saveButton, SIGNAL(clicked()), this, SLOT(updateSysexSave()));
  connect(OkCancelBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(OkCancelBox, SIGNAL(rejected()), this, SLOT(reject()));
}

InsertEventDialog::~InsertEventDialog() {
}

EventList* InsertEventDialog::elResult() {
  int curType = _eventTypeComboBox->currentIndex();
  if(curType == IED_Note) {
    //TODO
    return NULL;
  }
  else if(curType == IED_ProgramChange) {
    //TODO
    return NULL;
  }
  else if(curType == IED_ControlChange) {
    //TODO
    return NULL;
  }
  else if(curType == IED_Sysex) {
    EventList* res = new EventList;
    for(unsigned i = 0; (int)i < _sysexCountSpinBox->value(); i++) {
      Event evSysex(Sysex);
      evSysex.setPos(_timePosEdit->pos());
      evSysex.setData((const unsigned char*) _dataSysex[i].data(),
		      _dataSysex[i].size());
      res->add(evSysex);
    }
    return res;
  }
  else return NULL;
}

int InsertEventDialog::sysexLength() {
  return _dataSysex[_curSysexSpinBox->value()].size();
}
QString InsertEventDialog::ByteArray2Str(const QByteArray& ba) {
  QString res = "F0 ";
  for(int i = 0; i < ba.size(); i++) {
    res += QString::number((unsigned char)ba.at(i), 16);
    res += " ";
  }
  res += "F7";
  return res.toUpper();
}
QByteArray InsertEventDialog::Str2ByteArray(const QString& s) {
  QByteArray ba;
  QString simpli = s.simplified();
  QStringList sl = simpli.split(" ");
  bool ok;
  for(int i = 1; i < sl.size() - 1; i++) {//i=1 and until size-1 to skip F0, F7
    ba.push_back(sl[i].toInt(&ok, 16));
  }
  return ba;
}
void InsertEventDialog::setSysexTextEdit() {
  QString s = _dataSysexStr[_curSysexSpinBox->value()].toUpper();
  _sysexTextEdit->blockSignals(true);
  //print in gray F0
  _sysexTextEdit->setTextColor(Qt::darkGray);
  _sysexTextEdit->setPlainText(s.left(2));
  //move cursor at the end
  QTextCursor tc1(_sysexTextEdit->document());
  tc1.movePosition(QTextCursor::End);
  _sysexTextEdit->setTextCursor(tc1);
  //print in black the core
  _sysexTextEdit->setTextColor(Qt::black);
  _sysexTextEdit->insertPlainText(s.mid(2, s.size()-4));
  //move cursor at the end
  QTextCursor tc2(_sysexTextEdit->document());
  tc2.movePosition(QTextCursor::End);
  _sysexTextEdit->setTextCursor(tc2);
  //print in gray F7
  _sysexTextEdit->setTextColor(Qt::darkGray);
  _sysexTextEdit->insertPlainText(s.right(2));  
  //relocate cursor
  QTextCursor tc(_sysexTextEdit->document());
  tc.setPosition(_sysexCursorPos);
  _sysexTextEdit->setTextCursor(tc);
  //update length label
  _lengthIntLabel->setText(QString::number(sysexLength()));

  _sysexTextEdit->blockSignals(false);
}

//SLOTS
void InsertEventDialog::updateSysexTextEdit() {
  int cur = _curSysexSpinBox->value();
  int curCursor = _sysexTextEdit->textCursor().position();
  QString data = _sysexTextEdit->toPlainText();
  bool correctInput = _hexRegExp->exactMatch(data);

  if(correctInput) {
    _dataSysexStr[cur] = data;
    _sysexCursorPos = curCursor;
  }
 
  //display text, relocate cursor and update length
  setSysexTextEdit();
  //update _dataSysex
  _dataSysex[cur] = Str2ByteArray(_dataSysexStr[cur]);
}
void InsertEventDialog::updateSysexCursor() {
  if(_dataSysexStr[_curSysexSpinBox->value()]
     == _sysexTextEdit->toPlainText()) //only if the text hasn't changed
    _sysexCursorPos = _sysexTextEdit->textCursor().position();
}
void InsertEventDialog::updateSysexCount(int c) {
  while((int)_dataSysexStr.size() < c) {
    _dataSysexStr.push_back(QString("F0  F7"));
    QByteArray ba;
    _dataSysex.push_back(ba);
  }
  if(c <= _curSysexSpinBox->value()) _curSysexSpinBox->setValue(c-1);
  _curSysexSpinBox->setMaximum(c-1);
}
void InsertEventDialog::updateCurSysexSpinBox(int /*c*/) {
  setSysexTextEdit();
}
void InsertEventDialog::updateSysexLoad() {
  QByteArray ba;
  QString fileName =
    QFileDialog::getOpenFileName(this,
				 tr("Load Sysex dialog"),
				 _lastDir,
				 QString("*.syx;; *"));
  if(!fileName.isEmpty()) {
    QFileInfo fi(fileName);
    _lastDir = fi.path();

    QFile f(fileName);
    if(f.open(QIODevice::ReadOnly)) {
      ba = f.readAll();
      bool sysexStart = false;
      int sysexCount = 0;
      for(int i = 0; i < ba.size(); i++) {
	if((unsigned char)ba.at(i) == 0xF0) {
	  sysexStart = true;
	  sysexCount++;
	  while((int)_dataSysex.size() < sysexCount) {
	    QByteArray tempba;
	    _dataSysex.push_back(tempba);
	    _dataSysexStr.push_back(QString(""));
	  }
	  _dataSysex[sysexCount-1].clear();
	}
	else if((unsigned char)ba.at(i) == 0xF7) {
	  QByteArray endba(_dataSysex[sysexCount-1]);
	  sysexStart = false;
	  _dataSysexStr[sysexCount-1] = ByteArray2Str(endba);
	}
	else if(sysexStart) _dataSysex[sysexCount-1].push_back(ba.at(i));
      }
      _sysexCountSpinBox->setValue(sysexCount);
      _curSysexSpinBox->setValue(0);
      updateCurSysexSpinBox(0);
      //Message dialog
      if(sysexCount > 0) {
	QMessageBox::information(this, tr("Sysex loaded"),
				 tr("MusE has successfully detected"
				    " and loaded ") +
				 QString::number(sysexCount) + 
				 tr(" sysex messages."));
      }
      else {
	QMessageBox::information(this, tr("No sysex"),
				 tr("No sysex in this file"));
      }
    }
    else {
      QMessageBox::critical(0,
			    tr("Critical Error"),
			    tr("Cannot open file %1").arg(fileName));
    }
  }
}
void InsertEventDialog::updateSysexSave() {
  QString filename =
    QFileDialog::getSaveFileName(
				 this,
				 tr("Save Sysex Dialog"),
				 _lastDir,
				 QString("*"));
  if(!filename.isEmpty()) {
    QFileInfo fi(filename);
    _lastDir = fi.path();
    QFile f(filename);
    if(f.open(QIODevice::WriteOnly)) {
      for(unsigned i = 0; i < _dataSysex.size(); i++) {
	qint64 wok;
	wok = f.write(QByteArray(1, 0xF0));
	wok = f.write(_dataSysex[i]);
	wok = f.write(QByteArray(1, 0xF7));
	if(wok == -1) {
	  QMessageBox::critical(0,
				tr("Critical Error"),
				tr("Cannot write the sysex number %1")
				.arg(i));
	}
      }
    }
    else {
      QMessageBox::critical(0,
			    tr("Critical Error"),
			    tr("Cannot save file %1").arg(filename));
    }
  } 
}
void InsertEventDialog::updateType(int type) {
  if(_selectedType != type) {
    for(unsigned i = 0; i < IED_TypeCount; i++) {
      if(type == (int)i) _typeWidget[i]->show();
      else _typeWidget[i]->hide();
    }
    _selectedType = type;
  }
}
