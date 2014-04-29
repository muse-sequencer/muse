MusE midi event scripting format 0.6
====================================

= INTRODUCTION

MusE supports a simple format

= SPECIFICATION

The format of the scripts is just a set of text lines which deliberately
has been kept very brief and should be possible to handle from any programming
language, Python, Perl, C/C++, even shell script. Though for debugging
reasons an interpreted language is probably recommended.
     
There are two main requirements on scripts.

1. the actual script file must have the executable flag set, that is, it must be considered 
an executable from the perspective of the operating system.
2. a script takes it's input from a file given as argument and will send it's output
to the same file.

The tags that may occur in the file sent to the script are:
TIMESIG <n> <z>
PARTLEN <len in ticks of the current Part>
BEATLEN <len in ticks of a beat>
QUANTLEN <len in ticks of the current quantization>
NOTE <tick> <pitch> <len in ticks> <velocity>
CONTROLLER <tick> <a> <b> <c>

TIMESIG, PARTLEN, BEATLEN and QUANTLEN are there for informational purposes, to 
make some transformations possible. e.g. quantization, beat delay.

NOTE and CONTROLLER are the ones that are read back into MusE when the filter
stops executing. These may be manipulated, removed or multiplied as seen
fit by the filter.

* Note that it is generally a good idea to just pass on the lines your script
is not interested in, otherwise data may be unintentionally removed

= INSTALLATION

Scripts can be put in two different dirs.
<install_path>/share/muse/scripts
for scripts bundled
or $HOME/.config/MusE/scripts
for user created scripts

Remember that a script must have the executable bit set. This means
that the script must be executable -on it's own-. For an interpreted
language that means the file should start with a hashbang (#!) and the path
to it's interpreter.
e.g
#!/usr/bin/python

= EXAMPLES

There are a few existing scripts in this directory that can be used as 
templates for creating more advanced scripts.

- DoNothing, just reads the input file, loops through the lines and outputs
  them again.
- DoubleSpeed, reads all notes and outputs them with all ticks divided
  in half.
- RemoveShortEvents, a script with a GUI in PyQt which removes events 
  shorter then the set note length

For the moment there are only scripts written in python in the standard
installation. 

= DEBUGGING

The process of debugging a script may seem daunting. There is no immediate way
to retrieve information about the internals of the script.

Currently the standard output is suppressed from the scripts so while running
from MusE the script won't make a peep in the console.
if MusE is started with -D argument the contents of the data file is written
to standard out both before it enters the script and after it returns, this 
can be used as a means for communication.
The file containing the script data is normally removed but when running 
MusE with -D it will be kept.
So, when the file exists you can use it and run the script from a command 
line, remember though that the file will be overwritten by the script.

Running the script like this makes it a bit easier:
cp origData testData && MyTestScript testData
Then the data can be compared to the original and prints in the script can
be read on the commandline.

= END

