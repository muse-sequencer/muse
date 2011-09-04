{
	echo 'HEADERS = \';
	find . -name '*.h' -and -not -path '*/build/*' -printf '\t%p \\\n';
	echo ' ';
	echo;
	
	
	echo 'SOURCES = \';
	find . -name '*.cpp' -and -not -path '*/build/*' -printf '\t%p \\\n';
	echo ' ';
	echo;
	
	
	echo 'FORMS = \';
	find . -name '*.ui' -and -not -path '*/build/*' -printf '\t%p \\\n';
	echo ' ';
	echo;
	
	
	echo 'TRANSLATIONS = \';
	find . -name '*.ts' -and -not -path '*/build/*' -printf '\t%p \\\n';
	echo ' ';
	echo;
} > muse.pro
