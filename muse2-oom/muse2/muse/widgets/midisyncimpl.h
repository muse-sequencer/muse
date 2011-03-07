//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midisyncimpl.h,v 1.1.1.1.2.3 2009/05/03 04:14:01 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDISYNCIMPL_H__
#define __MIDISYNCIMPL_H__

#include "ui_midisync.h"
#include "sync.h"

class QCloseEvent;
class QDialog;
class QTreeWidgetItem;

//----------------------------------------------------------
//   MidiSyncLViewItem
//----------------------------------------------------------

class MidiSyncLViewItem : public QTreeWidgetItem
{
   //MidiSyncInfo _syncInfo;
   //MidiDevice* _device;
   int _port;
   
   //protected:
      //int _port;
      
   public:
      MidiSyncLViewItem(QTreeWidget* parent)
         : QTreeWidgetItem(parent) { _port = -1; _inDet = _curDet = _tickDet = false; }
         //: QListViewItem(parent) { _device = 0; }
         
      //MidiSyncLViewItem(QListView* parent, QListViewItem* after)
      //   : QListViewItem(parent, after) { _port = -1; }
      
      //virtual QString text(int column) const;
      //virtual unsigned tick() = 0;
      
      //int _port;
      bool _inDet;
      bool _curDet;
      bool _curMTCDet;
      bool _tickDet;
      bool _MRTDet;
      bool _MMCDet;
      bool _MTCDet;
      int _recMTCtype;
     
      int _idOut;
      int _idIn;
      
      bool _sendMC;
      bool _sendMRT;
      bool _sendMMC;
      bool _sendMTC;
      bool _recMC;
      bool _recMRT;
      bool _recMMC;
      bool _recMTC;
      
      bool _recRewOnStart;
      //bool _sendContNotStart;
      
      int port() const { return _port; }
      void setPort(int port);
      //MidiDevice* device() const { return _device; }
      //void setDevice(MidiDevice* d);
      
      //MidiSyncInfo& syncInfo()  { return _syncInfo; }
      void copyFromSyncInfo(const MidiSyncInfo &sp);
      void copyToSyncInfo(MidiSyncInfo &sp);
};

//---------------------------------------------------------
//   MSConfig
//---------------------------------------------------------

class MidiSyncConfig : public QDialog, public Ui::MidiSyncConfigBase {
      Q_OBJECT

      bool inHeartBeat;
      bool _dirty;
      
      void updateSyncInfoLV();
      void closeEvent(QCloseEvent*);
      void setToolTips(QTreeWidgetItem *item);
      void setWhatsThis(QTreeWidgetItem *item);
      void addDevice(QTreeWidgetItem *item, QTreeWidget *tree);

   private slots:
      void heartBeat();
      void syncChanged();
      void extSyncChanged(bool v);
      void ok();
      void cancel();
      void apply();
      //void dlvClicked(QListViewItem*, const QPoint&, int);
      void dlvClicked(QTreeWidgetItem*, int);
      void dlvDoubleClicked(QTreeWidgetItem*, int);
      //void renameOk(QListViewItem*, int, const QString&);
      void songChanged(int);

   //signals:
   //   void deleted(unsigned long);

   public:
      MidiSyncConfig(QWidget* parent=0);
      //MidiSyncConfig();
      ~MidiSyncConfig();
      void show();
      void setDirty();
      };

#endif

