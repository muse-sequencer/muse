"""
//=========================================================
//  MusE
//  Linux Music Editor
//  (C) Copyright 2009 Mathias Gyllengahm (lunar_shuttle@users.sf.net)
//=========================================================
"""

import Pyro.core
import time
muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')
parts = muse.getParts("Track 1")

ptick = parts[0]['tick']
len = parts[0]['len']
muse.setLPos(ptick)
muse.setRPos(ptick + len)
muse.setCPos(ptick + len / 2)

songlen = muse.getSongLen()
#print "Song length: " + str(songlen)

#
# Copy first part to after current song length, thus increase song length with length of first part
#
newsonglen = songlen + parts[0]['len']
muse.setSongLen(newsonglen)
muse.createPart("Track 1", songlen + 1, parts[0]['len'], parts[0])
time.sleep(1)

lastpart = muse.getParts("Track 1").pop()
print lastpart['id']
muse.deletePart(lastpart['id'])
print muse.getDivision()

