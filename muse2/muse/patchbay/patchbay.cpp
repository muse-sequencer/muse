#include "patchbay.h"

PatchBay::PatchBay (void) {
  _patchbay = new PatchBayBase (this, "patchbay");
  setCaption(tr("MusE: ALSA MIDI Patch Bay"));
}

PatchBay::~PatchBay (void) {
  if (_patchbay) delete _patchbay;
}
