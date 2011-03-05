//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: tools.h,v 1.1.1.1 2003/10/27 18:54:49 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Robert Jonsson (rj@spamatica.se)
//=========================================================
#ifndef VISIBLETRACKS_H
#define VISIBLETRACKS_H

#include <QToolBar>

class Action;

class QAction;
class QPixmap;
class QWidget;

struct VisibleToolB {
      QPixmap** icon;
      const char* tip;
      const char* ltip;
      };

extern VisibleToolB visTrackList[];

//---------------------------------------------------------
//   EditToolBar
//---------------------------------------------------------

class VisibleTracks : public QToolBar {
      Q_OBJECT
      Action** actions;
      int nactions;

   private slots:
      void visibilityChanged(QAction* action);

   signals:
      void visibilityChanged();

   public:
      VisibleTracks(QWidget* /*parent*/, const char* name = 0);  // Needs a parent !
      void updateVisibleTracksButtons();
      ~VisibleTracks();
      };



#endif // VISIBLETRACKS_H
