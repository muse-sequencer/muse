//=============================================================================
//  Awl
//  Audio Widget Library
//  $Id:$
//
//  Copyright (C) 1999-2011 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License
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
//=============================================================================

#ifndef __AWLPLUGIN_H__
#define __AWLPLUGIN_H__

#include <QtDesigner/QDesignerCustomWidgetInterface>

//---------------------------------------------------------
//   AwlPlugin
//---------------------------------------------------------

class AwlPlugin : public QDesignerCustomWidgetInterface {
	Q_INTERFACES(QDesignerCustomWidgetInterface)
      bool m_initialized;

   public:
    	AwlPlugin() : m_initialized(false) { }
	bool isContainer() const     { return false;         }
    	bool isInitialized() const   { return m_initialized; }
    	QIcon icon() const           { return QIcon();       }
    	virtual QString codeTemplate() const { return QString();     }
    	QString whatsThis() const    { return QString();     }
    	QString toolTip() const      { return QString();     }
    	QString group() const        { return "MusE Awl Widgets"; }
	void initialize(QDesignerFormEditorInterface *) {
		if (m_initialized)
			return;
		m_initialized = true;
		}
      };

//---------------------------------------------------------
//   KnobPlugin
//---------------------------------------------------------

class KnobPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	KnobPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return QString("awl/knob.h"); }
      QString name() const        { return "Awl::Knob"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   VolKnobPlugin
//---------------------------------------------------------

class VolKnobPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	VolKnobPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/volknob.h"; }
      QString name() const { return "Awl::VolKnob"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   SliderPlugin
//---------------------------------------------------------

class SliderPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	SliderPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/slider.h"; }
      QString name() const { return "Awl::Slider"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   VolSliderPlugin
//---------------------------------------------------------

class VolSliderPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	VolSliderPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/volslider.h"; }
      QString name() const { return "Awl::VolSlider"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   MeterSliderPlugin
//---------------------------------------------------------

class MeterSliderPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	MeterSliderPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/mslider.h"; }
      QString name() const { return "Awl::MeterSlider"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   PosEditPlugin
//---------------------------------------------------------

class PosEditPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	PosEditPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/posedit.h"; }
      QString name() const { return "Awl::PosEdit"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   PosLabelPlugin
//---------------------------------------------------------

class PosLabelPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	PosLabelPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/poslabel.h"; }
      QString name() const { return "Awl::PosLabel"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   TempoEditPlugin
//---------------------------------------------------------

class TempoEditPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	TempoEditPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/tempoedit.h"; }
      QString name() const { return "Awl::TempoEdit"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   TempoLabelPlugin
//---------------------------------------------------------

class TempoLabelPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	TempoLabelPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/tempolabel.h"; }
      QString name() const { return "Awl::TempoLabel"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   CheckBoxPlugin
//---------------------------------------------------------

class CheckBoxPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	CheckBoxPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/checkbox.h"; }
      QString name() const { return "Awl::CheckBox"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   ComboBoxPlugin
//---------------------------------------------------------

class ComboBoxPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	ComboBoxPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/combobox.h"; }
      QString name() const { return "Awl::ComboBox"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   FloatEntryPlugin
//---------------------------------------------------------

class FloatEntryPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	FloatEntryPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/floatentry.h"; }
      QString name() const { return "Awl::FloatEntry"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   PanKnobPlugin
//---------------------------------------------------------

class PanKnobPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	PanKnobPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/panknob.h"; }
      QString name() const { return "Awl::PanKnob"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   MidiPanKnobPlugin
//---------------------------------------------------------

class MidiPanKnobPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	MidiPanKnobPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/midipanknob.h"; }
      QString name() const { return "Awl::MidiPanKnob"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   DrawbarPlugin
//---------------------------------------------------------

class DrawbarPlugin : public QObject, public AwlPlugin {
      Q_OBJECT

   public:
     	DrawbarPlugin(QObject* parent = 0) : QObject(parent) {}
      QString includeFile() const { return "awl/drawbar.h"; }
      QString name() const { return "Awl::Drawbar"; }
      QWidget* createWidget(QWidget* parent);
      };

//---------------------------------------------------------
//   AwlPlugins
//---------------------------------------------------------

class AwlPlugins : public QObject, public QDesignerCustomWidgetCollectionInterface {
      Q_OBJECT
      Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

   public:
      QList<QDesignerCustomWidgetInterface*> customWidgets() const;
      };

#endif

