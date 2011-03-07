#ifndef __APPEARANCE_H__
#define __APPEARANCE_H__

#include "appearancebase.h"
#include <qfont.h>

class MusE;
class Arranger;
class QColor;
class GlobalConfigValues;

//---------------------------------------------------------
//   Appearance Dialog
//---------------------------------------------------------

class Appearance : public AppearanceDialogBase {
      Arranger* arr;
      QColor* color;
      QString currentBg;
      GlobalConfigValues* config;

      Q_OBJECT
      void updateFonts();
      void updateColor();

   private slots:
      void apply();
      void ok();
      void cancel();
      void configBackground();
      void clearBackground();
      void colorItemSelectionChanged();
      void browseFont(int);
      void browseFont0();
      void browseFont1();
      void browseFont2();
      void browseFont3();
      void browseFont4();
      void browseFont5();
      void browseFont6();
      void rsliderChanged(int);
      void gsliderChanged(int);
      void bsliderChanged(int);
      void hsliderChanged(int);
      void ssliderChanged(int);
      void vsliderChanged(int);
      void addToPaletteClicked();
      void paletteClicked(int);

   public:
      Appearance(Arranger*, QWidget* parent=0, const char* name=0);
      ~Appearance();
      void resetValues();
      };

#endif
