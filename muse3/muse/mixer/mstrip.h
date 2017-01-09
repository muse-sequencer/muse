//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mstrip.h,v 1.4.2.4 2009/10/25 19:26:29 lunar_shuttle Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 - 2017 Tim E. Real (terminator356 on sourceforge)
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

#ifndef __MSTRIP_H__
#define __MSTRIP_H__

#include "type_defs.h"
#include "strip.h"

class QWidget;
class QAction;
class QDialog;
class QString;
class QResizeEvent;
class QString;
class QPoint;
class QVBoxLayout;

namespace MusECore {
class MidiTrack;
}

namespace MusEGui {
class DoubleLabel;
class Slider;
class CompactSlider;
class CompactPatchEdit;
class ScrollArea;
class ElidedLabel;
class CompactToolButton;


//---------------------------------------------------------
//   MidiComponentRack
//---------------------------------------------------------

class MidiComponentRack : public ComponentRack
{
  Q_OBJECT
    
  public:      
      // Type of component.
      //enum MStripComponentType { type = userComponent };
      // Possible widget types.
      enum MStripComponentWidgetType { mStripCompactPatchEditComponentWidget = userComponentWidget };
      // Some controller types.
      enum MStripControlType { mStripPanControl = 0, mStripVarSendControl, mStripRevSendControl, mStripChoSendControl, mStripProgramControl };
      // Possible component properties.
      enum MStripComponentProperties 
      {
        mStripInstrumentProperty = userComponentProperty,
        mStripTranspProperty,
        mStripDelayProperty, 
        mStripLenProperty, 
        mStripVeloProperty, 
        mStripComprProperty
      };
      
  protected:
    MusECore::MidiTrack* _track;
    
    // Creates a new component widget from the given desc. Called by newComponent().
    // Connects known widget types' signals to slots.
    virtual void newComponentWidget( ComponentDescriptor* desc, const ComponentWidget& before = ComponentWidget() );
    // Scan and automatically remove missing / add new controllers.
    void scanControllerComponents();
    // Set component colours upon config changed.
    void setComponentColors();
    
  protected slots:
    virtual void controllerChanged(int val, int id);
    virtual void controllerChanged(double val, int id);
    virtual void controllerChanged(double val, bool isOff, int id, int scrollMode);
    virtual void controllerMoved(double, int, bool);
    virtual void controllerPressed(int);
    virtual void controllerReleased(int);
    virtual void controllerRightClicked(QPoint, int);
    virtual void propertyChanged(double val, bool isOff, int id, int scrollMode);
    virtual void propertyMoved(double, int, bool);
    virtual void propertyPressed(int);
    virtual void propertyReleased(int);
    virtual void propertyRightClicked(QPoint, int);
    virtual void labelPropertyPressed(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    virtual void labelPropertyReleased(QPoint p, int id, Qt::MouseButtons buttons, Qt::KeyboardModifiers keys);
    void patchPopup(QPoint p);
    void patchPopupActivated(QAction*);
    void instrPopup(QPoint p);

    
    virtual void patchEditNameClicked(QPoint p, int id);
    
   public slots:
    virtual void configChanged();
    virtual void songChanged(MusECore::SongChangedFlags_t);
    
  public:
    MidiComponentRack(MusECore::MidiTrack* track, int id = -1, QWidget* parent = 0, Qt::WindowFlags f = 0);
    
    MusECore::MidiTrack* track() { return _track; }
    // Adds a component to the layout and the list. Creates a new component using 
    //  the given desc values if the desc widget is not given.
    virtual void newComponent( ComponentDescriptor* desc, const ComponentWidget& before = ComponentWidget() );
    // Updates all the components, for example updating the values of sliders.
    void updateComponents();
    // Enable or disable all the aux components in this rack.
    void setAuxEnabled(bool enable);
};

//---------------------------------------------
// CompactPatchEditComponentDescriptor
//  Class defining a CompactPatchEdit to be added to a rack layout.
//---------------------------------------------

class CompactPatchEditComponentDescriptor : public ComponentDescriptor
{
  public:
    // Return value pointer created by the function, corresponding to a ComponentWidgetType:
    CompactPatchEdit* _compactPatchEdit;
    
    double _initVal;
    bool _isOff;
    QColor _readoutColor;

    // Slots:
    const char* _patchEditChangedSlot;
    const char* _patchEditValueRightClickedSlot;
    const char* _patchEditNameClickedSlot;
    const char* _patchEditNameRightClickedSlot;
    
  public:        
    CompactPatchEditComponentDescriptor() :
      ComponentDescriptor(ComponentRack::CompactSliderComponentWidget,
                          ComponentRack::controllerComponent),
      _compactPatchEdit(0),
      _initVal(0.0),
      _isOff(false),
      _patchEditChangedSlot(0), _patchEditValueRightClickedSlot(0), _patchEditNameClickedSlot(0), _patchEditNameRightClickedSlot(0)
      { }
                            
    CompactPatchEditComponentDescriptor(
      ComponentWidget::ComponentType componentType,
      const char* objName = 0,
      int index = 0,
      const QString& toolTipText = QString(),
      const QString& label = QString(),
      const QColor& readoutColour = QColor(),
      bool enabled = true,
      double initVal = 0.0,
      bool isOff = false,
      const char* patchEditChangedSlot = 0,
      const char* patchEditSliderRightClickedSlot = 0,
      const char* patchEditNameClickedSlot = 0,
      const char* patchEditNameRightClickedSlot = 0
    )
    : ComponentDescriptor(MidiComponentRack::mStripCompactPatchEditComponentWidget,
                          componentType,
                          objName,
                          index,
                          toolTipText,
                          label,
                          readoutColour,
                          enabled
                         ),
      _compactPatchEdit(0),
      _initVal(initVal),
      _isOff(isOff),
      _patchEditChangedSlot(patchEditChangedSlot),
      _patchEditValueRightClickedSlot(patchEditSliderRightClickedSlot),
      _patchEditNameClickedSlot(patchEditNameClickedSlot),
      _patchEditNameRightClickedSlot(patchEditNameRightClickedSlot)
      { }
};


//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

class MidiStrip : public Strip {
      Q_OBJECT

  public:      
      // ID numbers for each rack in this strip.
      enum MStripRacks { mStripUpperRack = 0, mStripInfoRack = 1, mStripLowerRack = 2 };
      
  private:
      GridPosStruct _preScrollAreaPos_A;
      GridPosStruct _preScrollAreaPos_B;
      GridPosStruct _infoSpacerTop;
      GridPosStruct _infoSpacerBottom;
      GridPosStruct _propertyRackPos;
      GridPosStruct _sliderPos;
      GridPosStruct _sliderLabelPos;
      GridPosStruct _postScrollAreaPos_A;
      GridPosStruct _postScrollAreaPos_B;
      GridPosStruct _offPos;
      GridPosStruct _recPos;
      GridPosStruct _mutePos;
      GridPosStruct _soloPos;
      GridPosStruct _routesPos;
      GridPosStruct _automationPos;
      GridPosStruct _rightSpacerPos;
      
      MusEGui::Slider* slider;
      MusEGui::DoubleLabel* sl;
      QToolButton* off;
      
      MidiComponentRack* _upperRack;
      MidiComponentRack* _lowerRack;
      MidiComponentRack* _infoRack;
      
      // Whether the layout is in mode A (normal, racks on left) or B (racks on right).
      bool _isExpanded;
      // Current local state of knobs versus sliders preference global setting.
      bool _preferKnobs;

      CompactToolButton* _midiThru;
      int _heartBeatCounter;
      
      int volume;
      bool inHeartBeat;

      void updateControls();
      void updateOffState();
      void updateRackSizes(bool upper, bool lower);
   
   protected:
      void resizeEvent(QResizeEvent*);
     
   private slots:
      void midiThruToggled(bool);
      void offToggled(bool);
      void iRoutePressed();
      void oRoutePressed();
      void setVolume(double);
      void ctrlChanged(double v, bool off, int num);
      
      void volLabelDoubleClicked();
      void volLabelChanged(double);
      void controlRightClicked(QPoint, int);

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void songChanged(MusECore::SongChangedFlags_t);
      virtual void configChanged();
      void incVolume(int v);
      void incPan(int v);
   public:
      MidiStrip(QWidget* parent, MusECore::MidiTrack*, bool hasHandle = false, bool isEmbedded = true);
      
      static const int xMarginHorSlider;
      static const int yMarginHorSlider;
      static const int upperRackSpacerHeight;
      static const int rackFrameWidth;

      // Destroy and rebuild strip components.
      virtual void buildStrip();
      };

} // namespace MusEGui

#endif



