//
// C++ Implementation: ssplugingui
//
// Description:
//
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
//  Contributer: (C) Copyright 2011 Tim E. Real (terminator356 at users.sourceforge.net)
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
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "ssplugingui.h"
#include "ssplugin.h"
#include "simpledrumsgui.h"

#define SS_PLUGINGUI_XOFF       300
#define SS_PLUGINGUI_YOFF       300
#define SS_PLUGINGUI_WIDTH      450
#define SS_PLUGINGUI_MAX_WIDTH  700

#define SS_PLUGINFRONT_MINWIDTH SS_PLUGINGUI_WIDTH
#define SS_PLUGINFRONT_MINHEIGHT 70
#define SS_PLUGINFRONT_MARGIN    9
#define SS_PLUGINFRONT_INC_PARAM    30
#define SS_PLUGINFRONT_INC_PARAM_MIN 60
#define SS_PLUGINGUI_HEIGHT (SS_NR_OF_SENDEFFECTS * SS_PLUGINFRONT_MINHEIGHT)

#define SS_PLUGINCHOOSER_NAMECOL     0
#define SS_PLUGINCHOOSER_LABELCOL    1
#define SS_PLUGINCHOOSER_INPORTSCOL  2
#define SS_PLUGINCHOOSER_OUTPORTSCOL 3
#define SS_PLUGINCHOOSER_CREATORCOL  4


/*!
    \fn SS_PluginChooser::SS_PluginChooser(QWidget* parent, const char* name = 0)
 */
SS_PluginChooser::SS_PluginChooser(QWidget* parent)
      :QDialog(parent)
      {
      SS_TRACE_IN
      setupUi(this);
      selectedPlugin = 0;

      for (iPlugin i=plugins.begin(); i !=plugins.end(); i++) {
            //Support for only 2 or 1 inport/outports
            if ( ((*i)->outports() == 2 || (*i)->outports() == 1) && ((*i)->inports() == 2 || (*i)->inports() == 1) ) {
                  QTreeWidgetItem* tmpItem = new QTreeWidgetItem(effectsListView);
                  tmpItem->setText(SS_PLUGINCHOOSER_NAMECOL, (*i)->name());
                  tmpItem->setText(SS_PLUGINCHOOSER_LABELCOL, (*i)->label());
                  tmpItem->setText(SS_PLUGINCHOOSER_INPORTSCOL, QString::number((*i)->inports()));
                  tmpItem->setText(SS_PLUGINCHOOSER_OUTPORTSCOL, QString::number((*i)->outports()));
                  tmpItem->setText(SS_PLUGINCHOOSER_CREATORCOL, (*i)->maker());
                  effectsListView->addTopLevelItem(tmpItem);
                  }
            }
      connect(okButton, SIGNAL(pressed()), SLOT(okPressed()));
      connect(cancelButton, SIGNAL(pressed()), SLOT(cancelPressed()));
      
      //connect(effectsListView, SIGNAL(selectionChanged(QTreeWidgetItem*)), SLOT(selectionChanged(QTreeWidgetItem*)));
      //connect(effectsListView, SIGNAL(doubleClicked(QTreeWidgetItem*)), SLOT(doubleClicked(QTreeWidgetItem*)));
      connect(effectsListView, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()));                            // p4.0.27
      connect(effectsListView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(doubleClicked(QTreeWidgetItem*)));  //
      
      SS_TRACE_OUT
      }

/*!
    \fn SS_PluginChooser::selectionChanged(QListViewItem* item)
 */
//void SS_PluginChooser::selectionChanged(QTreeWidgetItem* item)
void SS_PluginChooser::selectionChanged()
      {
      SS_TRACE_IN
      //selectedItem  = item;
      selectedItem = effectsListView->currentItem();  
      SS_TRACE_OUT
      }

/*!
    \fn SS_PluginChooser::okPressed()
 */
void SS_PluginChooser::okPressed()
      {
      SS_TRACE_IN
      selectedPlugin = findSelectedPlugin();
      done(QDialog::Accepted);
      SS_TRACE_OUT
      }

/*!
    \fn SS_PluginChooser::cancelPressed()
 */
void SS_PluginChooser::cancelPressed()
      {
      SS_TRACE_IN
      SS_TRACE_OUT
      done(QDialog::Rejected);
      }

/*!
    \fn SS_PluginChooser::doubleClicked(QListViewItem* item)
 */
void SS_PluginChooser::doubleClicked(QTreeWidgetItem* item)
      {
      SS_TRACE_IN
      selectedItem = item;    // p4.0.27 Tim
      selectedPlugin = findSelectedPlugin();
      SS_TRACE_OUT
      done(QDialog::Accepted);
      }

/*!
    \fn SS_PluginChooser::getSelectedPlugin()
 */
LadspaPlugin* SS_PluginChooser::findSelectedPlugin()
      {
      SS_TRACE_IN
      if(!selectedItem)        // p4.0.27 Tim
        return 0;
      LadspaPlugin* selected = 0;
      for (iPlugin i=plugins.begin(); i != plugins.end(); i++) {
            if ((*i)->name() == selectedItem->text(SS_PLUGINCHOOSER_NAMECOL))
                  selected = (LadspaPlugin*) (*i);
            }
      SS_TRACE_OUT
      return selected;
      }

/*!
    \fn SS_PluginFront::SS_PluginFront(QWidget* parent, const char* name = 0)
 */
SS_PluginFront::SS_PluginFront(QWidget* parent, int in_fxid)
      : QGroupBox(parent), fxid (in_fxid)
      {
      SS_TRACE_IN
      expanded = false;
      pluginChooser = 0;
      plugin = 0;
      expGroup = 0;

//TD      setLineWidth(3);
      setFlat(false);
//TD      setFrameStyle( Q3Frame::Box | Q3Frame::Raised );
//TD      setFrameShape(QFrame::StyledPanel);
//      setFrameShadow(Qt::Sunken);
      setFocusPolicy(Qt::NoFocus);
      setMinimumSize(SS_PLUGINFRONT_MINWIDTH, SS_PLUGINFRONT_MINHEIGHT);
      setMaximumSize(SS_PLUGINGUI_MAX_WIDTH, SS_PLUGINFRONT_MINHEIGHT);

      QVBoxLayout* bigLayout = new QVBoxLayout(this);
      bigLayout->setContentsMargins(SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN);
      bigLayout->setAlignment(Qt::AlignTop);
//TODO      bigLayout->setResizeMode(QLayout::SetNoConstraint);

      layout = new QHBoxLayout;
      bigLayout->addLayout(layout);
      layout->setAlignment(Qt::AlignVCenter);
//TODO      layout->setResizeMode(QLayout::SetNoConstraint);


      QVBoxLayout* onOffLayout = new QVBoxLayout;
      layout->addLayout(onOffLayout);
      onOffLayout->setContentsMargins(SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN);
      onOff = new QCheckBox(this);
      onOffLayout->addWidget(new QLabel("On/Off", this));
      onOffLayout->addWidget(onOff);
      connect(onOff, SIGNAL(toggled(bool)), SLOT(onOffToggled(bool)));

      pluginName = new QLineEdit(this);
      pluginName->setReadOnly(true);
      layout->addWidget(pluginName);

      loadFxButton = new QPushButton("L", this);
      QRect r = loadFxButton->geometry();
      loadFxButton->setGeometry(r.x(), r.y(), 20, pluginName->geometry().height());
      loadFxButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      loadFxButton->setMinimumSize(20,pluginName->geometry().height());
      loadFxButton->setMaximumSize(30,pluginName->geometry().height());
      connect(loadFxButton, SIGNAL(clicked()), SLOT(loadButton()));
      layout->addWidget(loadFxButton);

      clearFxButton = new QPushButton("C", this);
      r = clearFxButton->geometry();
      clearFxButton->setGeometry(r.x(), r.y(), 20, pluginName->geometry().height());
      clearFxButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      clearFxButton->setMinimumSize(20,pluginName->geometry().height());
      clearFxButton->setMaximumSize(30,pluginName->geometry().height());
      connect(clearFxButton, SIGNAL(clicked()), SLOT(clearButtonPressed()));
      layout->addWidget(clearFxButton);

      layout->addSpacing(5);

      expandButton = new QPushButton("->", this);
      r = loadFxButton->geometry();
      expandButton->setGeometry(r.x(), r.y(), 20, pluginName->geometry().height());
      expandButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      expandButton->setMinimumSize(20,pluginName->geometry().height());
      expandButton->setMaximumSize(30,pluginName->geometry().height());
      connect(expandButton, SIGNAL(clicked()), SLOT(expandButtonPressed()));
      layout->addWidget(expandButton);

      layout->addSpacing(5);

      QVBoxLayout* gainSliderLayout = new QVBoxLayout;
      layout->addLayout(gainSliderLayout);
      gainSliderLayout->addWidget(new QLabel("Return level", this));
      gainSliderLayout->setContentsMargins(SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN);
      outGainSlider = new QSlider(Qt::Horizontal, this);
      outGainSlider->setMinimumSize(100, pluginName->geometry().height());
      outGainSlider->setMaximumSize(500, pluginName->geometry().height());
      loadFxButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      outGainSlider->setRange(0, 127);
      outGainSlider->setValue(75);
      connect(outGainSlider, SIGNAL(valueChanged(int)), SLOT(returnSliderMoved(int)));
      gainSliderLayout->addWidget(outGainSlider);
      clearPluginDisplay();

      expLayout = new QVBoxLayout; // (bigLayout, 2);
      bigLayout->addLayout(expLayout);

      clearFxButton->setToolTip(tr("Clear and unload effect"));
      loadFxButton->setToolTip(tr("Load effect"));
      expandButton->setToolTip(tr("Toggle display of effect parameters"));
      onOff->setToolTip(tr("Turn effect on/off"));
      SS_TRACE_OUT
      }

SS_PluginFront::~SS_PluginFront()
      {
      if (pluginChooser)
            delete pluginChooser;
      }

/*!
    \fn SS_PluginFront::clearPluginDisplay()
 */
void SS_PluginFront::clearPluginDisplay()
      {
      SS_TRACE_IN
      if (expanded)
            expandButtonPressed();

      pluginName->setText("No plugin loaded");
      pluginName->setEnabled(false);
      onOff->setEnabled(false);
      onOff->blockSignals(true);
      onOff->setChecked(false);
      onOff->blockSignals(false);

      clearFxButton->setEnabled(false);
      expandButton->setEnabled(false);
      outGainSlider->setEnabled(false);
      SS_TRACE_OUT
      }

/*!
    \fn SS_PluginFront::setPluginName(QString name)
 */
void SS_PluginFront::setPluginName(QString name)
      {
      pluginName->setText(name);
      }


/*!
    \fn SS_PluginFront::loadButton()
 */
void SS_PluginFront::loadButton()
      {
      SS_TRACE_IN
      if (!pluginChooser)
            pluginChooser = new SS_PluginChooser(this);

      pluginChooser->exec();
      if ((pluginChooser->result() == QDialog::Accepted) && pluginChooser->getSelectedPlugin()) {
            Plugin* p = pluginChooser->getSelectedPlugin();
            //printf("Selected plugin: %s\n", pluginChooser->getSelectedPlugin()->name().toLatin1().constData());
            emit loadPlugin(fxid, p->lib(), p->label());
            }
      SS_TRACE_OUT
      }

/*!
    \fn SS_PluginFront::returnSliderMoved(int val)
 */
void SS_PluginFront::returnSliderMoved(int val)
      {
      emit returnLevelChanged(fxid, val);
      }


/*!
    \fn SS_PluginFront::updatePluginValue(unsigned i)
 */
void SS_PluginFront::updatePluginValue(unsigned k)
      {
      SS_TRACE_IN
      // If parameters are shown - close them
      if (expanded) {
            expandButtonPressed();
            }

      unsigned j=0;
      if (k > plugins.size()) {
            fprintf(stderr, "Internal error, tried to update plugin w range outside of list\n");
            return;
            }

      iPlugin i;
      for (i = plugins.begin(); j != k; i++, j++) ;
      plugin = (LadspaPlugin*) *(i);
      setPluginName(plugin->label());
      outGainSlider->setEnabled(true);
      clearFxButton->setEnabled(true);
      expandButton->setEnabled(true);
      pluginName->setEnabled(true);
      onOff->setEnabled(true);
      ///onOff->setChecked(true);
      SS_TRACE_OUT
      }

/*!
    \fn SS_PluginFront::onOffToggled(bool state)
 */
void SS_PluginFront::onOffToggled(bool state)
      {
      emit fxToggled(fxid, state);
      }

/*!
    \fn SS_PluginFront::sizeHint() const
 */
QSize SS_PluginFront::sizeHint() const
      {
      return QSize(SS_PLUGINFRONT_MINWIDTH, 50);
      }

/*!
    \fn SS_PluginFront::sizePolicy() const
 */
QSizePolicy SS_PluginFront::sizePolicy() const
      {
      return QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
      }


/*!
    \fn SS_PluginFront::clearButtonPressed()
 */
void SS_PluginFront::clearButtonPressed()
      {
      // If parameters are shown - close them
      if (expanded) {
            expandButtonPressed();
            }
      emit clearPlugin(fxid);
      }

/*!
    \fn SS_PluginFront::setRetGain(int val)
 */
void SS_PluginFront::setRetGain(int val)
      {
      outGainSlider->blockSignals(true);
      outGainSlider->setValue(val);
      outGainSlider->blockSignals(false);
      }

/*!
    \fn SS_PluginFront::setOnOff(bool val)
 */
void SS_PluginFront::setOnOff(bool val)
      {
      onOff->blockSignals(true);
      onOff->setChecked(val);
      onOff->blockSignals(false);
      }
/*!
    \fn SS_PluginFront::expandButtonPressed()
 */
void SS_PluginFront::expandButtonPressed()
      {
      SS_TRACE_IN
      int sizeIncrease = 0;
      QRect pf = geometry();

      if (!expanded) {
            plugin->parameter() == 1 ? sizeIncrease = SS_PLUGINFRONT_INC_PARAM_MIN : sizeIncrease = plugin->parameter() * SS_PLUGINFRONT_INC_PARAM;
            pf.setHeight(pf.height() + sizeIncrease);
            setMinimumSize(QSize(pf.width(), pf.height()));
            setMaximumSize(QSize(SS_PLUGINGUI_MAX_WIDTH, pf.height()));
            setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
            setGeometry(pf);
            emit sizeChanged(fxid, sizeIncrease);

            expanded = true;
            expandButton->setText("<-");
            createPluginParameters();
            }
      else {
//TODO            expLayout->remove(expGroup);
            expGroup->hide();
            expGroup->deleteLater();
            paramWidgets.clear();
            expGroup = 0;
            plugin->parameter() == 1 ? sizeIncrease = (0-SS_PLUGINFRONT_INC_PARAM_MIN) : sizeIncrease = 0 - (plugin->parameter() * SS_PLUGINFRONT_INC_PARAM);
            expandButton->setText("->");
            expanded = false;
            pf.setHeight(pf.height() + sizeIncrease);
            pf.setTop(pf.top() + sizeIncrease);
            pf.setBottom(pf.bottom() + sizeIncrease);
            setGeometry(pf);
            adjustSize();
            layout->activate();
            setMinimumSize(QSize(pf.width(), pf.height()));
            setMaximumSize(QSize(SS_PLUGINGUI_MAX_WIDTH, pf.height()));
            setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
            emit sizeChanged(fxid, sizeIncrease);
            }
      SS_TRACE_OUT
      }

/*!
    \fn SS_PluginFront::createPluginParameters()
 */
void SS_PluginFront::createPluginParameters()
      {
      SS_TRACE_IN
      expGroup = new QGroupBox(this);

      expGroup->setMinimumSize(QSize(50, 50));
      expGroup->setMaximumSize(QSize(SS_PLUGINGUI_MAX_WIDTH, (plugin->parameter() * SS_PLUGINFRONT_INC_PARAM  - SS_PLUGINFRONT_MARGIN)));
      expGroup->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
      expLayout->addWidget(expGroup);
      expGroup->show();
      QVBoxLayout* expGroupLayout = new QVBoxLayout(expGroup); // , 1);
      expGroupLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//TD      expGroupLayout->setResizeMode(QLayout::FreeResize);
      expGroupLayout->setContentsMargins(SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN, SS_PLUGINFRONT_MARGIN);

      for (int i=0; i < plugin->parameter(); i++) {
            QHBoxLayout* paramStrip = new QHBoxLayout; // (expGroupLayout, 3);
            expGroupLayout->addLayout(paramStrip);
            paramStrip->setAlignment(Qt::AlignLeft);
            QLabel* paramName = new QLabel(plugin->getParameterName(i), expGroup);
            paramName->show();
            paramName->setMinimumSize(QSize(150, 10));
            paramName->setMaximumSize(QSize(300, SS_PLUGINFRONT_INC_PARAM));
            paramName->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));

            paramStrip->addWidget(paramName);

            if (plugin->isBool(i)) {
                  SS_ParameterCheckBox* paramCheckBox = new SS_ParameterCheckBox(expGroup, plugin, fxid, i);
                  paramCheckBox->setEnabled(true);
                  paramCheckBox->setParamValue((int) plugin->getControlValue(i));
                  paramCheckBox->show();
                  paramStrip->addWidget(paramCheckBox);
                  connect(paramCheckBox, SIGNAL(valueChanged(int, int, int)), SLOT(parameterValueChanged(int, int, int)));
                  }
            else  {
                  SS_ParameterSlider* paramSlider = new SS_ParameterSlider(expGroup, plugin, fxid, i);
                  paramSlider->setEnabled(true);
                  paramSlider->show();
                  paramSlider->setRange(SS_PLUGIN_PARAM_MIN, SS_PLUGIN_PARAM_MAX);

                  float max, min;
                  plugin->range(i, &min, &max);
                  //int intval = 0;
                  paramSlider->setParamValue(plugin->getGuiControlValue(i));
                  connect(paramSlider, SIGNAL(valueChanged(int, int, int)), SLOT(parameterValueChanged(int, int, int)));
                  paramStrip->addWidget(paramSlider);
                  }
            }
      expLayout->activate();
      SS_TRACE_OUT
      }

/*!
    \fn SS_PluginFront::parameterValueChanged(int fxid, int parameter, int val)
 */
void SS_PluginFront::parameterValueChanged(int fxid, int parameter, int val)
      {
      emit effectParameterChanged(fxid, parameter, val);
      }

/*!
    \fn SS_PluginFront::setParameterValue(int param, float val)
 */
void SS_PluginFront::setParameterValue(int param, int val)
      {
      SS_TRACE_IN
      int j=0;
      for (SS_iParameterWidgetList i=paramWidgets.begin(); i != paramWidgets.end(); i++, j++) {
            if (j == param) {
                  (*i)->setParamValue(val);
                  }
            }
      SS_TRACE_OUT
      }

SS_PluginGui::SS_PluginGui(QWidget* parent)
      : QDialog(parent)
      {
      setWindowTitle("SimpleDrums LADSPA sendeffects");
      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
            pluginFronts[i] = 0;
            }
      layout = new QVBoxLayout(this);

      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
            pluginFronts[i] = new SS_PluginFront(this, i);
            pluginFronts[i]->update();
            layout->addWidget(pluginFronts[i]);
            connect(pluginFronts[i], SIGNAL(loadPlugin(int, QString, QString)), simplesynthgui_ptr, SLOT(loadEffectInvoked(int, QString, QString)));
            connect(pluginFronts[i], SIGNAL(returnLevelChanged(int, int)), simplesynthgui_ptr, SLOT(returnLevelChanged(int, int)));
            connect(pluginFronts[i], SIGNAL(fxToggled(int, int)), simplesynthgui_ptr, SLOT(toggleEffectOnOff(int, int)));
            connect(pluginFronts[i], SIGNAL(clearPlugin(int)), simplesynthgui_ptr, SLOT(clearPlugin(int)));
            connect(pluginFronts[i], SIGNAL(sizeChanged(int, int)), SLOT(pluginFrontSizeChanged(int, int)));
            connect(pluginFronts[i], SIGNAL(effectParameterChanged(int, int, int)), simplesynthgui_ptr, SLOT(effectParameterChanged(int, int, int)));
            }
            
      // FIXME: These are causing window height to be fixed way too small - can't see anything. Why? It was working before. Tim p4.0.49
      //setMinimumSize(QSize(SS_PLUGINGUI_WIDTH, geometry().height()));
      //setMaximumSize(QSize(SS_PLUGINGUI_MAX_WIDTH, geometry().height()));
      }


/*!
    \fn SS_PluginGui::pluginFrontSizeChanged(int fxid, int val)
 */
void SS_PluginGui::pluginFrontSizeChanged(int /*fxid*/, int val)
      {
      QRect r = geometry();
      r.setHeight(r.height() + val);
      setMinimumSize(QSize(SS_PLUGINGUI_WIDTH, r.height()));
      setMaximumSize(QSize(SS_PLUGINGUI_MAX_WIDTH, r.height()));
      setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
      setGeometry(r);
      adjustSize();
      }

SS_PluginFront* SS_PluginGui::getPluginFront(unsigned i)
      {
      SS_TRACE_IN
      if (i<SS_NR_OF_SENDEFFECTS)
      SS_TRACE_OUT
      return pluginFronts[i];
      }
