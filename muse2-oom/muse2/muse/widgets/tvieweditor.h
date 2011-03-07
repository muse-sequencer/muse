//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: $
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#ifndef __TRACKVIEWEDITOR_H__
#define __TRACKVIEWEDITOR_H__

#include "ui_trackvieweditorbase.h"
#include <QList>
#include <QObject>
#include "trackview.h"
#include "track.h"

class TrackView;
class Track;
class QDialog;
class QPushButton;

class TrackViewEditor : public QDialog, public Ui::TrackViewEditorBase 
{
	Q_OBJECT
	TrackList* _allTracks;
	TrackViewList* _viewList;
	TrackList* _selected;
	TrackList _tracks;      // tracklist as seen by arranger
	MidiTrackList  _midis;
	WaveTrackList _waves;
	InputList _inputs;      // audio input ports
	OutputList _outputs;    // audio output ports
	GroupList _groups;      // mixer groups
	AuxList _auxs;          // aux sends
	SynthIList _synthIs;

	QStringList _trackTypes;
	QPushButton* btnAdd;
	QPushButton* btnRemove;

	private slots:
		void cmbViewSelected(QString&);
		void cmbTypeSelected(int);
		void btnAddTrack(bool);
		void btnRemoveTrack(bool);

	public:
		TrackViewEditor(QWidget*, TrackViewList* = 0);
		void setSelectedTracks(TrackList*);
		TrackList* selectedTracks( ) { return _selected; }
		void setTypes(QStringList);
		void setViews(TrackViewList*);
		QStringList trackTypes(){return _trackTypes;}
		TrackViewList* views(){return _viewList;}

		TrackList* tracks()         { return &_tracks;  }
		MidiTrackList* midis()      { return &_midis;   }
		WaveTrackList* waves()      { return &_waves;   }
		InputList* inputs()         { return &_inputs;  }
		OutputList* outputs()       { return &_outputs; }
		GroupList* groups()         { return &_groups;  }
		AuxList* auxs()             { return &_auxs;    }
		SynthIList* syntis()        { return &_synthIs; }
};

#endif
