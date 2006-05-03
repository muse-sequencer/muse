//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midirack.h,v 1.7 2005/12/28 13:13:26 wschweer Exp $
//
//  (C) Copyright 2005 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MIDIRACK_H__
#define __MIDIRACK_H__

class MidiTrackBase;

//---------------------------------------------------------
//   MidiRack
//---------------------------------------------------------

class MidiRack : public QListWidget {
      MidiTrackBase* track;
      Q_OBJECT

      virtual void contextMenuEvent(QContextMenuEvent*);

   private slots:
      void doubleClicked(QListWidgetItem*);
      void songChanged(int);

   public:
      MidiRack(QWidget*, MidiTrackBase* t);
      QSize sizeHint() const;
      };

#endif

