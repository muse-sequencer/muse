//=========================================================
//  MusE
//  Linux Music Editor
//
//  (C) Copyright 2008 Robert Jonsson (rj@spamatica.se)
//  (C) Copyright 2005- Werner Schweer (ws@seh.de)
// Copyright: See COPYING file that comes with this distribution
//=========================================================

#ifndef __DRUMGLUEGUI_H__
#define __DRUMGLUEGUI_H__

//#include "ui_drumglue.h"
#include "ui_drumgluegui.h"


//---------------------------------------------------------
//   DrumGlueGui
//---------------------------------------------------------
class DrumGlue;

class DrumGlueGui : public QDialog, public Ui::DrumGlueBase {
      Q_OBJECT

      DrumGlue* drumGlue;

   signals:
      void hideWindow();

   public:
      DrumGlueGui(DrumGlue*, QWidget* parent=0);
      void init();
      
   private slots:
      void addInstrument();
      void removeInstrument();
      
      };

#endif

