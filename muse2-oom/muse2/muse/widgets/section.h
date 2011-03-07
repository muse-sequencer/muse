//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: section.h,v 1.1.1.1 2003/10/27 18:54:27 wschweer Exp $
//  (C) Copyright 2002 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __SECTION_H__
#define __SECTION_H__

struct Section {
      int offset;
      unsigned len;
      int voff;
      int val;
      };

#endif

