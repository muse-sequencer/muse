//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: astrip.h,v 1.8.2.6 2009/11/14 03:37:48 terminator356 Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011-2016 Tim E. Real (terminator356 on sourceforge)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __ASTRIP_H__
#define __ASTRIP_H__

#include "type_defs.h"
#include "strip.h"
#include "clipper_label.h"

class QButton;
class QHBoxLayout;
class QVBoxLayout;
class QColor;
class QWidget;

namespace MusECore {
class AudioTrack;
}

namespace MusEGui {
class DoubleLabel;
class EffectRack;
class Knob;
class Slider;
class CompactSlider;
class IconButton;

//---------------------------------------------------------
//   AudioComponentRack
//---------------------------------------------------------

class AudioComponentRack : public ComponentRack
{
  Q_OBJECT

  public:
      // Type of component.
      enum AStripComponentType { aStripAuxComponent = userComponent };
      // Possible widget types.
      //enum AStripComponentWidgetType { type = userComponentWidget };
      // Possible component properties.
      enum AStripComponentProperties { aStripGainProperty = userComponentProperty };

  protected:
    MusECore::AudioTrack* _track;
    bool _manageAuxs;

    // Creates a new component widget from the given desc. Called by newComponent().
    // Connects known widget types' signals to slots.
    //virtual void newComponentWidget( ComponentDescriptor* desc, const ComponentWidget& before = ComponentWidget() );
    // Scan and automatically remove missing / add new controllers.
    void scanControllerComponents();
    // Scan and automatically remove missing / add new aux.
    void scanAuxComponents();
    // Set component colours upon config changed.
    void setComponentColors();

  protected slots:
    virtual void controllerChanged(double val, bool isOff, int id, int scrollMode);
    virtual void controllerMoved(double, int, bool);
    virtual void controllerPressed(double, int);
    virtual void controllerReleased(double, int);
    virtual void controllerRightClicked(QPoint, int);
    virtual void propertyChanged(double val, bool isOff, int id, int scrollMode);
    virtual void propertyMoved(double, int, bool);
    virtual void propertyPressed(double, int);
    virtual void propertyReleased(double, int);
    virtual void propertyRightClicked(QPoint, int);
    virtual void auxChanged(double val, bool isOff, int id, int scrollMode);
    virtual void auxMoved(double, int, bool);
    virtual void auxPressed(double, int);
    virtual void auxReleased(double, int);
    virtual void auxRightClicked(QPoint p, int);

  public slots:
    virtual void configChanged();
    virtual void songChanged(MusECore::SongChangedStruct_t);

  public:
    AudioComponentRack(MusECore::AudioTrack* track, int id = -1, bool _manageAuxs = false, QWidget* parent = 0, Qt::WindowFlags f = 0);

    MusECore::AudioTrack* track() { return _track; }
    // Adds a component to the layout and the list. Creates a new component using
    //  the given desc values if the desc widget is not given.
    virtual void newComponent( ComponentDescriptor* desc, const ComponentWidget& before = ComponentWidget() );
    // Updates all the components, for example updating the values of sliders.
    void updateComponents();
    // Enable or disable all the aux components in this rack.
    void setAuxEnabled(bool enable);
};


//---------------------------------------------------------
//   AudioStripProperties
//---------------------------------------------------------

class AudioStripProperties : QWidget {
      Q_OBJECT

//    Q_PROPERTY(QColor bgColor READ bgColor WRITE setBgColor)
    Q_PROPERTY(int sliderRadius READ sliderRadius WRITE setSliderRadius)
    Q_PROPERTY(int sliderRadiusHandle READ sliderRadiusHandle WRITE setSliderRadiusHandle)
    Q_PROPERTY(int sliderHandleHeight READ sliderHandleHeight WRITE setSliderHandleHeight)
    Q_PROPERTY(int sliderHandleWidth READ sliderHandleWidth WRITE setSliderHandleWidth)
    Q_PROPERTY(int sliderGrooveWidth READ sliderGrooveWidth WRITE setSliderGrooveWidth)
    Q_PROPERTY(bool sliderFillOver READ sliderFillOver WRITE setSliderFillOver)
    Q_PROPERTY(bool sliderUseGradient READ sliderUseGradient WRITE setSliderUseGradient)
    Q_PROPERTY(int sliderScalePos READ sliderScalePos WRITE setSliderScalePos)
    Q_PROPERTY(int meterWidth READ meterWidth WRITE setMeterWidth)

//    QColor _bgColor;
    int _sliderRadius;
    int _sliderRadiusHandle;
    int _sliderHandleHeight;
    int _sliderHandleWidth;
    int _sliderGrooveWidth;
    bool _sliderFillOver;
    bool _sliderUseGradient;
    int _sliderScalePos;
    int _meterWidth;

public:
    AudioStripProperties();

//    QColor bgColor() const { return _bgColor; }
//    void setBgColor(const QColor c) { _bgColor = c; }

    int sliderRadius() const { return _sliderRadius; }
    void setSliderRadius(int radius) { _sliderRadius = radius; }
    int sliderRadiusHandle() const { return _sliderRadiusHandle; }
    void setSliderRadiusHandle(int radiusHandle) { _sliderRadiusHandle = radiusHandle; }

    int sliderHandleHeight() const { return _sliderHandleHeight; }
    void setSliderHandleHeight(int h) { _sliderHandleHeight = h; }
    int sliderHandleWidth() const { return _sliderHandleWidth; }
    void setSliderHandleWidth(int w) { _sliderHandleWidth = w; }
    int sliderGrooveWidth() const { return _sliderGrooveWidth; }
    void setSliderGrooveWidth(int w) { _sliderGrooveWidth = w; }

    bool sliderFillOver() const { return _sliderFillOver; }
    void setSliderFillOver(bool b) { _sliderFillOver = b; }
    bool sliderUseGradient() const { return _sliderUseGradient; }
    void setSliderUseGradient(bool b) { _sliderUseGradient = b; }

    int sliderScalePos() const { return _sliderScalePos; }
    void setSliderScalePos(int p) { _sliderScalePos = p; }

    int meterWidth() const { return _meterWidth; }
    void setMeterWidth(int w) { _meterWidth = w; }
};

//---------------------------------------------------------
//   AudioStrip
//---------------------------------------------------------

class AudioStrip : public Strip {
      Q_OBJECT

  public:
      // ID numbers for each rack in this strip.
      enum AStripRacks { aStripUpperRack = 0, aStripInfoRack = 1, aStripLowerRack = 2 };

  private:
      GridPosStruct _preScrollAreaPos_A;
      //GridPosStruct _preScrollAreaPos_B;
      GridPosStruct _effectRackPos;
      GridPosStruct _stereoToolPos;
      GridPosStruct _preToolPos;
      GridPosStruct _gainToolPos;
      GridPosStruct _infoSpacerTop;
      GridPosStruct _infoSpacerBottom;
      GridPosStruct _propertyRackPos;
      GridPosStruct _sliderPos;
      GridPosStruct _sliderLabelPos;
      GridPosStruct _postScrollAreaPos_A;
      //GridPosStruct _postScrollAreaPos_B;
      GridPosStruct _offPos;
      GridPosStruct _recPos;
      GridPosStruct _mutePos;
      GridPosStruct _soloPos;
      //GridPosStruct _routesPos;
      GridPosStruct _inRoutesPos;
      GridPosStruct _outRoutesPos;
      GridPosStruct _automationPos;
      //GridPosStruct _rightSpacerPos;
      GridPosStruct _offMonRecPos;

      int _sliderRadius;
      int _sliderRadiusHandle;
      int _sliderHandleHeight;
      int _sliderHandleWidth;
      int _sliderGrooveWidth;
      bool _sliderFillOver;
      bool _sliderUseGradient;
      int _sliderScalePos;
      int _meterWidth;

      int channel;
      MusEGui::Slider* slider;
      MusEGui::DoubleLabel* sl;
      EffectRack* rack;

      AudioComponentRack* _upperRack;
      AudioComponentRack* _lowerRack;
//      AudioComponentRack* _infoRack;

      // Whether the layout is in mode A (normal, racks on left) or B (racks on right).
      bool _isExpanded;
      // Current local state of knobs versus sliders preference global setting.
      bool _preferKnobs;

      IconButton* _recMonitor;
      IconButton* stereo;
      IconButton* pre;
      IconButton* off;

      double volume;
      bool _volPressed;

      ClipperLabel* _clipperLabel[MusECore::MAX_CHANNELS];
      QHBoxLayout* _clipperLayout;

      void setClipperTooltip(int ch);

      void updateOffState();
      void updateVolume();
      void updateChannels();
      void updateRackSizes(bool upper, bool lower);

   private slots:
      void recMonitorToggled(bool);
      void stereoToggled(bool);
      void preToggled(bool);
      void offToggled(bool);
      void iRoutePressed();
      void oRoutePressed();
      void volumeMoved(double val, int id, bool shift_pressed);
      void volumeChanged(double val, int id, int scrollMode);
      void volumePressed(double val, int id);
      void volumeReleased(double val, int id);
      void volLabelChanged(double);
      void volumeRightClicked(QPoint);
      void resetClipper();

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void configChanged();
      virtual void songChanged(MusECore::SongChangedStruct_t);
      void incVolume(int v);
      void incPan(int v);

   public:
      AudioStrip(QWidget* parent, MusECore::AudioTrack*, bool hasHandle = false, bool isEmbedded = true);
      virtual ~AudioStrip();

      static const double volSliderStep;
      static const double volSliderMax;
      static const int    volSliderPrec;

      static const double auxSliderStep;
      static const double auxSliderMax;
      static const int    auxSliderPrec;

      static const double gainSliderStep;
      static const double gainSliderMin;
      static const double gainSliderMax;
      static const int    gainSliderPrec;

      static const int xMarginHorSlider;
      static const int yMarginHorSlider;
      static const int upperRackSpacerHeight;
      static const int rackFrameWidth;

      // Destroy and rebuild strip components.
      virtual void buildStrip();

      // Sets up tabbing for the entire strip.
      // Accepts a previousWidget which can be null and returns the last widget in the strip,
      //  which allows chaining other widgets.
      virtual QWidget* setupComponentTabbing(QWidget* previousWidget = 0);
      };

} // namespace MusEGui

#endif

