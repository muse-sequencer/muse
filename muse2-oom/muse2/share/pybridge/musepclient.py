#
# Example client for MusE Pyro bridge (Python Remote Object)
#
import Pyro.core
import time

muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')
print "Current position is: " + str(muse.getCPos())
muse.startPlay()
time.sleep(1) # Sleep one second
muse.stopPlay()
print "New position is: " + str(muse.getCPos())
muse.rewindStart()
print "Pos after rewind is: " + str(muse.getCPos())
print "Lpos, Rpos: " + str(muse.getLPos()) + ":" + str(muse.getRPos())


