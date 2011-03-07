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

#include "editgainbase.h"

class EditGain : public EditGainBase
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
