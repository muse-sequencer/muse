README.txt
----------

Graphical frontend and built-in softsynth (MusE Experimental Soft Synth) for MusE, based on Fluidsynth
(http://www.fluidsynth.org).

Features:
---------
- Loading/unloading of soundfonts
- Easy control of fluidsynth's send effects and their parameters
- Mapping of soundfonts to fluidsynth channels
- Stores all settings in the current project file and automatically loads all effect parameters,
  soundfonts, channel settings and presets when re-opening the project.
- Makes it possible to use several soundfonts in one single fluidsynth instance (thereby reducing CPU usage since they share
  the same send effects)


Changelog/History
-----------------
040524
- Err... Fount out that this changelog is neglected. See ../../Changelog.txt instead.
031019
- Bugfixes and changes in storing/retrieving init parameters (Mathias Lundgren)
031009
- Unloading of soundfonts works (Mathias Lundgren)
- Last dir stored in project-file (Mathias Lundgren)
- Ordinary controller-events enabled (Mathias Lundgren)
031008
- Mapping of soundfonts to fluidchannels and selection of patches implemented. (Mathias Lundgren)
- Permanent storage of channels & patches. Extended GUI. (Mathias Lundgren)
031002
- Various communication problems fixed between GUI and client (Mathias Lundgren)
- Storage of synth parameters and soundfonts enabled (Mathias Lundgren/Robert Jonsson)

0309xx
- Problem with loading of soundfonts resulting in Jack timeout fixed by moving loading of soundfonts to separate thread. (Robert Jonsson)

Original code written by Robert Ham (no information about the history of his work)


Known problems/TODO:
--------------------------------------------------------------
* Turning on the chorus and/or modifying chorus parameters locks the client.
* Illegal chorus parameters can be sent to fluidsynth.
* Drum patches (lbank=128) not implemented yet
