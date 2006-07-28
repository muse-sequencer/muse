//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: waveedit.h,v 1.19 2006/02/01 22:44:40 wschweer Exp $
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __WAVE_EDIT_H__
#define __WAVE_EDIT_H__

#include "midiedit/midieditor.h"

class PartList;
class WaveView;
class ScrollScale;
class SNode;

namespace Awl {
      class PosLabel;
      };
using Awl::PosLabel;

//---------------------------------------------------------
//   WaveEdit
//---------------------------------------------------------

class WaveEdit : public Editor {
      Q_OBJECT

      PartList* _parts;
      WaveView* view;
      QToolBar* tools;
      QToolBar* tb1;
      QAction* solo;
      PosLabel* pos1;
      PosLabel* pos2;

      static int _widthInit, _heightInit;

      virtual void keyPressEvent(QKeyEvent*);

      QMenu* menuFunctions, *select, *menuGain;

   private slots:
      void cmd(QAction*);
      void setTime(unsigned t);
      void soloChanged(SNode* s);
      void soloChanged(bool flag);

   public slots:
      void configChanged();

   public:
      WaveEdit(PartList*, bool);
      ~WaveEdit();
      PartList* parts() const { return _parts; }

      enum { CMD_MUTE=0, CMD_NORMALIZE, CMD_FADE_IN, CMD_FADE_OUT, CMD_REVERSE,
             CMD_GAIN_FREE, CMD_GAIN_200, CMD_GAIN_150, CMD_GAIN_75, CMD_GAIN_50, CMD_GAIN_25,
             CMD_EDIT_EXTERNAL,
             CMD_SELECT_ALL, CMD_SELECT_NONE };
      static int initWidth, initHeight;
      static const int INIT_WIDTH  = 650;
      static const int INIT_HEIGHT = 450;
      };

static const bool INIT_FOLLOW = false;

#endif

