//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: mstrip.h,v 1.4.2.4 2009/10/25 19:26:29 lunar_shuttle Exp $
//
//  (C) Copyright 2000-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2011 Tim E. Real (terminator356 on sourceforge)
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

// #include <QFrame>
//#include <QVBoxLayout>

#include "type_defs.h"
#include "strip.h"
//#include <QLabel>

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
// class Knob;
class Slider;
class CompactSlider;
class CompactPatchEdit;
class TransparentToolButton;
class ScrollArea;
class ElidedLabel;
class CompactToolButton;

//class CompactControllerRack;

// //---------------------------------------------------------
// //   RackLayout
// //---------------------------------------------------------
// 
// class RackLayout : public QVBoxLayout {
//       Q_OBJECT
//       
//   private:
//     int _minimumHeight;
//     
//   public:
//     RackLayout(int minimumHeight = 0) : QVBoxLayout(), _minimumHeight(minimumHeight) { }
//     RackLayout(QWidget* parent, int minimumHeight = 0) : QVBoxLayout(parent), _minimumHeight(minimumHeight) { }
//     
//     QSize minimumSize() const { return QSize(QVBoxLayout::minimumSize().width(), _minimumHeight); }
//     void setMinimumSize(int height) { _minimumHeight = height; activate(); }
// };

// //---------------------------------------------------------
// //   MidiControllerRack
// //---------------------------------------------------------
// 
// class MidiControllerRack : public QFrame {
//       Q_OBJECT
//       
// //   private:
// //     int _minimumHeight;
//     
//   public:
//     MidiControllerRack(int minimumHeight = 0) : QVBoxLayout(), _minimumHeight(minimumHeight) { }
//     RackLayout(QWidget* parent, int minimumHeight = 0) : QVBoxLayout(parent), _minimumHeight(minimumHeight) { }
//     
//     QSize minimumSize() const { return QSize(QVBoxLayout::minimumSize().width(), _minimumHeight); }
//     void setMinimumSize(int height) { _minimumHeight = height; activate(); }
// };



//---------------------------------------------------------
//   MidiStrip
//---------------------------------------------------------

class MidiStrip : public Strip {
      Q_OBJECT

  private:
      // REMOVE Tim. Trackinfo. Added.
      enum ControlType { KNOB_PAN = 0, KNOB_VAR_SEND, KNOB_REV_SEND, KNOB_CHO_SEND, KNOB_PROGRAM };
      enum PropertyType { PropertyTransp = 0, PropertyDelay, PropertyLen, PropertyVelo, PropertyCompr };
    
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
      //GridPosStruct _inRoutesPos;
      //GridPosStruct _outRoutesPos;
      GridPosStruct _routesPos;
      GridPosStruct _automationPos;
      
      MusEGui::Slider* slider;
      MusEGui::DoubleLabel* sl;
//       MusEGui::TransparentToolButton* off;
      QToolButton* off;
//       ScrollArea* _upperScrollArea;
//       ScrollArea* _lowerScrollArea;
      //CompactControllerRack* _upperScrollArea;
      //CompactControllerRack* _lowerScrollArea;
      QFrame* _upperRack;
      QFrame* _lowerRack;
      QFrame* _infoRack;
      QVBoxLayout* _upperScrollLayout;
      QVBoxLayout* _lowerScrollLayout;
      QVBoxLayout* _infoLayout;
//       RackLayout* _upperScrollLayout;
//       RackLayout* _lowerScrollLayout;
      // Whether the layout is in mode A (normal, racks on left) or B (racks on right).
      bool _isExpanded;
      

// REMOVE Tim. Trackinfo. Changed.
//       struct KNOB {
//             MusEGui::Knob* knob;
//             MusEGui::DoubleLabel* dl;
//             QLabel* lb;
//             } controller[4];    // pan variation reverb chorus
      struct CONTROL {
        CompactSlider* _control;
        CompactPatchEdit* _patchControl;
        double _cachedVal;
        CONTROL(CompactSlider* ctrl = 0, CompactPatchEdit* patchCtrl = 0, int initVal = 0) 
               : _control(ctrl), _patchControl(patchCtrl), _cachedVal(initVal) { }
      //} controller[4];    // pan variation reverb chorus
      } controller[5];    // pan variation reverb chorus patch
      
// REMOVE Tim. Trackinfo. Added.
      CompactToolButton* _midiThru;
      ElidedLabel* _instrLabel;
      int _heartBeatCounter;
      CompactSlider* _properties[5];
      
// REMOVE Tim. Trackinfo. Added.
//       int program;
      
      int volume;
// REMOVE Tim. Trackinfo. Removed.
//       int variSend;
//       int reverbSend;
//       int chorusSend;
//       int pan;
      bool inHeartBeat;

// REMOVE Tim. Trackinfo. Changed.
//       void addKnob(int idx, const QString&, const QString&, const char*, bool);
      void addController(QVBoxLayout* rackLayout, 
                         ControlType idx,
                         int midiCtrlNum,
                         const QString& toolTipText, 
                         const QString& label, 
                         const char* slot, 
                         bool enabled,
                         double initVal);
      CompactSlider* addProperty(QVBoxLayout* rackLayout, 
                       PropertyType idx, 
                       const QString& toolTipText, 
                       const QString& label, 
                       const char* slot, 
                       bool enabled,
                       double min,
                       double max, 
                       double initVal);
      
// REMOVE Tim. Trackinfo. Changed.
//       void ctrlChanged(int num, int val);
      //void ctrlChanged(int num, int val, bool off = false);
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
// REMOVE Tim. Trackinfo. Changed.
// //       void setPan(double);
// //       void setChorusSend(double);
// //       void setVariSend(double);
// //       void setReverbSend(double);
//       void setPan(double v, bool off);
//       void setChorusSend(double v, bool off);
//       void setVariSend(double v, bool off);
//       void setReverbSend(double v, bool off);
// // REMOVE Tim. Trackinfo. Added.
//       void setProgram(double v, bool off);
      void ctrlChanged(double v, bool off, int num);
      void propertyChanged(double v, bool off, int num);
      
// REMOVE Tim. Trackinfo. Changed.
//       void labelDoubleClicked(int);
      void volLabelDoubleClicked();
      void volLabelChanged(double);
      void controlRightClicked(const QPoint&, int);
      void propertyRightClicked(const QPoint&, int);
      void instrPopup();
      void patchPopup();
      void patchPopupActivated(QAction*);

   protected slots:
      virtual void heartBeat();

   public slots:
      virtual void songChanged(MusECore::SongChangedFlags_t);
      virtual void configChanged();

   public:
      MidiStrip(QWidget* parent, MusECore::MidiTrack*);
      };

} // namespace MusEGui

#endif



