//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: $
//
//  (C) Copyright 2010 Andrew Williams and Christopher Cherrett
//=========================================================

#include "event.h"
#include "song.h"
#include "xml.h"
#include "globaldefs.h"
#include "trackview.h"
#include "track.h"

TrackView::TrackView()
{
}

TrackView::~TrackView()
{
}

void TrackView::setDefaultName()/*{{{*/
{
	QString base;
	switch(_type) {
		case Track::MIDI:
		case Track::DRUM:
		case Track::WAVE:
			base = QString("Track View");
			break;
		case Track::AUDIO_OUTPUT:
			base = QString("Out View");
			break;
		case Track::AUDIO_GROUP:
			base = QString("Group View");
			break;
		case Track::AUDIO_AUX:
			base = QString("Aux View");
			break;
		case Track::AUDIO_INPUT:
			base = QString("Input View");
			break;
		case Track::AUDIO_SOFTSYNTH:
			base = QString("Synth View");
			break;
	};
	base += " ";
	for (int i = 1; true; ++i) {
		QString n;
		n.setNum(i);
		QString s = base + n;
		TrackView* tv = song->findTrackView(s);
		if (tv == 0) {
			setViewName(s);
			break;
		}
	}
}/*}}}*/

//---------------------------------------------------------
//    addTrack
//---------------------------------------------------------

void TrackView::addTrack(Track* t)/*{{{*/
{
	Track::TrackType type = (Track::TrackType) t->type();
	if(type == _type)
	{
		_tracks.push_back(t);
	}
}/*}}}*/

void TrackView::removeTrack(Track* t)
{
	_tracks.erase(t);
	//This needs to fire something so the gui gets updated
}

//---------------------------------------------------------
//   TrackView::read
//---------------------------------------------------------

void TrackView::read(Xml& xml)/*{{{*/
{
	for (;;) {
		Xml::Token token = xml.parse();
		const QString& tag = xml.s1();
		switch (token) {
			case Xml::Error:
			case Xml::End:
				return;
			case Xml::TagStart:
				if (tag == "vtrack") {
					Track* t = song->findTrack(xml.parse1());
					if(t != 0)
					{
						addTrack(t);
					}
				}
				break;
			case Xml::Attribut:
				if (tag == "name")
					_name = xml.parse1();
				else if (tag == "comment")
					_comment = xml.parse1();
				else if (tag == "selected")
					_selected = xml.parseInt();
				else if(tag == "type")
					_type = (Track::TrackType)xml.parseInt();
				break;
			case Xml::TagEnd:
				break;
			default:
            break;
		}
	}
}/*}}}*/
