//
// C++ Interface: editgain
//
// Description: 
//
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef EDITGAIN_H
#define EDITGAIN_H

#include "ui_editgainbase.h"

class QDialog;

class EditGain : public QDialog, public Ui::EditGainBase
{
      Q_OBJECT
public:
    EditGain(QWidget* parent = 0, int initGainValue=100);

    ~EditGain();
    int getGain();

private:
    int gain;

private slots:
    void resetPressed();
    void applyPressed();
    void cancelPressed();
    void gainChanged(int value);
};

#endif
