//=========================================================
//  MusE
//  Linux Music Editor
//
//  vamgui.c
//	This is a simple GUI implemented with QT for
//	vam software synthesizer.
//	(Many) parts of this file was taken from Werner Schweer's GUI
//	for his organ soft synth.
//
//  (C) Copyright 2002 Jotsif Lindman Hï¿½nlund (jotsif@linux.nu)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02111-1301, USA or point your web browser to http://www.gnu.org.
//=========================================================

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <list>

#include "common_defs.h"
#include "vamgui.h"
#include "vam.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QLCDNumber>
#include <QSignalMapper>
#include <QSlider>
#include <QSocketNotifier>

#include "muse/globals.h"
#include "muse/xml.h"
#include "muse/midi.h"
#include "muse/midictrl.h"
#include "muse/icons.h"

const char *vam_ctrl_names[] = {
  "DCO1_PITCHMOD", "DCO1_WAVEFORM", "DCO1_FM", "DCO1_PWM",
  "DCO1_ATTACK", "DCO1_DECAY", "DCO1_SUSTAIN", "DCO1_RELEASE",
  "DCO2_PITCHMOD", "DCO2_WAVEFORM", "DCO2_FM", "DCO2_PWM",
  "DCO2_ATTACK", "DCO2_DECAY", "DCO2_SUSTAIN", "DCO2_RELEASE",
  "LFO_FREQ", "LFO_WAVEFORM", "FILT_ENV_MOD", "FILT_KEYTRACK",
  "FILT_RES", "FILT_ATTACK", "FILT_DECAY", "FILT_SUSTAIN",
  "FILT_RELEASE", "DCO2ON", "FILT_INVERT", "FILT_CUTOFF",
  "DCO1_DETUNE", "DCO2_DETUNE", "DCO1_PW", "DCO2_PW"
};

//---------------------------------------------------------
//   Preset
//---------------------------------------------------------

struct Preset {
	QString name;
	int ctrl[NUM_CONTROLLER];
	void readConfiguration(MusECore::Xml& xml);
	void readControl(MusECore::Xml& xml);
	void writeConfiguration(MusECore::Xml& xml, int level);
      };

std::list<Preset> presets;
typedef std::list<Preset>::iterator iPreset;

// Removed by T356
//QString museProject;
//QString museGlobalShare;
//QString museUser;
//QString instanceName;

// char* presetFileTypes[] = {
//      "Presets (*.pre)",
//      0
//      };

//---------------------------------------------------------
//   readControl
//---------------------------------------------------------

void Preset::readControl(MusECore::Xml& xml)
{
	int idx = 0;
	int val = 0;
	for (;;) {
		MusECore::Xml::Token token(xml.parse());
		const QString& tag(xml.s1());
		switch (token) {
			case MusECore::Xml::Error:
			case MusECore::Xml::End:
				return;
			case MusECore::Xml::TagStart:
				xml.unknown("control");
				break;
			case MusECore::Xml::Attribut:
				if (tag == "idx") {
					idx = xml.s2().toInt();
						if (idx >= NUM_CONTROLLER)
							idx = 0;
				}
				else if (tag == "val")
					val = xml.s2().toInt();
				break;
			case MusECore::Xml::TagEnd:
				if (tag == "control") {
					ctrl[idx] = val;
					return;
				}
			default:
                    		break;
		}
	}
}

//---------------------------------------------------------
//   readConfiguration
//---------------------------------------------------------

void Preset::readConfiguration(MusECore::Xml& xml)
{
	for (;;) {
		MusECore::Xml::Token token(xml.parse());
		const QString& tag(xml.s1());
		switch (token) {
			case MusECore::Xml::Error:
			case MusECore::Xml::End:
				return;
			case MusECore::Xml::TagStart:
				if (tag == "control")
					readControl(xml);
				else
					xml.unknown("preset");
				break;
			case MusECore::Xml::Attribut:
				if (tag == "name")
					name = xml.s2();
				break;
			case MusECore::Xml::TagEnd:
				if (tag == "preset")
					return;
			default:
				break;
		}
	}
}

//---------------------------------------------------------
//   writeConfiguration
//---------------------------------------------------------

void Preset::writeConfiguration(MusECore::Xml& xml, int level)
{
	//xml.tag(level++, "preset name=\"%s\"", name.ascii());
        xml.tag(level++, "preset name=\"%s\"", MusECore::Xml::xmlString(name).toAscii().constData());
	for (int i = 0; i < NUM_CONTROLLER; ++i) {
		xml.tag(level, "control idx=\"%d\" val=\"%d\" /", i, ctrl[i]);
	}
	xml.tag(level--, "/preset");
}

//---------------------------------------------------------
//   VAMGui
//---------------------------------------------------------

VAMGui::VAMGui()
   : QWidget(0, Qt::Window),
	MessGui()
{
      setupUi(this);
      QSocketNotifier* s = new QSocketNotifier(readFd, QSocketNotifier::Read);
      connect(s, SIGNAL(activated(int)), SLOT(readMessage(int)));

      loadPresets->setIcon(QIcon(*MusEGui::openIcon));
      savePresets->setIcon(QIcon(*MusEGui::saveIcon));
      savePresetsToFile->setIcon(QIcon(*MusEGui::saveasIcon));
      deletePreset->setIcon(QIcon(*MusEGui::deleteIcon));

        // p4.0.27 First ctrl offset.
	dctrl[DCO1_PITCHMOD - VAM_FIRST_CTRL] = SynthGuiCtrl(PitchModS, LCDNumber1,  SynthGuiCtrl::SLIDER);
	dctrl[DCO1_WAVEFORM - VAM_FIRST_CTRL] = SynthGuiCtrl(Waveform, 0, SynthGuiCtrl::COMBOBOX);
	dctrl[DCO1_FM - VAM_FIRST_CTRL] = SynthGuiCtrl(FMS, LCDNumber1_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_PWM - VAM_FIRST_CTRL] = SynthGuiCtrl(PWMS, LCDNumber1_3, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_ATTACK - VAM_FIRST_CTRL] = SynthGuiCtrl(AttackS, LCDNumber1_3_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_DECAY - VAM_FIRST_CTRL] = SynthGuiCtrl(DecayS, LCDNumber1_3_2_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_SUSTAIN - VAM_FIRST_CTRL] = SynthGuiCtrl(SustainS, LCDNumber1_3_2_3, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_RELEASE - VAM_FIRST_CTRL] = SynthGuiCtrl(ReleaseS, LCDNumber1_3_2_4, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_PITCHMOD - VAM_FIRST_CTRL] = SynthGuiCtrl(PitchModS2, LCDNumber1_4, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_WAVEFORM - VAM_FIRST_CTRL] = SynthGuiCtrl(Waveform2, 0, SynthGuiCtrl::COMBOBOX);
	dctrl[DCO2_FM - VAM_FIRST_CTRL] = SynthGuiCtrl(FMS2, LCDNumber1_2_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_PWM - VAM_FIRST_CTRL] = SynthGuiCtrl(PWMS2, LCDNumber1_3_3, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_ATTACK - VAM_FIRST_CTRL] = SynthGuiCtrl(AttackS2, LCDNumber1_3_2_5, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_DECAY - VAM_FIRST_CTRL] = SynthGuiCtrl(DecayS2, LCDNumber1_3_2_2_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_SUSTAIN - VAM_FIRST_CTRL] = SynthGuiCtrl(SustainS2, LCDNumber1_3_2_3_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_RELEASE - VAM_FIRST_CTRL] = SynthGuiCtrl(ReleaseS2, LCDNumber1_3_2_4_2, SynthGuiCtrl::SLIDER);
	dctrl[LFO_FREQ - VAM_FIRST_CTRL] = SynthGuiCtrl(FreqS, LCDNumber1_5, SynthGuiCtrl::SLIDER);
	dctrl[LFO_WAVEFORM - VAM_FIRST_CTRL] = SynthGuiCtrl(Waveform2_2, 0, SynthGuiCtrl::COMBOBOX);
	dctrl[FILT_ENV_MOD - VAM_FIRST_CTRL] = SynthGuiCtrl(EnvModS, LCDNumber1_5_3, SynthGuiCtrl::SLIDER);
	dctrl[FILT_KEYTRACK - VAM_FIRST_CTRL] = SynthGuiCtrl(KeyTrack, 0, SynthGuiCtrl::SWITCH);
	dctrl[FILT_RES - VAM_FIRST_CTRL] = SynthGuiCtrl(ResS, LCDNumber1_5_5,  SynthGuiCtrl::SLIDER);
	dctrl[FILT_ATTACK - VAM_FIRST_CTRL] = SynthGuiCtrl(AttackS3, LCDNumber1_3_2_5_2, SynthGuiCtrl::SLIDER);
	dctrl[FILT_DECAY - VAM_FIRST_CTRL] = SynthGuiCtrl(DecayS3, LCDNumber1_3_2_2_2_2, SynthGuiCtrl::SLIDER);
	dctrl[FILT_SUSTAIN - VAM_FIRST_CTRL] = SynthGuiCtrl(SustainS3, LCDNumber1_3_2_3_2_2, SynthGuiCtrl::SLIDER);
	dctrl[FILT_RELEASE - VAM_FIRST_CTRL] = SynthGuiCtrl(ReleaseS3, LCDNumber1_3_2_4_2_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO2ON - VAM_FIRST_CTRL] = SynthGuiCtrl(DCO2On, 0, SynthGuiCtrl::SWITCH);
	dctrl[FILT_INVERT - VAM_FIRST_CTRL] = SynthGuiCtrl(FilterInvert, 0, SynthGuiCtrl::SWITCH);
	dctrl[FILT_CUTOFF - VAM_FIRST_CTRL] = SynthGuiCtrl(CutoffS, LCDNumber1_5_5_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_DETUNE - VAM_FIRST_CTRL] = SynthGuiCtrl(DetuneS, LCDNumber1_6, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_DETUNE - VAM_FIRST_CTRL] = SynthGuiCtrl(DetuneS2, LCDNumber1_6_2, SynthGuiCtrl::SLIDER);
	dctrl[DCO1_PW - VAM_FIRST_CTRL] = SynthGuiCtrl(PWS, LCDNumber1_2_3, SynthGuiCtrl::SLIDER);
	dctrl[DCO2_PW - VAM_FIRST_CTRL] = SynthGuiCtrl(PWS2, LCDNumber1_2_4, SynthGuiCtrl::SLIDER);


	map = new QSignalMapper(this);
	for (int i = 0; i < NUM_CONTROLLER; ++i) {
		map->setMapping(dctrl[i].editor, i);
		if (dctrl[i].type == SynthGuiCtrl::SLIDER)
			connect((QSlider*)(dctrl[i].editor), SIGNAL(valueChanged(int)), map, SLOT(map()));
		else if (dctrl[i].type == SynthGuiCtrl::COMBOBOX)
			connect((QComboBox*)(dctrl[i].editor), SIGNAL(activated(int)), map, SLOT(map()));
		else if (dctrl[i].type == SynthGuiCtrl::SWITCH)
			connect((QCheckBox*)(dctrl[i].editor), SIGNAL(toggled(bool)), map, SLOT(map()));
	}
	connect(map, SIGNAL(mapped(int)), this, SLOT(ctrlChanged(int)));

	connect(presetList, SIGNAL(itemClicked(QListWidgetItem*)),
		this, SLOT(presetClicked(QListWidgetItem*)));
	// presetNameEdit
	connect(presetSet,   SIGNAL(clicked()), this, SLOT(setPreset()));
	connect(savePresets, SIGNAL(clicked()), this, SLOT(savePresetsPressed()));
	connect(loadPresets, SIGNAL(clicked()), this, SLOT(loadPresetsPressed()));
	connect(deletePreset, SIGNAL(clicked()), this, SLOT(deletePresetPressed()));
	connect(savePresetsToFile, SIGNAL(clicked()), this, SLOT(savePresetsToFilePressed()));

	ctrlHi = 0;
	ctrlLo = 0;
	dataHi = 0;
	dataLo = 0;
  presetFileName = NULL; 

      // work around for probable QT/WM interaction bug.
      // for certain window managers, e.g xfce, this window is
      // is displayed although not specifically set to show();
      // bug: 2811156  	 Softsynth GUI unclosable with XFCE4 (and a few others)
      show();
      hide();
      }

//---------------------------------------------------------
//   ctrlChanged
//---------------------------------------------------------

void VAMGui::ctrlChanged(int idx)
      {
	SynthGuiCtrl* ctrl = &dctrl[idx];
	int val = 0;
	if (ctrl->type == SynthGuiCtrl::SLIDER) {
    		QSlider* slider = (QSlider*)(ctrl->editor);
		int max = slider->maximum();
		val = (slider->value() * 16383 + max/2) / max;
	      }
	else if (ctrl->type == SynthGuiCtrl::COMBOBOX) {
		val = ((QComboBox*)(ctrl->editor))->currentIndex();
	      }
	else if (ctrl->type == SynthGuiCtrl::SWITCH) {
		val = ((QCheckBox*)(ctrl->editor))->isChecked();
	      }
      //sendController(0, idx + MusECore::CTRL_RPN14_OFFSET, val);
      sendController(0, idx + VAM_FIRST_CTRL, val);          // p4.0.27
      }

int VAMGui::getController(int idx)
      {
      // p4.0.27
      if(idx < VAM_FIRST_CTRL || idx > VAM_LAST_CTRL)
      {
        //#ifdef VAM_DEBUG
        printf("VAMGui:: invalid controller number %d\n", idx);
        //#endif
        return 0;
      }
      idx -= VAM_FIRST_CTRL;  
      
      SynthGuiCtrl* ctrl = &dctrl[idx];
      int val = 0;
      if (ctrl->type == SynthGuiCtrl::SLIDER) {
            QSlider* slider = (QSlider*)(ctrl->editor);
        int max = slider->maximum();
        val = (slider->value() * 16383 + max/2) / max;
            }
      else if (ctrl->type == SynthGuiCtrl::COMBOBOX) {
        val = ((QComboBox*)(ctrl->editor))->currentIndex();
            }
      else if (ctrl->type == SynthGuiCtrl::SWITCH) {
        val = ((QCheckBox*)(ctrl->editor))->isChecked();
            }
      return val;
      }

int VAMGui::getControllerInfo(int id, const char** name, int* controller,
   int* min, int* max, int* /*initval*/) const
      {
      if (id >= NUM_CONTROLLER)
            return 0;

      //*controller = id;
      *controller = id + VAM_FIRST_CTRL;    // p4.0.27

      *name = vam_ctrl_names[id];
      const SynthGuiCtrl* ctrl = (const SynthGuiCtrl*)&dctrl[id];
      //int val = 0;
      if (ctrl->type == SynthGuiCtrl::SLIDER) {
            QSlider* slider = (QSlider*)(ctrl->editor);
            *max = 16383; //slider->maximum();
            *min = slider->minimum();
            //val = (slider->value() * 16383 + max/2) / max;
            
            //val = 16383 + 1/2 
            }
      else if (ctrl->type == SynthGuiCtrl::COMBOBOX) {
            //val = ((QComboBox*)(ctrl->editor))->currentIndex();
            *min = 0;
            *max = ((QComboBox*)(ctrl->editor))->count();
            }
      else if (ctrl->type == SynthGuiCtrl::SWITCH) {
            //val = ((QCheckBox*)(ctrl->editor))->isChecked();
            *min=0;
            *max=1;
            }
      return ++id;
      }

//---------------------------------------------------------
//   presetClicked
//---------------------------------------------------------

void VAMGui::presetClicked(QListWidgetItem* item)
{
	if (item == 0)
		return;
	presetNameEdit->setText(item->text());
	Preset* preset = 0;
	for (iPreset i = presets.begin(); i != presets.end(); ++i) {
		if (i->name == item->text()) {
			preset = &*i;
			break;
		}
	}
	activatePreset(preset);
}

//---------------------------------------------------------
//   activatePreset
//---------------------------------------------------------

void VAMGui::activatePreset(Preset* preset)
{
	if (preset == 0) {
		fprintf(stderr, "internal error 1\n");
		exit(-1);
	}
	//for (unsigned int i = 0; i < sizeof(dctrl)/sizeof(*dctrl); ++i) {
        for (unsigned int i = 0; i < NUM_CONTROLLER; ++i) {                 // p4.0.27
 		setParam(i, preset->ctrl[i]);
		ctrlChanged(i);
	}
}

//---------------------------------------------------------
//   setPreset
//---------------------------------------------------------

void VAMGui::setPreset()
{
	if (presetNameEdit->text().isEmpty())
		return;
	for (iPreset i = presets.begin(); i != presets.end(); ++i) {
		if (i->name == presetNameEdit->text()) {
			setPreset(&*i);
			return;
		}
	}
	addNewPreset(presetNameEdit->text());
}

//---------------------------------------------------------
//   addNewPreset
//---------------------------------------------------------

void VAMGui::addNewPreset(const QString& name)
{
	Preset p;
	p.name = name;
	setPreset(&p);
	presets.push_back(p);
	presetList->addItem(name);
}

//---------------------------------------------------------
//   deleteNamedPreset
//---------------------------------------------------------
void VAMGui::deleteNamedPreset(const QString& name)
{
        QListWidgetItem * item = presetList->findItems(name, Qt::MatchExactly)[0];
	if (!item) {
		fprintf(stderr, "%s: Could not find preset!\n", __FUNCTION__);
		return;
	}
	presetList->clearSelection();
	int index = presetList->row(item);
	presetList->takeItem(index);
	for (iPreset i = presets.begin(); i != presets.end(); ++i) {
		if (i->name == name) {
			presets.erase(i);
			break;
		}
	}
}


//---------------------------------------------------------
//   setPreset
//---------------------------------------------------------

void VAMGui::setPreset(Preset* preset)
{
	for (unsigned int i = 0; i < NUM_CONTROLLER; ++i) {
		int val = 0;
		SynthGuiCtrl* ctrl = &dctrl[i];
		if (ctrl->type == SynthGuiCtrl::SLIDER) {
			QSlider* slider = (QSlider*)(ctrl->editor);
			int max = slider->maximum();
			val = (slider->value() * 16383 + max/2) / max;
		}
		else if (ctrl->type == SynthGuiCtrl::COMBOBOX) {
			val = ((QComboBox*)(ctrl->editor))->currentIndex();
		}
		else if (ctrl->type == SynthGuiCtrl::SWITCH) {
			val = ((QCheckBox*)(ctrl->editor))->isChecked();
		}

		preset->ctrl[i] = val;
	}
	//
	// send sysex to synti
	//
#if 0
	putchar(0xf0);
	putchar(MUSE_SYNTH_SYSEX_MFG_ID);    // mess
	putchar(VAM_UNIQUE_ID);              // vam
	putchar(0x3);                        // setPreset
        QByteArray ba = preset->name.toLatin1();
	const char* name = ba.constData();
	while (*name)
		putchar(*name++ & 0x7f);
	putchar(0);
	for (int i = 0; i < NUM_CONTROLLER; ++i) {
		putchar(i);
		putchar(preset->ctrl[i]);
	}
	putchar(0xf7);
#endif
}

//---------------------------------------------------------
//   setParam
//    set param in gui
//    val -- midi value 0 - 16383
//---------------------------------------------------------

void VAMGui::setParam(int param, int val)
      {
	//if (param >= int(sizeof(dctrl)/sizeof(*dctrl))) {
        if (param >= NUM_CONTROLLER) {                       // p4.0.27
                #ifdef VAM_DEBUG
		fprintf(stderr, "vam: set unknown parameter 0x%x to 0x%x\n", param, val);
                #endif
		return;
	      }
	SynthGuiCtrl* ctrl = &dctrl[param];
	ctrl->editor->blockSignals(true);
	if (ctrl->type == SynthGuiCtrl::SLIDER) {
		QSlider* slider = (QSlider*)(ctrl->editor);
		int max = slider->maximum();
		if(val < 0) val = (val * max + 8191) / 16383 - 1;
		else val = (val * max + 8191) / 16383;
		
		slider->setValue(val);
		if (ctrl->label)
			((QLCDNumber*)(ctrl->label))->display(val);
	      }
	else if (ctrl->type == SynthGuiCtrl::COMBOBOX) {
		((QComboBox*)(ctrl->editor))->setCurrentIndex(val);
	      }
	else if (ctrl->type == SynthGuiCtrl::SWITCH) {
		((QCheckBox*)(ctrl->editor))->setChecked(val);
	      }
	ctrl->editor->blockSignals(false);
      }

//---------------------------------------------------------
//   sysexReceived
//---------------------------------------------------------

void VAMGui::sysexReceived(const unsigned char* /*data*/, int /*len*/)
{
    // Removed, this is not used.   p4.0.27
    /*	
        if (len >= 4) {
		//---------------------------------------------
		//  MusE Soft Synth
		//---------------------------------------------

		if (data[0] == MUSE_SYNTH_SYSEX_MFG_ID) {
			if (data[1] == VAM_UNIQUE_ID) {     // vam
				if (data[2] == 2) {        // parameter response
					if (len != 6) {
						fprintf(stderr, "vam gui: bad sysEx len\n");
						return;
					}
					int val = data[4] + (data[5]<<7);
					switch(data[3])
					{
						case DCO1_PITCHMOD:
						case DCO2_PITCHMOD:
						case DCO1_DETUNE:
						case DCO2_DETUNE:
							setParam(data[3], ((val + 1) * 2) - 16383);
							break;
						default:
							setParam(data[3], val);
							break;
					}
					return;
				}
				else if (data[2] == 1) {  // param request
					return;
				}
			}
		}
	}
        
        #ifdef VAM_DEBUG
        fprintf(stderr, "vam gui: unknown sysex received, len %d:\n", len);
	for (int i = 0; i < len; ++i)
		fprintf(stderr, "%02x ", data[i]);
	fprintf(stderr, "\n");
        #endif
  */    
        
}

//---------------------------------------------------------
//   processEvent
//---------------------------------------------------------

void VAMGui::processEvent(const MusECore::MidiPlayEvent& ev)
      {
      if (ev.type() == MusECore::ME_CONTROLLER)
      {
            //setParam(ev.dataA() & 0xfff, ev.dataB());
            // p4.0.27
            int ctl = ev.dataA();
            if(ctl < VAM_FIRST_CTRL || ctl > VAM_LAST_CTRL)
            {
              //#ifdef VAM_DEBUG
              printf("VAMGui:: invalid controller number %d\n", ctl);
              //#endif
              return;
            }
            setParam(ctl - VAM_FIRST_CTRL, ev.dataB());
            
      }      
      else if (ev.type() == MusECore::ME_SYSEX)
            sysexReceived(ev.data(), ev.len());
      else
      {
            #ifdef VAM_DEBUG
            printf("VAMGui::illegal event type received\n");
            #endif
      }      
      }

//---------------------------------------------------------
//   loadPresetsPressed
//---------------------------------------------------------

void VAMGui::loadPresetsPressed()
{
#if 1   // TODO
	QString iname;
	QString s(MusEGlobal::configPath);
	
/*      QString filename = QFileDialog::getOpenFileName(lastdir, QString("*.[Ss][Ff]2"),
                                                      this,
                                                      "Load Soundfont dialog",
                                                      "Choose soundfont");*/
        QString fn = QFileDialog::getOpenFileName(this, tr("MusE: Load VAM Presets"), 
                                                  s, "Presets (*.vam)");

	if (fn.isEmpty())
		return;
	bool popenFlag=false;
	FILE* f = fopen(fn.toAscii().constData(),"r");//fileOpen(this, fn, QString(".pre"), "r", popenFlag, true);
	if (f == 0)
		return;
	presets.clear();
	presetList->clear();

	MusECore::Xml xml(f);
	int mode = 0;
	for (;;) {
		MusECore::Xml::Token token = xml.parse();
		QString tag = xml.s1();
		switch (token) {
			case MusECore::Xml::Error:
			case MusECore::Xml::End:
				return;
			case MusECore::Xml::TagStart:
				if (mode == 0 && tag == "muse")
					mode = 1;
//				else if (mode == 1 && tag == "instrument")
//					mode = 2;

				else if (mode == 2  && tag == "preset") {
					Preset preset;
					preset.readConfiguration(xml);
					presets.push_back(preset);
					presetList->addItem(preset.name);
				}
				else if(mode != 1)
					xml.unknown("SynthPreset");
				break;
			case MusECore::Xml::Attribut:
				if(mode == 1 && tag == "iname") {
//					fprintf(stderr, "%s\n", xml.s2().toLatin1());
					if(xml.s2() != "vam-1.0")
						return;
					else mode = 2;
				}
                    		break;
			case MusECore::Xml::TagEnd:
				if (tag == "muse")
				goto ende;
			default:
				break;
		}
	}
ende:
	if (popenFlag)
		pclose(f);
	else
		fclose(f);

	if (presetFileName) delete presetFileName;
	presetFileName = new QString(fn);
	QString dots ("...");
	fileName->setText(fn.right(32).insert(0, dots));

	if (presets.empty())
		return;
	Preset preset = presets.front();
	activatePreset(&preset);
#endif
}

//---------------------------------------------------------
//   doSavePresets
//---------------------------------------------------------
void VAMGui::doSavePresets(const QString& fn, bool showWarning)
{
    showWarning=showWarning; // prevent of unsused variable warning
#if 1
	bool popenFlag=false;
  if (fn=="") {
    printf("empty name\n");
    return;
    } 
  printf("fn=%s\n",fn.toAscii().constData());
  FILE* f = fopen(fn.toAscii().constData(),"w");//fileOpen(this, fn, QString(".pre"), "w", popenFlag, false, showWarning);
	if (f == 0)
		return;
	MusECore::Xml xml(f);
	xml.header();
	xml.tag(0, "muse version=\"1.0\"");
	xml.tag(0, "instrument iname=\"vam-1.0\" /");

	for (iPreset i = presets.begin(); i != presets.end(); ++i)
		i->writeConfiguration(xml, 1);

	xml.tag(1, "/muse");

	if (popenFlag)
		pclose(f);
	else
		fclose(f);
#endif
}

//---------------------------------------------------------
//   savePresetsPressed
//---------------------------------------------------------

void VAMGui::savePresetsPressed()
{
#if 1 // TODO
        QString s(MusEGlobal::configPath);
	QString fn = QFileDialog::getSaveFileName(this, tr("MusE: Save VAM Presets"), 
                                                  s, "Presets (*.vam)");
	if (fn.isEmpty())
		return;
	doSavePresets (fn, true);
#endif
}


//---------------------------------------------------------
//   savePresetsToFilePressed
//---------------------------------------------------------

void VAMGui::savePresetsToFilePressed()
{
	if (!presetFileName ) {
 
      QString s(MusEGlobal::configPath);
      QString fn = QFileDialog::getSaveFileName(this, tr("MusE: Save VAM Presets"), 
                                                s, "Presets (*.vam)");
      presetFileName = new QString(fn);
      }
  if (*presetFileName == QString(""))
    return;
  //if presetFileName->
	doSavePresets (*presetFileName, false);
}

//---------------------------------------------------------
//   deletePresetPressed
//---------------------------------------------------------

void VAMGui::deletePresetPressed()
{
       deleteNamedPreset (presetList->currentItem()->text());
}

//---------------------------------------------------------
//   readMessage
//---------------------------------------------------------

void VAMGui::readMessage(int)
      {
      MessGui::readMessage();
      }

