//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: cliplist.h,v 1.7 2006/01/06 22:48:09 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CLIPLIST_H__
#define __CLIPLIST_H__

#include "../cobject.h"
#include "event.h"

class Xml;
class Pos;
class ClipListEditorBase;

//---------------------------------------------------------
//   ClipListEdit
//---------------------------------------------------------

class ClipListEdit : public TopWin {
      Q_OBJECT
      ClipListEditorBase* editor;

      virtual void closeEvent(QCloseEvent*);
      void updateList();

   private slots:
      void songChanged(int);
      void startChanged(const Pos&);
      void lenChanged(const Pos&);
      void clipSelectionChanged();
      void clicked(Q3ListViewItem*);

   signals:
      void deleted(int);

   public:
      ClipListEdit();
      virtual void readStatus(QDomNode);
      virtual void writeStatus(Xml&) const;
      };

#endif

