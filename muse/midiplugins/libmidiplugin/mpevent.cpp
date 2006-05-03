//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.cpp,v 1.1 2005/05/08 17:01:30 wschweer Exp $
//
//  (C) Copyright 2002-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "mpevent.h"

//---------------------------------------------------------
//   MEvent
//---------------------------------------------------------

MEvent::MEvent(unsigned t, int tpe, const unsigned char* data, int len)
      {
      _time = t;
      edata.setData(data, len);
      _type = tpe;
      }

