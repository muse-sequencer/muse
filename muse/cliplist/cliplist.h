//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: cliplist.h,v 1.3.2.1 2005/12/11 21:29:23 spamatica Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CLIPLIST_H__
#define __CLIPLIST_H__

#include "../cobject.h"
#include "event.h"
//Added by qt3to4:
#include <QCloseEvent>

class Q3ListView;
class Q3ListViewItem;
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
      void deleted(unsigned long);

   public:
      ClipListEdit();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      };

#endif

