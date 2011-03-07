//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: $
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================


#include <QMessageBox>
#include <QDialog>
#include <QStringListModel>
#include <QPushButton>
#include <QItemSelectionModel>
#include <QModelIndexList>
#include <QModelIndex>

#include <math.h>
#include <string.h>
#include "tvieweditor.h"
#include "song.h"
#include "globals.h"
#include "config.h"
#include "gconfig.h"
#include "utils.h"
#include "audio.h"
#include "midi.h"
#include "icons.h"
#include "app.h"
#include "popupmenu.h"
#include "track.h"
#include "trackview.h"
#include "synth.h"

TrackViewEditor::TrackViewEditor(QWidget* parent, TrackViewList* vl) : QDialog(parent)
{
	setupUi(this); 
	_allTracks = song->tracks();
	_viewList = vl;
	//MIDI=0, DRUM, WAVE, AUDIO_OUTPUT, AUDIO_INPUT, AUDIO_GROUP,AUDIO_AUX
	_trackTypes = (QStringList() << "Audio_Out" << "Audio_In" << "Audio_Aux" << "Audio_Group" << "Midi" << "Soft_Synth"); //new QStringList();
	//Populate trackTypes and pass it to cmbTypes 
	cmbType->addItems(_trackTypes);
	QStringList stracks;
	for(ciTrack t = _allTracks->begin(); t != _allTracks->end(); ++t)
	{
		_tracks.push_back((*t));
		switch((*t)->type()) {/*{{{*/
			case Track::MIDI:
			case Track::DRUM:
				_midis.push_back((MidiTrack*)(*t));
				break;
			case Track::WAVE:
				_waves.push_back((WaveTrack*)(*t));
				break;
			case Track::AUDIO_OUTPUT:
				_outputs.push_back((AudioOutput*)(*t));
				stracks << (*t)->name();
				break;
			case Track::AUDIO_GROUP:
				_groups.push_back((AudioGroup*)(*t));
				break;
			case Track::AUDIO_AUX:
				_auxs.push_back((AudioAux*)(*t));
				break;
			case Track::AUDIO_INPUT:
				_inputs.push_back((AudioInput*)(*t));
				break;
			case Track::AUDIO_SOFTSYNTH:
				SynthI* s = (SynthI*)(*t);
				_synthIs.push_back(s);
				break;
		}/*}}}*/
	}
	listAllTracks->setModel(new QStringListModel(stracks));
	btnAdd = actionBox->button(QDialogButtonBox::Yes);
	btnAdd->setText(tr("Add Track"));
	connect(btnAdd, SIGNAL(clicked(bool)), SLOT(btnAddTrack(bool)));
	btnRemove = actionBox->button(QDialogButtonBox::No);
	btnRemove->setText(tr("Remove Track"));
	btnRemove->setFocusPolicy(btnAdd->focusPolicy());
	connect(btnRemove, SIGNAL(clicked(bool)), SLOT(btnRemoveTrack(bool)));
	
	connect(cmbViews, SIGNAL(currentIndexChanged(QString&)), SLOT(cmbViewSelected(QString&)));
	connect(cmbType, SIGNAL(currentIndexChanged(int)), SLOT(cmbTypeSelected(int)));
}


//----------------------------------------------
// Slots
//----------------------------------------------
void TrackViewEditor::cmbViewSelected(QString& sl)
{
	//Perform actions to populate list below based on selected view
}

void TrackViewEditor::cmbTypeSelected(int type)
{
	//Perform actions to populate list below based on selected type
	//We need to repopulate and filter the allTrackList
	//"Audio_Out" "Audio_In" "Audio_Aux" "Audio_Group" "Midi" "Soft_Synth"
	QStringList stracks;
	switch(type) {/*{{{*/
		case 0:
			for(ciTrack t = _outputs.begin(); t != _outputs.end(); ++t)
			{
				//This should be checked against track in other views
				stracks << (*t)->name();
			}
		case 1:
			for(ciTrack t = _inputs.begin(); t != _inputs.end(); ++t)
			{
				//This should be checked against track in other views
				stracks << (*t)->name();
			}
			break;
		case 2:
			for(ciTrack t = _auxs.begin(); t != _auxs.end(); ++t)
			{
				//This should be checked against track in other views
				stracks << (*t)->name();
			}
			break;
		case 3:
			for(ciTrack t = _groups.begin(); t != _groups.end(); ++t)
			{
				//This should be checked against track in other views
				stracks << (*t)->name();
			}
			break;
		case 4:
			for(ciTrack t = _midis.begin(); t != _midis.end(); ++t)
			{
				//This should be checked against track in other views
				stracks << (*t)->name();
			}
			break;
		case 5:
			for(ciTrack t = _synthIs.begin(); t != _synthIs.end(); ++t)
			{
				//This should be checked against track in other views
				stracks << (*t)->name();
			}
			break;
	}/*}}}*/
	listAllTracks->setModel(new QStringListModel(stracks));
}

void TrackViewEditor::btnAddTrack(bool state)
{
	//Perform actions to add action to right list and remove from left
	printf("Add button clicked\n");
	QItemSelectionModel* model = listAllTracks->selectionModel();
	if(model->hasSelection())
	{
		QModelIndexList sel = model->selectedRows(0);
		QList<QModelIndex>::const_iterator id;
		for (id = sel.constBegin(); id != sel.constEnd(); ++id)
		//for(QModelIndex* id = sel.begin(); id != sel.end(); ++id)
		{
			//We have to index we will get the row.
			int row = (*id).row();
			/*QStringListModel* m = */QAbstractItemModel* m = listAllTracks->model();
			QVariant v = m->data((*id));
			QString val = v.toString();
			Track* trk = song->findTrack(val);
			if(trk)
				printf("Adding Track from row: %d\n", row);
				//printf("Found Track %s at index %d with type %d\n", val, row, trk->type());
		}
	}
}

void TrackViewEditor::btnRemoveTrack(bool state)
{
	//Perform action to remove track from the selectedTracks list
	printf("Remove button clicked\n");
}

void TrackViewEditor::setSelectedTracks(TrackList* t)
{
	_selected = t;
	//Call methods to update the display
}

void TrackViewEditor::setTypes(QStringList t)
{
	_trackTypes = t;
	//Call methods to update the display
}

void TrackViewEditor::setViews(TrackViewList* l)
{
	_viewList = l;
	//Call methods to update the display
}
