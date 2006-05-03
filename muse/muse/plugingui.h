//=============================================================================
//  MusE
//  Linux Music Editor
//  $Id:$
//
//  Copyright (C) 2002-2006 by Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//=============================================================================

#ifndef __PLUGINGUI_H__
#define __PLUGINGUI_H__

namespace Awl {
      class FloatEntry;
      };
using Awl::FloatEntry;

class Plugin;
class PluginI;

//---------------------------------------------------------
//   GuiParam
//---------------------------------------------------------

struct GuiParam {
      enum {
            GUI_SLIDER, GUI_SWITCH
            };
      int type;
      FloatEntry* label;
      QWidget* actuator;  // Slider or Toggle Button (SWITCH)
      };

//---------------------------------------------------------
//   GuiWidget
//---------------------------------------------------------

struct GuiWidget {
      enum {
            SLIDER, FLOAT_ENTRY, CHECKBOX, COMBOBOX
            } type;
      QWidget* widget;
      int parameter;
      };

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

class PluginGui : public QMainWindow {
      Q_OBJECT

      PluginI* plugin;        // plugin instance
      std::vector<GuiWidget> gw;

      QToolButton* onOff;
      void connectPrebuiltGui(QWidget* wContainer);

   private slots:
      void load();
      void save();
      void bypassToggled(bool);

      void setController(float, int);
      void autoChanged();

   public:
      PluginGui(PluginI*);
      ~PluginGui();
      void setOn(bool);
      void updateValue(int, float);
      void updateValues();

   public slots:
      void controllerChanged(int id);
      };

//---------------------------------------------------------
//   PluginDialog
//---------------------------------------------------------

enum { SEL_SM, SEL_S, SEL_M, SEL_ALL };

class PluginDialog : public QDialog {
      QTreeWidget* pList;
      QRadioButton* allPlug;
      QRadioButton* onlyM;
      QRadioButton* onlyS;
      QRadioButton* onlySM;

      Q_OBJECT

   public:
      PluginDialog(QWidget* parent=0);
      static Plugin* getPlugin(QWidget* parent);
      Plugin* value();
      void accept();

  public slots:
      void fillPlugs(QAbstractButton*);
      void fillPlugs(int i);
      void fillPlugs(const QString& sortValue);

  private:
	QComboBox *sortBox;
	static int selectedPlugType;
	static QStringList sortItems;
      };

#endif

