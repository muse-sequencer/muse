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
for i in range(0,10):
      muse.setMute("Strings", False)
      muse.setMute("Lead1", True)
      time.sleep(1)
      muse.setMute("Strings", True)
      muse.setMute("Lead1", False)
      time.sleep(1)

