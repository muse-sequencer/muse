all-local: link

SOFTSYNTHDIRS = $(shell find ../../synti -type d \
	\( -not -name obj \
	   -not -name CVS \
	   -not -name ".*" \
	   -not -name synti \))

SOFTSYNTHS = $(notdir $(SOFTSYNTHDIRS))

link:
	find -type l -exec rm {} \;
	for synth in $(SOFTSYNTHS); do \
	  if test -e ../../synti/$${synth}/$${synth}; then \
	    ln -s ../../synti/$${synth}/$${synth} .; \
	  fi; \
	  if test -e ../../synti/$${synth}/$${synth}.so; then \
	    ln -s ../../synti/$${synth}/$${synth}.so .; \
	  fi; \
	done

CLEANFILES = $(SOFTSYNTHS) $(SOFTSYNTHS:%=%.so)

