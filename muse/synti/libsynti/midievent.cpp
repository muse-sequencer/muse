//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mpevent.cpp,v 1.3 2005/05/11 14:18:48 wschweer Exp $
//
//  (C) Copyright 2002-2004 Werner Schweer (ws@seh.de)
//=========================================================

#include "midievent.h"

//---------------------------------------------------------
//   MidiEvent
//---------------------------------------------------------

MidiEvent::MidiEvent(unsigned t, int tpe, const unsigned char* data, int len)
      {
      _time = t;
      edata.setData(data, len);
      _type = tpe;
      }

