//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: mtscale.h,v 1.3 2004/04/27 22:27:06 spamatica Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __PCSCALE_H__
#define __PCSCALE_H__

#include "view.h"
#include "pianoroll.h"
#include "midictrl.h"

//---------------------------------------------------------
//   PCScale
//    program change scale for midi track
//---------------------------------------------------------

class PCScale : public View {
      Q_OBJECT
	  PianoRoll* currentEditor;
      int* raster;
      unsigned pos[4];
      int button;
      bool barLocator;
      bool waveMode;


   private slots:
      void songChanged(int);

   protected:
      virtual void pdraw(QPainter&, const QRect&);
      virtual void viewMousePressEvent(QMouseEvent* event);
      virtual void viewMouseMoveEvent(QMouseEvent* event);
      virtual void viewMouseReleaseEvent(QMouseEvent* event);
      virtual void leaveEvent(QEvent*e);

   signals:
	  void selectInstrument();
	  void addProgramChange();

   public slots:
      void setPos(int, unsigned, bool);
	  void updateProgram();

   public:
      PCScale(int* raster, QWidget* parent, PianoRoll* editor, int xscale, bool f = false);
      void setBarLocator(bool f) { barLocator = f; }
	  void setEditor(PianoRoll*);
	  PianoRoll* getEditor() { return currentEditor; }
      };
#endif

