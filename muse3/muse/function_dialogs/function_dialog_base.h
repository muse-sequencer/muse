//=========================================================
//  MusE
//  Linux Music Editor
//
//  function_dialog_base.h
//  (C) Copyright 2018 Tim E. Real (terminator356 on users dot sourceforge dot net)
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

#ifndef __FUNCTION_DIALOG_BASE_H__
#define __FUNCTION_DIALOG_BASE_H__

#include "function_dialog_consts.h"
#include <QDialog>

// NOTE: To cure circular dependencies, of which there are many, these are
//        forward referenced and the corresponding headers included further down here.
class QButtonGroup;
class QWidget;

namespace MusECore {
class Xml;
}

namespace MusEGui {

class FunctionDialogBase : public QDialog
{
  Q_OBJECT
  protected:
    QButtonGroup* _range_group;
    QButtonGroup* _parts_group;
    QWidget* _range_container;
    QWidget* _parts_container;

    void setupButton(QButtonGroup* group, int buttonID, bool show);
    
    virtual FunctionReturnDialogFlags_t calc_return_flags()
    {
      const int cr = curRange();
      const int cp = curParts();
      FunctionReturnDialogFlags_t ret_flags = FunctionReturnNoFlags;
      if(cr == FunctionAllEventsButton || cr == FunctionLoopedButton)
        ret_flags |= FunctionReturnAllEvents;
      if(cp == FunctionAllPartsButton)
        ret_flags |= FunctionReturnAllParts;
      if(cr == FunctionLoopedButton || cr == FunctionSelectedLoopedButton)
        ret_flags |= FunctionReturnLooped;
      return ret_flags;
    }
    
  protected slots:
    virtual void accept();
    virtual void pull_values();

  public:
    FunctionDialogBase(QWidget* parent = 0);
    virtual ~FunctionDialogBase();

    // Returns true if the tag was handled by this base class, otherwsise false.
    static bool read_configuration(const QString& tag, MusECore::Xml& xml);
    // Writes a small portion, just part of the overall method in the sub classes.
    virtual void write_configuration(int level, MusECore::Xml& xml);
    
    virtual void setupDialog();

    virtual int curRange() const { return -1; }
    virtual void setCurRange(int) {  }
    virtual int curParts() const { return -1; }
    virtual void setCurParts(int) {  }
    virtual FunctionDialogElements_t elements() const { return FunctionDialogNoElements; }
    virtual void setReturnFlags(FunctionReturnDialogFlags_t) {  }
    
  public slots:
    virtual int exec();
};

} // namespace MusEGui

#endif


