//===========================================================================
//
//    ladspapandelay
//
//    Version 0.0.1
//
//
//
//
//  Copyright (c) 2006 Nil Geisweiller
//
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA or point your web browser to http://www.gnu.org.
//===========================================================================

#ifndef __LADSPAPANDELAY_H
#define __LADSPAPANDELAY_H

#include "pandelaymodel.h"
#include "../../muse/ladspa.h"

#define NBRPARAM 5

class LADSPAPanDelay : public PanDelayModel {
 private:

 public:
  LADSPAPanDelay(unsigned long samplerate);
  ~LADSPAPanDelay();

  LADSPA_Data* port[NBRPARAM + 4];
  float param[NBRPARAM];

  void updateParameters();
  void processMix(long numsamples);
  void processReplace(long numsamples);

  void activate();
};

#endif
