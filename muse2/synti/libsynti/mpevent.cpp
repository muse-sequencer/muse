//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.cpp,v 1.1 2004/02/12 18:30:30 wschweer Exp $
//
//  (C) Copyright 2002-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "mpevent.h"

//---------------------------------------------------------
//   MEvent
//---------------------------------------------------------

MEvent::MEvent(unsigned t, int port, int tpe, const unsigned char* data, int len)
      {
      _time = t;
      _port = port;
      edata.setData(data, len);
      _type = tpe;
      }

