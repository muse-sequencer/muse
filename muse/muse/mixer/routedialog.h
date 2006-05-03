//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: routedialog.h,v 1.5 2006/01/06 22:48:09 wschweer Exp $
//
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __ROUTEDIALOG_H__
#define __ROUTEDIALOG_H__

#include "ui_routedialog.h"

//---------------------------------------------------------
//   RouteDialog
//---------------------------------------------------------

class RouteDialog : public QDialog, public Ui::RouteDialogBase {
      Q_OBJECT

      virtual void closeEvent(QCloseEvent*);
      void routingChanged();

   private slots:
      void routeSelectionChanged();
      void removeRoute();
      void addRoute();
      void srcSelectionChanged();
      void dstSelectionChanged();
      void songChanged(int);

   signals:
      void closed();

   public:
      RouteDialog(QWidget* parent);
      };


#endif

