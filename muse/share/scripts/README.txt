MusE midi event scripting format 0.5

Some information for the budding script writer, here is some info 
about the format currently used.
     
Scripts can be put in two different dirs.
<install_path>/share/muse/scripts
for scripts bundled
or $HOME/.muse/scripts
for user created scripts

There are two main requirements on scripts.

1. a script must have the executable flag set, that is, it must be considered 
an executable from the perspective of the operating system.
2. a script shall take an input file as argument and will update this
file with the sought output.

The tags that may occur in the file sent to the script are:
PARTLEN <len in ticks>
BEATLEN <len in ticks>
QUANTLEN <len in ticks>
NOTE <tick> <pitch> <len in ticks> <velocity>
CONTROLLER <tick> <a> <b> <c>

PARTLEN, BEATLEN and QUANTLEN are there for informational purposes, to 
make some transformations possible. e.g. quantization, beat delay.

NOTE and CONTROLLER are the ones that are read back into MusE when the filter
stops executing. These may be manipulated, removed or multiplied as seen
fit by the filter.
-- Note that it is a good idea to just pass on the lines your script is not
interested in, otherwise data may unintentionally be removed --

A short example in pyton that does nothing but pass on output from input 
to output is available in script DoNothing
