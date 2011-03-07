//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: confmport.h,v 1.3 2004/01/25 11:20:31 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __CONFMPORT_H__
#define __CONFMPORT_H__

#include <qwidget.h>
#include <qwhatsthis.h>
#include <qtooltip.h>

#include "synthconfigbase.h"

class QListView;
class QListViewItem;
class QPoint;
class QPopupMenu;
class QHeader;
class Xml;

//----------------------------------------------------------
//   MPHeaderTip
//----------------------------------------------------------

class MPHeaderTip : public QToolTip {

   public:
      MPHeaderTip(QWidget * parent) : QToolTip(parent) {}
      virtual ~MPHeaderTip() {}
   protected:
      void maybeTip(const QPoint &);
      };

//---------------------------------------------------------
//   MPWhatsThis
//---------------------------------------------------------

class MPWhatsThis : public QWhatsThis {
      QHeader* header;

   protected:
      QString text(const QPoint&);

   public:
      MPWhatsThis(QWidget* parent, QHeader* h) : QWhatsThis(parent) {
            header = h;
            }
      };

//---------------------------------------------------------
//   MPConfig
//    Midi Port Config
//---------------------------------------------------------

class MPConfig : public SynthConfigBase {
      MPHeaderTip* _mptooltip;
      QPopupMenu* popup;
      QPopupMenu* instrPopup;
      
      int _showAliases; // -1: None. 0: First aliases. 1: Second aliases etc.
      
      Q_OBJECT

   private slots:
      void rbClicked(QListViewItem*, const QPoint&,int);
      void mdevViewItemRenamed(QListViewItem*, int, const QString&);
      void songChanged(int);
      void selectionChanged();
      void addInstanceClicked();
      void removeInstanceClicked();

   public:
      MPConfig(QWidget* parent, char* name);
      ~MPConfig();
      };

#endif
