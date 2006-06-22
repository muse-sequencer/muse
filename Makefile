#
#
#
#

default:
	if test ! -d build;                           \
         then                                       \
            echo "+creating build directory";       \
            mkdir build;                            \
            echo "+entering build directory";       \
            cd build;                               \
            # HACK:                                 \
            cp ../muse/all.h . ;                    \
            echo "+calling cmake" ;                 \
            cmake ../muse ;                         \
         else                                       \
            echo "+entering build directory";       \
            cd build;                               \
         fi;                                        \
      echo "+start top level make...";              \
      make -f Makefile


clean:
	-rm -rf build

dist:
	cd build; make package_source

package:
	cd build; make package


