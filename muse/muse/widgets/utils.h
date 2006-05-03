//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: utils.h,v 1.12 2006/01/12 14:49:13 wschweer Exp $
//  (C) Copyright 1999 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __UTILS_H__
#define __UTILS_H__

#include "gui.h"

class SimpleButton;

extern QString bitmap2String(int bm);
extern int string2bitmap(const QString& str);

extern int num2cols(int min, int max);
extern QFrame* hLine(QWidget* parent);
extern QFrame* vLine(QWidget* parent);
extern void dump(const unsigned char* p, int n);
extern double curTime();
extern SimpleButton* newMuteButton(QWidget* parent);
extern SimpleButton* newSoloButton(QWidget* parent);
extern SimpleButton* newMonitorButton(QWidget* parent);
extern SimpleButton* newDrumMapButton(QWidget* parent);
extern SimpleButton* newOffButton(QWidget* parent);
extern SimpleButton* newRecordButton(QWidget* parent);
extern SimpleButton* newAutoWriteButton(QWidget* parent);
extern SimpleButton* newAutoReadButton(QWidget* parent);
extern SimpleButton* newSyncButton(QWidget* parent);
extern SimpleButton* newPlusButton(QWidget* parent);
extern SimpleButton* newMinusButton(QWidget* parent);
extern SimpleButton* newStereoButton(QWidget* parent);

extern void fatalError(const char*);

extern QColor lineColor[splitWidth];
extern void paintHLine(QPainter& p, int x1, int x2, int y);

#endif

