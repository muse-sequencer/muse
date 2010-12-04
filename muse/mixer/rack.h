//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.h,v 1.5.2.3 2006/09/24 19:32:31 terminator356 Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __RACK_H__
#define __RACK_H__

#include <q3listbox.h>
#include <QDragLeaveEvent>
//Added by qt3to4:
#include <QMouseEvent>
#include <QDropEvent>
#include <QDragEnterEvent>
#include "xml.h"

class AudioTrack;

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

class EffectRack : public Q3ListBox {
      AudioTrack* track;
      Q_OBJECT

      virtual QSize minimumSizeHint() const;
      virtual QSize sizeHint() const;
      
      void startDrag(int idx);
      void initPlugin(Xml xml, int idx);
      QPoint dragPos;
      void savePreset(int idx);

   private slots:
      void menuRequested(Q3ListBoxItem*, const QPoint&);
      void doubleClicked(Q3ListBoxItem*);
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

