#ifndef __MUSE_PATCHBAY_H__
#define __MUSE_PATCHBAY_H__

#include "cobject.h"
#include "patchbaybase.h"

class PatchBay : public TopWin {
  Q_OBJECT
  PatchBayBase * _patchbay;
  
public:
  PatchBay (void);
  ~PatchBay (void);
};

#endif /* __MUSE_PATCHBAY_H__ */