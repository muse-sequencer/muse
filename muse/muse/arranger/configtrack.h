//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: configtrack.h,v 1.4 2005/11/02 18:02:10 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CONFIG_TRACK_H__
#define __CONFIG_TRACK_H__

#include "ui_configtrack.h"
#include "track.h"
#include "trelement.h"

struct TrElement;
class TrGroupList;

//---------------------------------------------------------
//   ConfigTrackList
//---------------------------------------------------------

class ConfigTrackList : public QDialog, Ui::ConfigTrackListBase {
      Q_OBJECT

      TrGroupList list[Track::TRACK_TYPES];
      void init();
      bool dirty;
      void saveTrackType();
      int curType;

   private slots:
      void trackTypeChanged(int);
      void availableSelected();
      void configuredSelected();
      void upClicked();
      void downClicked();
      void addItemClicked();
      void removeItemClicked();
      virtual void done(int);

   signals:
      void trackConfigChanged();

   public:
      ConfigTrackList(QWidget*);
      };

#endif
