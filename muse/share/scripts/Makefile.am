EXTRA_DIST = DoNothing RemoveShortEvents DoubleSpeed ConstantLength SwingQuantize1

#musemidiscriptspkgdata_DATA = $(EXTRA_DIST)

#musemidiscriptspkgdatadir = $(pkgdatadir)/scripts

install-data-local: 
	# mkdir -p $(pkgdatadir)/scripts/
	# cp $(EXTRA_DIST) $(pkgdatadir)/scripts/
	# chmod a+x $(pkgdatadir)/scripts/*
	mkdir -p $(DESTDIR)/$(pkgdatadir)/scripts/
	cp $(EXTRA_DIST) $(DESTDIR)/$(pkgdatadir)/scripts/
	chmod a+x $(DESTDIR)/$(pkgdatadir)/scripts/*

