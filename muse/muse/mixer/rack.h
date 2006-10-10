//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: rack.h,v 1.12 2006/01/14 23:44:57 spamatica Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __RACK_H__
#define __RACK_H__

#include <al/xml.h>

class AudioTrack;
class Plugin;

//---------------------------------------------------------
//   EffectRack
//---------------------------------------------------------

class EffectRack : public QListWidget {
      Q_OBJECT

      bool prefader;
      AudioTrack* track;
      
      QPoint dragPos;

      virtual void contextMenuEvent(QContextMenuEvent*);
      virtual void mouseDoubleClickEvent(QMouseEvent*);
      void selectNew();
      
      void startDrag(int idx);
      void initPlugin(QDomNode &node, int idx);
      void addPlugin(Plugin* plugin);

   private slots:
      void doubleClicked(QListWidgetItem*);
      void songChanged(int);

   protected:
      void dropEvent(QDropEvent *event);
      void dragEnterEvent(QDragEnterEvent *event);
      void mousePressEvent(QMouseEvent *event);
      void mouseMoveEvent(QMouseEvent *event);
      void dragMoveEvent(QDragMoveEvent *event);

   public:
      EffectRack(QWidget*, AudioTrack*, bool);
      QSize sizeHint() const;
      };

#endif

