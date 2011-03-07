//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: organgui.h,v 1.8 2005/10/04 21:37:44 lunar_shuttle Exp $
//
//  (C) Copyright 2001-2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ORGANGUI_H__
#define __ORGANGUI_H__

#include "ui_organgui.h"
#include "organ.h"
#include "libsynti/gui.h"
#include "libsynti/midievent.h"

class QSignalMapper;

#define NUM_GUI_CONTROLLER 18

//---------------------------------------------------------
//   OrganGui
//---------------------------------------------------------

class OrganGui : public QWidget, public Ui::OrganGuiBase, public MessGui {
      Q_OBJECT

      bool ignoreControllerChange;

      QSignalMapper* map;
      virtual void processEvent(const MidiEvent&);

   protected:
      void setParam(int, int);
      void setParamIdx(int ctrlIdx, int val);

   private slots:
      void ctrlChanged(QWidget*);
      void readMessage(int);

   public:
      friend class Organ;
      friend class Mess2;
      OrganGui();
      };

#endif

