all-local: link

PLUGINDIRS = $(shell find ../../plugins -type d \
	\( -not -name obj \
	   -not -name CVS \
	   -not -name ".*" \
	   -not -name plugins \))

PLUGINS = $(notdir $(PLUGINDIRS))

link:
	find -type l -exec rm {} \;
	for plugin in $(PLUGINS); do \
	  if test -e ../../plugins/$${plugin}/$${plugin}; then \
	    ln -s ../../plugins/$${plugin}/$${plugin} .; \
	  fi; \
	  if test -e ../../plugins/$${plugin}/$${plugin}.so; then \
	    ln -s ../../plugins/$${plugin}/$${plugin}.so .; \
	  fi; \
	done

CLEANFILES = $(PLUGINS) $(PLUGINS:%=%.so)

