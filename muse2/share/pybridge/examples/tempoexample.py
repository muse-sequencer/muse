import Pyro.core

muse=Pyro.core.getProxyForURI('PYRONAME://:Default.muse')

print "Tempo: " + str(muse.getTempo(0))

