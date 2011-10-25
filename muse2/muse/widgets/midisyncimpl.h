//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midisyncimpl.h,v 1.1.1.1.2.3 2009/05/03 04:14:01 terminator356 Exp $
//
//  (C) Copyright 1999/2000 Werner Schweer (ws@seh.de)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __MIDISYNCIMPL_H__
#define __MIDISYNCIMPL_H__

#include "ui_midisync.h"
#include "sync.h"

class QCloseEvent;
class QDialog;
class QTreeWidgetItem;

namespace MusEGui {

//----------------------------------------------------------
//   MidiSyncLViewItem
//----------------------------------------------------------

class MidiSyncLViewItem : public QTreeWidgetItem
{
   //MusECore::MidiSyncInfo _syncInfo;
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
      
      //MusECore::MidiSyncInfo& syncInfo()  { return _syncInfo; }
      void copyFromSyncInfo(const MusECore::MidiSyncInfo &sp);
      void copyToSyncInfo(MusECore::MidiSyncInfo &sp);
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

   public:
      MidiSyncConfig(QWidget* parent=0);
      //MidiSyncConfig();
      ~MidiSyncConfig();
      void show();
      void setDirty();
      };

} // namespace MusEGui

#endif

