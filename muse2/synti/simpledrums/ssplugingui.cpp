//
// C++ Implementation: ssplugingui
//
// Description:
//
//
// Author: Mathias Lundgren <lunar_shuttle@users.sf.net>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <stdlib.h>
#include <qlayout.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3Frame>
#include <QLabel>
#include <Q3VBoxLayout>
#include <QtGui>
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
SS_PluginChooser::SS_PluginChooser(QWidget* parent, const char* name)
      :SS_PluginChooserBase(parent, name)
      {
      SS_TRACE_IN
      selectedPlugin = 0;

      for (iPlugin i=plugins.begin(); i !=plugins.end(); i++) {
            //Support for only 2 or 1 inport/outports
            if ( ((*i)->outports() == 2 || (*i)->outports() == 1) && ((*i)->inports() == 2 || (*i)->inports() == 1) ) {
                  Q3ListViewItem* tmpItem = new Q3ListViewItem(effectsListView);
                  tmpItem->setText(SS_PLUGINCHOOSER_NAMECOL, (*i)->name());
                  tmpItem->setText(SS_PLUGINCHOOSER_LABELCOL, (*i)->label());
                  tmpItem->setText(SS_PLUGINCHOOSER_INPORTSCOL, QString::number((*i)->inports()));
                  tmpItem->setText(SS_PLUGINCHOOSER_OUTPORTSCOL, QString::number((*i)->outports()));
                  tmpItem->setText(SS_PLUGINCHOOSER_CREATORCOL, (*i)->maker());
                  effectsListView->insertItem(tmpItem);
                  }
            }
      connect(okButton, SIGNAL(pressed()), SLOT(okPressed()));
      connect(cancelButton, SIGNAL(pressed()), SLOT(cancelPressed()));
      connect(effectsListView, SIGNAL(selectionChanged(Q3ListViewItem*)), SLOT(selectionChanged(Q3ListViewItem*)));
      connect(effectsListView, SIGNAL(doubleClicked(Q3ListViewItem*)), SLOT(doubleClicked(Q3ListViewItem*)));
      SS_TRACE_OUT
      }

/*!
    \fn SS_PluginChooser::selectionChanged(QListViewItem* item)
 */
void SS_PluginChooser::selectionChanged(Q3ListViewItem* item)
      {
      SS_TRACE_IN
      selectedItem  = item;
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
void SS_PluginChooser::doubleClicked(Q3ListViewItem* /*item*/)
      {
      SS_TRACE_IN
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
SS_PluginFront::SS_PluginFront(QWidget* parent, int in_fxid, const char* name)
      : Q3GroupBox(parent, name), fxid (in_fxid)
      {
      SS_TRACE_IN
      expanded = false;
      pluginChooser = 0;
      plugin = 0;
      expGroup = 0;

      setLineWidth(3);
      setFlat(false);
      setFrameStyle( Q3Frame::Box | Q3Frame::Raised );
      setFrameShape(Q3GroupBox::Box);//  QFrame::Box);
      setFrameShadow(Sunken);
      setFocusPolicy(Qt::NoFocus);
      setMinimumSize(SS_PLUGINFRONT_MINWIDTH, SS_PLUGINFRONT_MINHEIGHT);
      setMaximumSize(SS_PLUGINGUI_MAX_WIDTH, SS_PLUGINFRONT_MINHEIGHT);

      Q3VBoxLayout* bigLayout = new Q3VBoxLayout(this);
      bigLayout->setMargin(SS_PLUGINFRONT_MARGIN);
      bigLayout->setAlignment(Qt::AlignTop);
      bigLayout->setResizeMode(QLayout::SetNoConstraint);

      layout = new Q3HBoxLayout(bigLayout);
      layout->setAlignment(Qt::AlignVCenter);
      layout->setResizeMode(QLayout::SetNoConstraint);


      Q3VBoxLayout* onOffLayout = new Q3VBoxLayout(layout);
      onOffLayout->setMargin(SS_PLUGINFRONT_MARGIN);
      onOff = new QCheckBox(this);
      onOffLayout->add(new QLabel("On/Off", this));
      onOffLayout->add(onOff);
      connect(onOff, SIGNAL(toggled(bool)), SLOT(onOffToggled(bool)));

      pluginName = new QLineEdit(this);
      pluginName->setReadOnly(true);
      layout->add(pluginName);

      loadFxButton = new QPushButton("L", this);
      QRect r = loadFxButton->geometry();
      loadFxButton->setGeometry(r.x(), r.y(), 20, pluginName->geometry().height());
      loadFxButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      loadFxButton->setMinimumSize(20,pluginName->geometry().height());
      loadFxButton->setMaximumSize(30,pluginName->geometry().height());
      connect(loadFxButton, SIGNAL(clicked()), SLOT(loadButton()));
      layout->add(loadFxButton);

      clearFxButton = new QPushButton("C", this);
      r = clearFxButton->geometry();
      clearFxButton->setGeometry(r.x(), r.y(), 20, pluginName->geometry().height());
      clearFxButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      clearFxButton->setMinimumSize(20,pluginName->geometry().height());
      clearFxButton->setMaximumSize(30,pluginName->geometry().height());
      connect(clearFxButton, SIGNAL(clicked()), SLOT(clearButtonPressed()));
      layout->add(clearFxButton);

      layout->addSpacing(5);

      expandButton = new QPushButton("->", this);
      r = loadFxButton->geometry();
      expandButton->setGeometry(r.x(), r.y(), 20, pluginName->geometry().height());
      expandButton->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      expandButton->setMinimumSize(20,pluginName->geometry().height());
      expandButton->setMaximumSize(30,pluginName->geometry().height());
      connect(expandButton, SIGNAL(clicked()), SLOT(expandButtonPressed()));
      layout->add(expandButton);

      layout->addSpacing(5);

      Q3VBoxLayout* gainSliderLayout = new Q3VBoxLayout(layout);
      gainSliderLayout->add(new QLabel("Return level", this));
      gainSliderLayout->setMargin(SS_PLUGINFRONT_MARGIN);
      outGainSlider = new QSlider(Qt::Horizontal, this);
      outGainSlider->setMinimumSize(100, pluginName->geometry().height());
      outGainSlider->setMaximumSize(500, pluginName->geometry().height());
      loadFxButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
      outGainSlider->setRange(0, 127);
      outGainSlider->setValue(75);
      connect(outGainSlider, SIGNAL(valueChanged(int)), SLOT(returnSliderMoved(int)));
      gainSliderLayout->add(outGainSlider);
      clearPluginDisplay();

      expLayout = new Q3VBoxLayout(bigLayout, 2);

      QToolTip::add(clearFxButton, "Clear and unload effect");
      QToolTip::add(loadFxButton,  "Load effect");
      QToolTip::add(expandButton,  "Toggle display of effect parameters");
      QToolTip::add(onOff,         "Turn effect on/off");
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
            pluginChooser = new SS_PluginChooser(this, "temppluginchooser");

      pluginChooser->exec();
      if ((pluginChooser->result() == QDialog::Accepted) && pluginChooser->getSelectedPlugin()) {
            Plugin* p = pluginChooser->getSelectedPlugin();
            //printf("Selected plugin: %s\n", pluginChooser->getSelectedPlugin()->name().latin1());
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
      onOff->setChecked(true);
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
            expLayout->remove(expGroup);
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
      expGroup = new Q3ButtonGroup(this);

      expGroup->setMinimumSize(QSize(50, 50));
      expGroup->setMaximumSize(QSize(SS_PLUGINGUI_MAX_WIDTH, (plugin->parameter() * SS_PLUGINFRONT_INC_PARAM  - SS_PLUGINFRONT_MARGIN)));
      expGroup->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
      expLayout->add(expGroup);
      expGroup->show();
      Q3VBoxLayout* expGroupLayout = new Q3VBoxLayout(expGroup, 1);
      expGroupLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      expGroupLayout->setResizeMode(QLayout::SetNoConstraint);
      expGroupLayout->setMargin(SS_PLUGINFRONT_MARGIN);

      for (int i=0; i < plugin->parameter(); i++) {
            Q3HBoxLayout* paramStrip = new Q3HBoxLayout(expGroupLayout, 3);
            paramStrip->setAlignment(Qt::AlignLeft);
            QLabel* paramName = new QLabel(plugin->getParameterName(i), expGroup);
            paramName->show();
            paramName->setMinimumSize(QSize(150, 10));
            paramName->setMaximumSize(QSize(300, SS_PLUGINFRONT_INC_PARAM));
            paramName->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));

            paramStrip->add(paramName);

            if (plugin->isBool(i)) {
                  SS_ParameterCheckBox* paramCheckBox = new SS_ParameterCheckBox(expGroup, plugin, fxid, i);
                  paramCheckBox->setEnabled(true);
                  paramCheckBox->setParamValue((int) plugin->getControlValue(i));
                  paramCheckBox->show();
                  paramStrip->add(paramCheckBox);
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
                  paramStrip->add(paramSlider);
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

SS_PluginGui::SS_PluginGui(QWidget* parent, const char* name)
      : QDialog(parent, name, false)
      {
      this->setCaption("SimpleDrums LADSPA sendeffects");
      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
            pluginFronts[i] = 0;
            }
      layout = new Q3VBoxLayout(this);

      for (int i=0; i<SS_NR_OF_SENDEFFECTS; i++) {
            pluginFronts[i] = new SS_PluginFront(this, i);
            pluginFronts[i]->update();
            layout->add(pluginFronts[i]);
            connect(pluginFronts[i], SIGNAL(loadPlugin(int, QString, QString)), simplesynthgui_ptr, SLOT(loadEffectInvoked(int, QString, QString)));
            connect(pluginFronts[i], SIGNAL(returnLevelChanged(int, int)), simplesynthgui_ptr, SLOT(returnLevelChanged(int, int)));
            connect(pluginFronts[i], SIGNAL(fxToggled(int, int)), simplesynthgui_ptr, SLOT(toggleEffectOnOff(int, int)));
            connect(pluginFronts[i], SIGNAL(clearPlugin(int)), simplesynthgui_ptr, SLOT(clearPlugin(int)));
            connect(pluginFronts[i], SIGNAL(sizeChanged(int, int)), SLOT(pluginFrontSizeChanged(int, int)));
            connect(pluginFronts[i], SIGNAL(effectParameterChanged(int, int, int)), simplesynthgui_ptr, SLOT(effectParameterChanged(int, int, int)));
            }
      setMinimumSize(QSize(SS_PLUGINGUI_WIDTH, geometry().height()));
      setMaximumSize(QSize(SS_PLUGINGUI_MAX_WIDTH, geometry().height()));
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
