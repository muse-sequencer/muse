//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: helper.h,v 1.1.1.1 2003/10/27 18:52:11 wschweer Exp $
//  (C) Copyright 2003 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __HELPER_H__
#define __HELPER_H__

#include <QString>

class Part;

extern QString pitch2string(int v);

Part* partFromSerialNumber(int serial);

#endif

