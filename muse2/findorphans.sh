function f() {  # find
        ext=$1
        shift 1
        if [ "$ext" != "_" ]; then
                find . -iname "*.$ext" -print0 | xargs -0 grep "$@";
        else
                find . -print0 | xargs -0 grep "$@";
        fi
}


for i in `find . -name '*.cpp' `; do
	filename=`echo $i | sed -e 's,^.*/\([^/]*\)$,\1,g'`
	basename=`echo $i | sed -e 's,^\(.*/\)\([^/]*\)$,\1,g'`
#	echo $basename $filename
	(
	cd $basename; 
	result=`f txt $filename`
	if [[ x$result == x ]]; then echo ${basename}$filename is orphaned; fi;
	)
done
