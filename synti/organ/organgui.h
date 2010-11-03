//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organgui.h,v 1.6.2.1 2005/12/29 23:33:50 spamatica Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ORGANGUI_H__
#define __ORGANGUI_H__

#include "organguibase.h"
#include "organ.h"
#include "libsynti/gui.h"
//#include "libsynti/mpevent.h"
#include "muse/mpevent.h"   

class QSignalMapper;

#define NUM_GUI_CONTROLLER 18

//---------------------------------------------------------
//   OrganGui
//---------------------------------------------------------

class OrganGui : public OrganGuiBase, public MessGui {
      Q_OBJECT

      QSignalMapper* map;
      SynthGuiCtrl dctrl[NUM_GUI_CONTROLLER];
      void setParam(int, int);

   private slots:
      void ctrlChanged(int idx);
      void readMessage(int);

   public:
      virtual void processEvent(const MidiPlayEvent&);
      int getControllerMinMax(int id, int* min, int* max) const;
      OrganGui();
      };

#endif

