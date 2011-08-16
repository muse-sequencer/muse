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

#include "ui_cliplisteditorbase.h"

class QCloseEvent;
class QDialog;
class QWidget;
class QTreeWidgetItem;

class Xml;
class Pos;

//---------------------------------------------------------
//   ClipListEditorBaseWidget
//   Wrapper around Ui::ClipListEditorBase
//---------------------------------------------------------

class ClipListEditorBaseWidget : public QWidget, public Ui::ClipListEditorBase
{
      Q_OBJECT

   public:
      ClipListEditorBaseWidget(QWidget *parent = 0) : QWidget(parent) { setupUi(this); }
};

//---------------------------------------------------------
//   ClipListEdit
//---------------------------------------------------------

class ClipListEdit : public TopWin {
      Q_OBJECT
      ClipListEditorBaseWidget* editor;

      virtual void closeEvent(QCloseEvent*);
      void updateList();

   private slots:
      void songChanged(int);
      void startChanged(const Pos&);
      void lenChanged(const Pos&);
      void clipSelectionChanged();
      void clicked(QTreeWidgetItem*, int);

   signals:
      void deleted(TopWin*);

   public:
      ClipListEdit(QWidget* parent);
      ~ClipListEdit();
      virtual void readStatus(Xml&);
      virtual void writeStatus(int, Xml&) const;
      static void readConfiguration(Xml&);
      static void writeConfiguration(int, Xml&);
      };

#endif

