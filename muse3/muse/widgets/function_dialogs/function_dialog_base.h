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

class QButtonGroup;

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
    
    virtual void set_return_flags()
    {
      _ret_flags = FunctionReturnNoFlags;
      if(_range == FunctionAllEventsButton || _range == FunctionLoopedButton)
        _ret_flags |= FunctionReturnAllEvents;
      if(_parts == FunctionAllPartsButton)
        _ret_flags |= FunctionReturnAllParts;
      if(_range == FunctionLoopedButton || _range == FunctionSelectedLoopedButton)
        _ret_flags |= FunctionReturnLooped;
    }
    
  protected slots:
    virtual void accept();
    virtual void pull_values();

  public:
    FunctionDialogBase(QWidget* parent = 0);
    virtual ~FunctionDialogBase();

    static int _range;
    static int _parts;
    static FunctionReturnDialogFlags_t _ret_flags;
    static FunctionDialogElements_t _elements;
    
    // Returns true if the tag was handled by this base class, otherwsise false.
    static bool read_configuration(const QString& tag, MusECore::Xml& xml);
    // Writes a small portion, just part of the overall method in the sub classes.
    virtual void write_configuration(int level, MusECore::Xml& xml);
    
    static void setElements(FunctionDialogElements_t elements =
      FunctionAllEventsButton | FunctionSelectedEventsButton |
      FunctionLoopedButton | FunctionSelectedLoopedButton) { 
        _elements = elements; }
      
    virtual void setupDialog();
    
  public slots:
    virtual int exec();
};

} // namespace MusEGui

#endif


