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
#include <q3whatsthis.h>
#include <qtooltip.h>
//Added by qt3to4:
#include <Q3PopupMenu>

#include "synthconfigbase.h"

class Q3ListView;
class Q3ListViewItem;
class QPoint;
class Q3PopupMenu;
class Q3Header;
class Xml;

//----------------------------------------------------------
//   MPHeaderTip
//----------------------------------------------------------

class MPHeaderTip { // : public QToolTip { ddskrjo

   public:
    MPHeaderTip(QWidget * parent) {} // : QToolTip(parent) {} ddskrjo
      virtual ~MPHeaderTip() {}
   protected:
      void maybeTip(const QPoint &);
      };

//---------------------------------------------------------
//   MPWhatsThis
//---------------------------------------------------------

class MPWhatsThis : public Q3WhatsThis {
      Q3Header* header;

   protected:
      QString text(const QPoint&);

   public:
      MPWhatsThis(QWidget* parent, Q3Header* h) : Q3WhatsThis(parent) {
            header = h;
            }
      };

//---------------------------------------------------------
//   MPConfig
//    Midi Port Config
//---------------------------------------------------------

class MPConfig : public SynthConfigBase {
      MPHeaderTip* _mptooltip;
      Q3PopupMenu* popup;
      Q3PopupMenu* instrPopup;
      
      int _showAliases; // -1: None. 0: First aliases. 1: Second aliases etc.
      
      Q_OBJECT

   private slots:
      void rbClicked(Q3ListViewItem*, const QPoint&,int);
      void mdevViewItemRenamed(Q3ListViewItem*, int, const QString&);
      void songChanged(int);
      void selectionChanged();
      void addInstanceClicked();
      void removeInstanceClicked();

   public:
      MPConfig(QWidget* parent, char* name);
      ~MPConfig();
      };

#endif
