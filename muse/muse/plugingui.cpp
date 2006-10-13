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

#include "song.h"
#include "fastlog.h"
#include "widgets/filedialog.h"
#include "plugin.h"
#include "plugingui.h"
#include "icons.h"
#include "al/xml.h"
#include "gui.h"

#include "awl/floatentry.h"
#include "awl/slider.h"
#include "awl/checkbox.h"
#include "awl/combobox.h"

using Awl::FloatEntry;
using Awl::Slider;
using Awl::CheckBox;
using Awl::ComboBox;

static const char* preset_file_pattern[] = {
      QT_TR_NOOP("presets (*.pre *.pre.gz *.pre.bz2)"),
      QT_TR_NOOP("All Files (*)"),
      0
      };

int PluginDialog::selectedPlugType = SEL_SM;
QStringList PluginDialog::sortItems = QStringList();

//---------------------------------------------------------
//   PluginDialog
//    select Plugin dialog
//---------------------------------------------------------

PluginDialog::PluginDialog(QWidget* parent)
  : QDialog(parent)
      {
      setWindowTitle(tr("MusE: select plugin"));
      QVBoxLayout* layout = new QVBoxLayout(this);

      pList  = new QTreeWidget(this);
      pList->setColumnCount(11);
      pList->setSortingEnabled(true);
      QStringList headerLabels;
      headerLabels << tr("Lib");
      headerLabels << tr("Label");
      headerLabels << tr("Name");
      headerLabels << tr("AI");
      headerLabels << tr("AO");
      headerLabels << tr("CI");
      headerLabels << tr("CO");
      headerLabels << tr("IP");
      headerLabels << tr("id");
      headerLabels << tr("Maker");
      headerLabels << tr("Copyright");

      int sizes[] = { 110, 110, 0, 30, 30, 30, 30, 30, 40, 110, 110 };
      for (int i = 0; i < 11; ++i) {
            if (sizes[i] == 0) {
                  pList->header()->setResizeMode(i, QHeaderView::Stretch);
                  }
            else {
                  if (sizes[i] <= 40)     // hack alert!
                        pList->header()->setResizeMode(i, QHeaderView::Custom);
                  pList->header()->resizeSection(i, sizes[i]);
                  }
            }

      pList->setHeaderLabels(headerLabels);

      pList->setSelectionBehavior(QAbstractItemView::SelectRows);
      pList->setSelectionMode(QAbstractItemView::SingleSelection);
      pList->setAlternatingRowColors(true);

      fillPlugs(selectedPlugType);
      layout->addWidget(pList);

      //---------------------------------------------------
      //  Ok/Cancel Buttons
      //---------------------------------------------------

      QBoxLayout* w5 = new QHBoxLayout;
      layout->addLayout(w5);

      QPushButton* okB     = new QPushButton(tr("Ok"), this);
      okB->setDefault(true);
      QPushButton* cancelB = new QPushButton(tr("Cancel"), this);
      okB->setFixedWidth(80);
      cancelB->setFixedWidth(80);
      w5->addWidget(okB);
      w5->addSpacing(12);
      w5->addWidget(cancelB);

      QGroupBox* plugSelGroup = new QGroupBox;
      plugSelGroup->setTitle("Show plugs:");
      QHBoxLayout* psl = new QHBoxLayout;
      plugSelGroup->setLayout(psl);

      QButtonGroup* plugSel = new QButtonGroup(plugSelGroup);
      onlySM  = new QRadioButton;
      onlySM->setText(tr("Mono and Stereo"));
      onlySM->setCheckable(true);
      plugSel->addButton(onlySM);
      psl->addWidget(onlySM);
      onlyS = new QRadioButton;
      onlyS->setText(tr("Stereo"));
      onlyS->setCheckable(true);
      plugSel->addButton(onlyS);
      psl->addWidget(onlyS);
      onlyM = new QRadioButton;
      onlyM->setText(tr("Mono"));
      onlyM->setCheckable(true);
      plugSel->addButton(onlyM);
      psl->addWidget(onlyM);
      allPlug = new QRadioButton;
      allPlug->setText(tr("Show All"));
      allPlug->setCheckable(true);
      plugSel->addButton(allPlug);
      psl->addWidget(allPlug);
      plugSel->setExclusive(true);

      switch(selectedPlugType) {
            case SEL_SM:  onlySM->setChecked(true);  break;
            case SEL_S:   onlyS->setChecked(true);   break;
            case SEL_M:   onlyM->setChecked(true);   break;
            case SEL_ALL: allPlug->setChecked(true); break;
            }

      plugSelGroup->setToolTip(tr("Select which types of plugins should be visible in the list.<br>"
                             "Note that using mono plugins on stereo tracks is not a problem, two will be used in parallell.<br>"
                             "Also beware that the 'all' alternative includes plugins that probably not are usable by MusE."));

      w5->addSpacing(12);
      w5->addWidget(plugSelGroup);
      w5->addSpacing(12);

      QLabel *sortLabel = new QLabel;
      sortLabel->setText(tr("Search in 'Label' and 'Name':"));
      w5->addWidget(sortLabel);
      w5->addSpacing(2);

      sortBox = new QComboBox(this);
      sortBox->setEditable(true);
      if (!sortItems.empty())
            sortBox->addItems(sortItems);

      sortBox->setMinimumSize(100, 10);
      w5->addWidget(sortBox);
      w5->addStretch(-1);

      if (!sortBox->currentText().isEmpty())
            fillPlugs(sortBox->currentText());
      else
            fillPlugs(selectedPlugType);


      connect(pList,   SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), SLOT(accept()));
      connect(cancelB, SIGNAL(clicked()), SLOT(reject()));
      connect(okB,     SIGNAL(clicked()), SLOT(accept()));
      connect(plugSel, SIGNAL(buttonClicked(QAbstractButton*)), SLOT(fillPlugs(QAbstractButton*)));
      connect(sortBox, SIGNAL(editTextChanged(const QString&)),SLOT(fillPlugs(const QString&)));
      }

//---------------------------------------------------------
//   accept
//---------------------------------------------------------

void PluginDialog::accept()
      {
      if (!sortBox->currentText().isEmpty()) {
            foreach (QString item, sortItems)
                if(item == sortBox->currentText()) {
                    QDialog::accept();
                    return;
                    }
            sortItems.push_front(sortBox->currentText());
            }
      QDialog::accept();
      }

//---------------------------------------------------------
//   value
//---------------------------------------------------------

Plugin* PluginDialog::value()
      {
      QTreeWidgetItem* item = pList->selectedItems().at(0);
      if (item)
            return plugins.find(item->text(0), item->text(1));
printf("plugin not found\n");
      return 0;
      }

//---------------------------------------------------------
//   fillPlugs
//---------------------------------------------------------

void PluginDialog::fillPlugs(QAbstractButton* ab)
      {
      if (ab == allPlug)
            fillPlugs(SEL_ALL);
      else if (ab == onlyM)
            fillPlugs(SEL_M);
      else if (ab == onlyS)
            fillPlugs(SEL_S);
      else if (ab == onlySM)
            fillPlugs(SEL_SM);
      }

//---------------------------------------------------------
//    fillPlugs int
//---------------------------------------------------------

void PluginDialog::fillPlugs(int nbr)
      {
      pList->clear();
      for (iPlugin i = plugins.begin(); i != plugins.end(); ++i) {
            int ai = (*i)->inports();
            int ao = (*i)->outports();
            int ci = (*i)->parameter();
            int co = 0;
            bool addFlag = false;
            switch (nbr) {
                  case SEL_SM: // stereo & mono
                        if ((ai == 1 || ai == 2) && (ao == 1 || ao ==2)) {
                              addFlag = true;
                              }
                        break;
                  case SEL_S: // stereo
                        if ((ai == 1 || ai == 2) &&  ao ==2) {
                              addFlag = true;
                              }
                        break;
                  case SEL_M: // mono
                        if (ai == 1  && ao == 1) {
                              addFlag = true;
                              }
                        break;
                  case SEL_ALL: // all
                        addFlag = true;
                        break;
                  }
            if (addFlag) {
                  QTreeWidgetItem* item = new QTreeWidgetItem;
                  item->setText(0,  (*i)->lib());
                  item->setText(1,  (*i)->label());
                  item->setText(2,  (*i)->name());
                  item->setText(3,  QString().setNum(ai));
                  item->setText(4,  QString().setNum(ao));
                  item->setText(5,  QString().setNum(ci));
                  item->setText(6,  QString().setNum(co));
                  item->setText(7,  QString().setNum((*i)->inPlaceCapable()));
                  item->setText(8,  QString().setNum((*i)->id()));
                  item->setText(9,  (*i)->maker());
                  item->setText(10, (*i)->copyright());
                  pList->addTopLevelItem(item);
                  }
            }
      selectedPlugType = nbr;
      }

//---------------------------------------------------------
//    fillPlugs QString
//---------------------------------------------------------

void PluginDialog::fillPlugs(const QString &sortValue)
      {
      pList->clear();
      for (iPlugin i = plugins.begin(); i != plugins.end(); ++i) {
            int ai = (*i)->inports();
            int ao = (*i)->outports();
            int ci = (*i)->parameter();
            int co = 0;

            bool addFlag = false;

            if ((*i)->label().toLower().contains(sortValue.toLower()))
                  addFlag = true;
            else if ((*i)->name().toLower().contains(sortValue.toLower()))
                  addFlag = true;
            if (addFlag) {
                  QTreeWidgetItem* item = new QTreeWidgetItem;
                  item->setText(0,  (*i)->lib());
                  item->setText(1,  (*i)->label());
                  item->setText(2,  (*i)->name());
                  item->setText(3,  QString().setNum(ai));
                  item->setText(4,  QString().setNum(ao));
                  item->setText(5,  QString().setNum(ci));
                  item->setText(6,  QString().setNum(co));
                  item->setText(7,  QString().setNum((*i)->inPlaceCapable()));
                  item->setText(8,  QString().setNum((*i)->id()));
                  item->setText(9,  (*i)->maker());
                  item->setText(10, (*i)->copyright());
                  pList->addTopLevelItem(item);
                  }
            }
      }

//---------------------------------------------------------
//   getPlugin
//---------------------------------------------------------

Plugin* PluginDialog::getPlugin(QWidget* parent)
      {
      PluginDialog* dialog = new PluginDialog(parent);
      if (dialog->exec())
            return dialog->value();
      return 0;
      }

static const char* presetOpenText = "<img source=\"fileopen\"> "
      "Click this button to load a saved <em>preset</em>.";
static const char* presetSaveText = "Click this button to save curent parameter "
      "settings as a <em>preset</em>.  You will be prompted for a file name.";
static const char* presetBypassText = "Click this button to bypass effect unit";

//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::PluginGui(PluginI* p)
   : QMainWindow(0)
      {
      setIconSize(ICON_SIZE);
      plugin = p;
      setWindowTitle(plugin->name());

      QToolBar* tools = addToolBar(tr("File Buttons"));

      QAction* fileOpen = tools->addAction(QIcon(*openIcon), tr("Load Preset"),
         this, SLOT(load()));
      fileOpen->setWhatsThis(tr(presetOpenText));
      QAction* fileSave = tools->addAction(QIcon(*saveIcon), tr("Save Preset"),
         this, SLOT(save()));
      fileSave->setWhatsThis(tr(presetSaveText));

      tools->addAction(QWhatsThis::createAction(this));

      onOff = tools->addAction(*onOffIcon, tr("bypass plugin"));
      onOff->setCheckable(true);
      onOff->setChecked(plugin->on());
      onOff->setWhatsThis(tr(presetBypassText));
      connect(onOff, SIGNAL(triggered(bool)), SLOT(bypassToggled(bool)));

      QString id;
      id.setNum(plugin->plugin()->id());
      QString name(museGlobalShare + QString("/plugins/") + id + QString(".ui"));

      QWidget* mw;            // main widget
      QFile uifile(name);
      if (uifile.exists()) {
            //
            // construct GUI from *.ui file
            //
            QFormBuilder builder;
            //
            // HACK:
            //
            QString path(museGlobalLib + "/designer");
printf("build gui from ui <path><%s>\n", path.toLatin1().data());
            builder.addPluginPath(path);

            uifile.open(QFile::ReadOnly);
            mw = builder.load(&uifile, this);
            uifile.close();

            setCentralWidget(mw);
            connectPrebuiltGui(mw);
            }
      else {
            mw = new QWidget(this);
            setCentralWidget(mw);
            QGridLayout* grid = new QGridLayout;
            mw->setLayout(grid);
            grid->setSpacing(2);

            int n  = plugin->plugin()->parameter();
            resize(280, n*20+30);

            QFontMetrics fm = fontMetrics();
            int h           = fm.height() + 4;

            for (int i = 0; i < n; ++i) {
                  double lower;
                  double upper;
                  double dlower;
                  double dupper;
                  double val   = plugin->param(i);
                  double dval  = val;

                  plugin->range(i, &lower, &upper);
                  dlower = lower;
                  dupper = upper;

                  if (plugin->isLog(i)) {
                        if (lower == 0.0)
                              lower = 0.001;
                        dlower = fast_log10(lower)*20.0;
                        dupper = fast_log10(upper)*20.0;
                        if (val == 0.0f)
                              dval = dlower;
                        else
                              dval = fast_log10(val) * 20.0;
                        }
                  if (plugin->isBool(i)) {
                        CheckBox* cb = new CheckBox(mw);
                        cb->setId(i);
                        cb->setText(QString(plugin->getParameterName(i)));
                        cb->setChecked(plugin->param(i) > 0.5);
                        cb->setFixedHeight(h);
                        cb->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));

                        GuiWidget w;
                        w.widget    = cb;
                        w.parameter = i;
                        w.type      = GuiWidget::CHECKBOX;
                        gw.push_back(w);
                        grid->addWidget(cb, i, 0, 1, 3);
                        connect(cb, SIGNAL(valueChanged(double,int)), SLOT(setController(double, int)));
                        }
                  else {
                        QLabel* label    = new QLabel(QString(plugin->getParameterName(i)), mw);
                        label->setFixedHeight(20);
                        label->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
                        FloatEntry* e = new FloatEntry(mw);
                        e->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
                        e->setRange(lower, upper);
                        e->setId(i);
                        e->setFixedHeight(h);
                        e->setFrame(true);
                        GuiWidget w;
                        w.widget    = e;
                        w.parameter = i;
                        w.type      = GuiWidget::FLOAT_ENTRY;
                        gw.push_back(w);

                        Slider* s = new Slider(mw);
                        s->setId(i);
                        s->setLog(plugin->isLog(i));
                        s->setOrientation(Qt::Horizontal);
                        s->setFixedHeight(h);
                        s->setRange(dlower, dupper);
                        s->setLineStep((dupper-dlower)/100.0);
                        s->setPageStep((dupper-dlower)/10.0);
                        s->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
                        w.widget    = s;
                        w.parameter = i;
                        w.type      = GuiWidget::SLIDER;
                        gw.push_back(w);
                        grid->addWidget(label, i, 0);
                        grid->addWidget(e,     i, 1);
                        const char* p = plugin->getParameterLabel(i);
                        if (p) {
                              QLabel* l = new QLabel(mw);
                              l->setFixedHeight(h);
                              l->setText(p);
                              grid->addWidget(l, i, 2);
                              }
                        grid->addWidget(s,     i, 3);
                        connect(s, SIGNAL(valueChanged(double,int)), SLOT(setController(double,int)));
                        connect(e, SIGNAL(valueChanged(double,int)), SLOT(setController(double,int)));
                        }
                  updateValue(i, val);
                  }
            grid->setColumnStretch(3, 10);
            }
      connect(plugin->track(), SIGNAL(autoReadChanged(bool)), SLOT(autoChanged()));
      connect(plugin->track(), SIGNAL(autoWriteChanged(bool)), SLOT(autoChanged()));
      connect(plugin->track(), SIGNAL(controllerChanged(int)), SLOT(controllerChanged(int)));
      autoChanged();
      }



void PluginGui::connectPrebuiltGui(QWidget* wContainer)
      {
      QObjectList l = wContainer->children();
      for (int i = 0; i < l.size(); ++i) {

            QObject* obj = l.at(i);

            const char* name = obj->objectName().toLatin1().data();

            if (strcmp(obj->metaObject()->className(), "QFrame") == 0) {
                  connectPrebuiltGui((QWidget *)obj);
                  }
            if (*name !='P')
                  continue;
            GuiWidget w;
            w.widget = (QWidget*)obj;
            if (strcmp(obj->metaObject()->className(), "Awl::Slider") == 0) {
                  connect((Slider*)obj, SIGNAL(valueChanged(double,int)), SLOT(setController(double,int)));
                  w.type = GuiWidget::SLIDER;
                  w.parameter = ((Slider*)obj)->id();
                  }
            else if (strcmp(obj->metaObject()->className(), "Awl::FloatEntry") == 0) {
                  connect((FloatEntry*)obj, SIGNAL(valueChanged(double,int)), SLOT(setController(double,int)));
                  w.type = GuiWidget::FLOAT_ENTRY;
                  w.parameter = ((FloatEntry*)obj)->id();
                  }
            else if (strcmp(obj->metaObject()->className(), "Awl::CheckBox") == 0) {
                  w.type = GuiWidget::CHECKBOX;
                  w.parameter = ((CheckBox*)obj)->id();
                  connect(obj, SIGNAL(valueChanged(double, int)), SLOT(setController(double, int)));
                  }
            else if (strcmp(obj->metaObject()->className(), "Awl::ComboBox") == 0) {
                  w.type = GuiWidget::COMBOBOX;
                  w.parameter = ((ComboBox*)obj)->id();
                  connect(obj, SIGNAL(valueChanged(double, int)), SLOT(setController(double,int)));
                  }
            else {
                  printf("PluginGui::unknown widget class %s\n", obj->metaObject()->className());
                  continue;
                  }
            gw.push_back(w);
            }
}


//---------------------------------------------------------
//   PluginGui
//---------------------------------------------------------

PluginGui::~PluginGui()
      {
      }

//---------------------------------------------------------
//   setController
//---------------------------------------------------------

void PluginGui::setController(double val, int param)
      {
      if (plugin->isInt(param))
            val = rint(val);
      CVal cval;
      cval.f = val;
      song->setControllerVal(plugin->track(), plugin->controller(param), cval);
      }

//---------------------------------------------------------
//   load
//---------------------------------------------------------

void PluginGui::load()
      {
      QString s("presets/plugins/");
      s += plugin->plugin()->label();
      s += "/";

      QStringList pattern;
      const char** p = preset_file_pattern;
      while (*p)
            pattern << *p++;
      QString fn = getOpenFileName(s, pattern, this, tr("MusE: load preset"));
      if (fn.isEmpty())
            return;
      QFile* qf = fileOpen(this, fn, QString(".pre"), QIODevice::ReadOnly, true);
      if (qf == 0)
            return;

      QDomDocument doc;
      int line, column;
      QString err;
      if (!doc.setContent(qf, false, &err, &line, &column)) {
            QString col, ln, error;
            col.setNum(column);
            ln.setNum(line);
            error = err + "\n    at line: " + ln + " col: " + col;
            printf("error reading med file: %s\n", error.toLatin1().data());
            delete qf;
            return;
            }
      QDomNode node = doc.documentElement();

      while (!node.isNull()) {
            QDomElement e = node.toElement();
            if (e.isNull())
                  continue;
            if (e.tagName() == "muse") {
                  // QString version = e.attribute(QString("version"));
                  node = node.firstChild();
                  while (!node.isNull()) {
                        QDomElement e = node.toElement();
                        bool prefader;
                        if (e.tagName() == "plugin")
                              plugin->readConfiguration(node.firstChild(), &prefader);
                        else
                              printf("MusE:PluginGui: unknown tag %s\n", e.tagName().toLatin1().data());
                        node = node.nextSibling();
                        }
                  }
            else
                  printf("MusE: %s not supported\n", e.tagName().toLatin1().data());
            node = node.nextSibling();
            }
      qf->close();
      delete qf;
      }

//---------------------------------------------------------
//   save
//---------------------------------------------------------

void PluginGui::save()
      {
      QString s("presets/plugins/");
      s += plugin->plugin()->label();
      s += "/";

      QStringList pattern;
      const char** p = preset_file_pattern;
      while (*p)
            pattern << *p++;
      QString fn = getSaveFileName(s, pattern, this,
        tr("MusE: save preset"));
      if (fn.isEmpty())
            return;
      QFile* f = fileOpen(this, fn, QString(".pre"), QIODevice::WriteOnly, true);
      if (f == 0)
            return;
      Xml xml(f);
      xml.header();
      xml.tag("muse version=\"1.0\"");
      plugin->writeConfiguration(xml, true);
      xml.etag("muse");
      f->close();
      delete f;
      }

//---------------------------------------------------------
//   bypassToggled
//---------------------------------------------------------

void PluginGui::bypassToggled(bool val)
      {
      plugin->setOn(val);
      song->update(SC_ROUTE);
      }

//---------------------------------------------------------
//   setOn
//---------------------------------------------------------

void PluginGui::setOn(bool val)
      {
//      onOff->blockSignals(true);
      onOff->setChecked(val);
//      onOff->blockSignals(false);
      }

//---------------------------------------------------------
//   updateValue
//---------------------------------------------------------

void PluginGui::updateValue(int parameter, double value)
      {
      for (std::vector<GuiWidget>::iterator i = gw.begin(); i != gw.end(); ++i) {
            int idx = i->parameter;
            if (idx != parameter)
                  continue;
            switch (i->type) {
                  case GuiWidget::SLIDER:
                        ((Slider*)(i->widget))->setValue(value);
                        break;
                  case GuiWidget::FLOAT_ENTRY:
                        {
                        const char* p = plugin->getParameterDisplay(idx, value);
                        if (p)
                              ((FloatEntry*)(i->widget))->setText(QString(p));
                        else
                              ((FloatEntry*)(i->widget))->setValue(value);
                        }
                        break;
                  case GuiWidget::CHECKBOX:
                        ((CheckBox*)(i->widget))->setValue(value);
                        break;
                  case GuiWidget::COMBOBOX:
                        ((ComboBox*)(i->widget))->setValue(value);
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   updateValues
//---------------------------------------------------------

void PluginGui::updateValues()
      {
      int n = plugin->plugin()->parameter();
      for (int i = 0; i < n; ++i) {
            double val = plugin->param(i);
            updateValue(i, val);
            }
      }

//---------------------------------------------------------
//   controllerChanged
//---------------------------------------------------------

void PluginGui::controllerChanged(int id)
      {
      double value = plugin->track()->ctrlVal(id).f;
      for (std::vector<GuiWidget>::iterator i = gw.begin(); i != gw.end(); ++i) {
            int idx = i->parameter;
            if (plugin->controller(idx)->id() != id)
                  continue;
            switch (i->type) {
                  case GuiWidget::SLIDER:
                        ((Slider*)(i->widget))->setValue(value);
                        break;
                  case GuiWidget::FLOAT_ENTRY:
                        {
                        const char* p = plugin->getParameterDisplay(idx, value);
                        if (p)
                              ((FloatEntry*)(i->widget))->setText(QString(p));
                        else
                              ((FloatEntry*)(i->widget))->setValue(value);
                        }
                        break;
                  case GuiWidget::CHECKBOX:
                        ((CheckBox*)(i->widget))->setValue(value);
                        break;
                  case GuiWidget::COMBOBOX:
                        ((ComboBox*)(i->widget))->setValue(value);
                        break;
                  }
            }
      }

//---------------------------------------------------------
//   autoChanged
//---------------------------------------------------------

void PluginGui::autoChanged()
      {
      bool ar = plugin->track()->autoRead();
      bool aw = plugin->track()->autoWrite();

      //  controller are enabled if
      //    autoRead is off
      //    autoRead and autoWrite are on (touch mode)

      bool ec = !ar || (ar && aw);

      for (std::vector<GuiWidget>::iterator i = gw.begin(); i != gw.end(); ++i)
            i->widget->setEnabled(ec);
      }


