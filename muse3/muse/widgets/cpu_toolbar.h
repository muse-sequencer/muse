//=========================================================
//  MusE
//  Linux Music Editor
//  cpu_toolbar.h
//  (C) Copyright 2016 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __CPU_TOOLBAR_H__
#define __CPU_TOOLBAR_H__

#include <QLabel>
#include <QToolBar>

class QWidget;
class QString;
class QToolButton;
class QSize;

namespace MusEGui
{

  //---------------------------------
  //   PaddedValueLabel
  //---------------------------------

  class PaddedValueLabel : public QLabel
  {
    Q_OBJECT
    
    private:

      bool _isFloat;
      QString _prefix;
      QString _suffix;

      int _fieldWidth;
      int _precision;
      int _iVal;
      double _dVal;
      
      void updateText();
      
    public:
      PaddedValueLabel(bool isFloat = false, QWidget* parent = 0, Qt::WindowFlags f = 0, 
                       const QString& prefix = QString(), const QString& suffix = QString());

      void setFieldWidth(int val);
      void setPrecision(int val);
      
      void setIntValue(int val);
      void setFloatValue(double val);
      virtual QSize sizeHint() const;
  };

  //---------------------------------
  //   CpuToolbar
  //---------------------------------

  class CpuToolbar : public QToolBar
  {
    Q_OBJECT
    
    private:
      QToolButton* _resetButton;
      PaddedValueLabel* _cpuLabel;
      PaddedValueLabel* _dspLabel;
      PaddedValueLabel* _xrunsLabel;

      void init();
      
    public:
      CpuToolbar(QWidget* parent = 0);
      CpuToolbar(const QString& title, QWidget* parent = 0);

      void setCpuLabelText(const QString&);
      void setDspLabelText(const QString&);
      void setXrunsLabelText(const QString&);
      void setValues(float cpuLoad, float dspLoad, long xRunsCount);
      
    signals:
      void resetClicked();
  };

}

#endif
