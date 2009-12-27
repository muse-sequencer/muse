//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.h,v 1.5.2.3 2006/09/24 19:32:31 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __RACK_H__
#define __RACK_H__

#include <qlistbox.h>
#include "xml.h"

class AudioTrack;

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

class EffectRack : public QListBox {
      AudioTrack* track;
      Q_OBJECT

      virtual QSize minimumSizeHint() const;
      
      void startDrag(int idx);
      void initPlugin(Xml xml, int idx);
      QPoint dragPos;
      void savePreset(int idx);

   private slots:
      void menuRequested(QListBoxItem*, const QPoint&);
      void doubleClicked(QListBoxItem*);
      void songChanged(int);

   protected:
      void dropEvent(QDropEvent *event);
      void dragEnterEvent(QDragEnterEvent *event);
      void contentsDropEvent(QDropEvent *event);
      void contentsDragEnterEvent(QDragEnterEvent *event);
      void contentsMousePressEvent(QMouseEvent *event);
      void contentsMouseMoveEvent(QMouseEvent *event);
   
   public:
      EffectRack(QWidget*, AudioTrack* t);
      ~EffectRack();
      
      AudioTrack* getTrack() { return track; } 
      QPoint getDragPos() { return dragPos; }
      };

#endif

