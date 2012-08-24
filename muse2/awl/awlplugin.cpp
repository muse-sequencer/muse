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

#include "volknob.h"
#include "volslider.h"
#include "mslider.h"
#include "awlplugin.h"
#include "posedit.h"
#include "poslabel.h"
#include "tempoedit.h"
#include "tempolabel.h"
#include "checkbox.h"
#include "combobox.h"
#include "floatentry.h"
#include "panknob.h"
#include "midipanknob.h"
#include "drawbar.h"

#include <QtCore/QtPlugin>
#include <QtDesigner/QDesignerCustomWidgetInterface>

QWidget* KnobPlugin::createWidget(QWidget* parent)
	{
      return new Awl::Knob(parent);
      }
QWidget* VolKnobPlugin::createWidget(QWidget* parent)
	{
      return new Awl::VolKnob(parent);
      }
QWidget* SliderPlugin::createWidget(QWidget* parent)
	{
      return new Awl::Slider(parent);
      }
QWidget* VolSliderPlugin::createWidget(QWidget* parent)
	{
      return new Awl::VolSlider(parent);
      }
QWidget* MeterSliderPlugin::createWidget(QWidget* parent)
	{
      return new Awl::MeterSlider(parent);
      }
QWidget* PosEditPlugin::createWidget(QWidget* parent)
	{
      return new Awl::PosEdit(parent);
      }
QWidget* PosLabelPlugin::createWidget(QWidget* parent)
	{
      return new Awl::PosLabel(parent);
      }
QWidget* TempoEditPlugin::createWidget(QWidget* parent)
	{
      return new Awl::TempoEdit(parent);
      }
QWidget* TempoLabelPlugin::createWidget(QWidget* parent)
	{
      return new Awl::TempoLabel(parent);
      }
QWidget* CheckBoxPlugin::createWidget(QWidget* parent)
	{
      return new Awl::CheckBox(parent);
      }
QWidget* ComboBoxPlugin::createWidget(QWidget* parent)
	{
      return new Awl::ComboBox(parent);
      }
QWidget* FloatEntryPlugin::createWidget(QWidget* parent)
	{
      return new Awl::FloatEntry(parent);
      }
QWidget* PanKnobPlugin::createWidget(QWidget* parent)
	{
      return new Awl::PanKnob(parent);
      }
QWidget* MidiPanKnobPlugin::createWidget(QWidget* parent)
	{
      return new Awl::MidiPanKnob(parent);
      }
QWidget* DrawbarPlugin::createWidget(QWidget* parent)
	{
      return new Awl::Drawbar(parent);
      }

//---------------------------------------------------------
//   customWidgets
//---------------------------------------------------------

QList<QDesignerCustomWidgetInterface*> AwlPlugins::customWidgets() const
	{
	QList<QDesignerCustomWidgetInterface*> plugins;
            plugins
               << new VolKnobPlugin
               << new PanKnobPlugin
               << new MidiPanKnobPlugin
               << new KnobPlugin
      	   << new SliderPlugin
      	   << new VolSliderPlugin
      	   << new MeterSliderPlugin
      	   << new PosEditPlugin
      	   << new PosLabelPlugin
      	   << new TempoEditPlugin
      	   << new TempoLabelPlugin
      	   << new CheckBoxPlugin
      	   << new ComboBoxPlugin
      	   << new FloatEntryPlugin
      	   << new DrawbarPlugin;
      return plugins;
	}

Q_EXPORT_PLUGIN(AwlPlugins)

