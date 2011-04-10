//you need to download http://home.arcor.de/michael.jung11/glyphs.tar.bz2
//and extract it somewhere. then change FONT_PATH to the correct directory
//the trailing slash is necessary
#define FONT_PATH "/home/flo/muse-glyphs/"

//=========================================================
//  MusE
//  Linux Music Editor
//  scoreedit.cpp
//  (C) Copyright 2011 Florian Jung (florian.a.jung@web.de)
//=========================================================


#include <QLayout>
#include <QSizeGrip>
#include <QLabel>
#include <QScrollBar>
#include <QPushButton>
#include <QToolButton>
#include <QToolTip>
#include <QMenu>
#include <QSignalMapper>
#include <QMenuBar>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QAction>
#include <QKeySequence>
#include <QKeyEvent>
#include <QGridLayout>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QMimeData>
#include <QScrollArea>
#include <QSettings>
#include <QImage>

#include <stdio.h>
#include <math.h>

#include <iostream>
#include <sstream>
using namespace std;

#include "xml.h"
#include "mtscale.h"
#include "prcanvas.h"
#include "scoreedit.h"
#include "scrollscale.h"
#include "piano.h"
#include "../ctrl/ctrledit.h"
#include "splitter.h"
#include "ttoolbar.h"
#include "tb1.h"
#include "utils.h"
#include "globals.h"
#include "gconfig.h"
#include "icons.h"
#include "audio.h"

#include "cmd.h"
#include "quantconfig.h"
#include "shortcuts.h"

#include "mtrackinfo.h"

#include "sig.h"


//---------------------------------------------------------
//   ScoreEdit
//---------------------------------------------------------

ScoreEdit::ScoreEdit(PartList* pl, QWidget* parent, const char* name, unsigned initPos)
   : MidiEditor(0, 0, pl, parent, name)
{
	
//	Splitter* hsplitter;
	QPushButton* ctrl;
/*	
	hsplitter = new Splitter(Qt::Vertical, mainw, "hsplitter");
		hsplitter->setHandleWidth(2);

		ctrl = new QPushButton(tr("ctrl"), mainw);
			ctrl->setObjectName("Ctrl");
			ctrl->setFont(config.fonts[3]);
			ctrl->setToolTip(tr("Add Controller View"));

		hsplitter->addWidget(ctrl);
*/	
	/*
	QGridLayout* gridS1 = new QGridLayout(mainw);
		gridS1->setContentsMargins(0, 0, 0, 0);

      mainGrid->setRowStretch(0, 100);
      mainGrid->setColumnStretch(1, 100);

      gridS1->setRowStretch(2, 100);
      gridS1->setColumnStretch(1, 100);     
*/

	ScoreCanvas* test=new ScoreCanvas(this, mainw, 1, 1);	
	QScrollBar* hscroll = new QScrollBar(Qt::Horizontal, mainw);


connect(hscroll, SIGNAL(valueChanged(int)), test,   SLOT(scroll_event(int)));
connect(song, SIGNAL(songChanged(int)), test, SLOT(song_changed(int)));
//	      mainGrid->setRowStretch(0, 100);
//      mainGrid->setColumnStretch(1, 100);
      mainGrid->addWidget(test, 0, 0);
      mainGrid->addWidget(hscroll,1,0);

hscroll->setMinimum(0);
hscroll->setMaximum(1000);
hscroll->setPageStep(100);
//	gridS1->addWidget(test,0,0);
	
//	gridS1->addWidget(canvas,                 0,    0);
	//	hsplitter->addWidget(test);
}


//---------------------------------------------------------
//   ~ScoreEdit
//---------------------------------------------------------

ScoreEdit::~ScoreEdit()
{
	
}


ScoreCanvas::ScoreCanvas(MidiEditor* pr, QWidget* parent,
   int sx, int sy) : View(parent, sx, sy)
{
	editor      = pr;
	setFocusPolicy(Qt::StrongFocus);
	setBg(Qt::white);
	
	setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
//	setMinimumSize(400,600);
	
	load_pixmaps();
	
	x_pos=0;
	dragging=false;
	mouse_erases_notes=false;
	mouse_inserts_notes=true;

	curr_part=editor->parts()->begin()->second;
	
	last_len=384;
	new_len=-1;

	song_changed(0);	
//fertig mit aufbereiten	
	cout << "---------------- CALCULATING DONE ------------------" << endl;

}

void ScoreCanvas::song_changed(int)
{
	cout << "song changed!" << endl;
	pos_add_list.clear();
	eventlist=createAppropriateEventList(editor->parts());
	itemlist=create_itemlist(eventlist);
	process_itemlist(itemlist); // do note- and rest-grouping and collision avoiding
	calc_item_pos(itemlist);
	redraw();
	cout << "song had changed, recalculation complete" << endl;
}

//flo code starting here

string IntToStr(int i)
{
	ostringstream s;
	s<<i;
	return s.str();
}

void load_colored_pixmaps(string file, QPixmap* array)
{
	QString fn(file.c_str());
	QImage img(fn);
	
	int bytes=img.byteCount();
	uchar* bits=img.bits();
	uchar* ptr;
	
	array[BLACK_PIXMAP]=QPixmap::fromImage(img); //before anything was changed
	//TODO: really color it black, don't rely on the userdata be correct
	
	for (int color_index=0;color_index<NUM_PARTCOLORS; color_index++)
	{
		ptr=bits;
		int r,g,b;
		config.partColors[color_index].getRgb(&r,&g,&b);
		
		for (int i=0; i<bytes/4; i++)
		{
			QRgb* rgb=((QRgb*)ptr);
			(*rgb) = qRgba(r,g,b,qAlpha(*rgb));
			
			ptr+=4;
		}

		array[color_index]=QPixmap::fromImage(img);
	}
}


void ScoreCanvas::load_pixmaps()
{
	load_colored_pixmaps(FONT_PATH "whole.png", pix_whole);
	load_colored_pixmaps(FONT_PATH "half.png", pix_half);
	load_colored_pixmaps(FONT_PATH "quarter.png", pix_quarter);
	load_colored_pixmaps(FONT_PATH "dot.png", pix_dot);
	load_colored_pixmaps(FONT_PATH "acc_none.png", pix_noacc);
	load_colored_pixmaps(FONT_PATH "acc_sharp.png", pix_sharp);
	load_colored_pixmaps(FONT_PATH "acc_b.png", pix_b);

	pix_r1.load(FONT_PATH "rest1.png");
	pix_r2.load(FONT_PATH "rest2.png");
	pix_r4.load(FONT_PATH "rest4.png");
	pix_r8.load(FONT_PATH "rest8.png");
	pix_r16.load(FONT_PATH "rest16.png");
	pix_flag_up[0].load(FONT_PATH "flags8u.png");
	pix_flag_up[1].load(FONT_PATH "flags16u.png");
	pix_flag_up[2].load(FONT_PATH "flags32u.png");
	pix_flag_up[3].load(FONT_PATH "flags64u.png");
	pix_flag_down[0].load(FONT_PATH "flags8d.png");
	pix_flag_down[1].load(FONT_PATH "flags16d.png");
	pix_flag_down[2].load(FONT_PATH "flags32d.png");
	pix_flag_down[3].load(FONT_PATH "flags64d.png");
	
	for (int i=0;i<10;i++)
		pix_num[i].load(QString((string(FONT_PATH "")+IntToStr(i)+string(".png")).c_str()));	
}



int modulo(int a, int b) // similar to a % b
{
	return (((a%b)+b)%b);
}

#define DEFAULT_REST_HEIGHT 6 // TODO


bool operator< (const note_pos_t& a, const note_pos_t& b)
{
	if (a.height<b.height) return true;
	if (a.height>b.height) return false;
	return a.vorzeichen<b.vorzeichen;
}


//TODO: all das unten richtig machen!
#define TICKS_PER_WHOLE (config.division*4) 
#define SONG_LENGTH (song->len())

#define quant_max 3  //whole, half, quarter = 0,1,2
#define quant_max_fraction (1 << quant_max) //whole, half, quarter= 1,2,4
#define FLO_QUANT (TICKS_PER_WHOLE/quant_max_fraction)
//FLO_QUANT = how many ticks has a single quantisation area?


//FINDMICH MARKER
//TODO: quant_max richtig setzen!

/* builds the event list used by the score editor.
 * that list contains only note-on and -off, time-sig- and
 * key-change events.
 * it stores them sorted by their time (quantized); if more
 * events with the same time occur, the NOTE-OFFs are
 * put before the NOTE-ONs
 * it only operates on parts which were selected in the
 * arranger when the score viewer was started
 * 
 * this abstracts the rest of the renderer from muse's internal
 * data structures, making this easy to port to another application
 */
 
int flo_quantize(int tick)
{
	//TODO quantizing must be done with the proper functions!
	return int(rint((float)tick / FLO_QUANT))*FLO_QUANT;
}
 
int flo_quantize_floor(int tick)
{
	//TODO quantizing must be done with the proper functions, see above
	return int(tick / FLO_QUANT) * FLO_QUANT;
}
 
ScoreEventList ScoreCanvas::createAppropriateEventList(PartList* pl)
{
	using AL::sigmap;
	using AL::iSigEvent;

	ScoreEventList result;
	
	// phase one: fill the list -----------------------------------------
	
	//insert note on events
	for (iPart partIt=pl->begin(); partIt!=pl->end(); partIt++)
	{
		Part* part=partIt->second;
		EventList* el=part->events();
		
		for (iEvent it=el->begin(); it!=el->end(); it++)
		{
			Event& event=it->second;
			
			if (event.isNote() && !event.isNoteOff())
			{
				unsigned begin, end;
				begin=flo_quantize(event.tick()+part->tick());
				end=flo_quantize(event.endTick()+part->tick());
				cout <<"inserting note on at "<<begin<<" with pitch="<<event.pitch()<<" and len="<<end-begin<<endl;
				result.insert(pair<unsigned, FloEvent>(begin, FloEvent(begin,event.pitch(), event.velo(),end-begin,FloEvent::NOTE_ON,part,&it->second)));
			}
			//else ignore it
		}
	}

	//insert bars and time signatures
	for (iSigEvent it=sigmap.begin(); it!=sigmap.end(); it++)
	{
		unsigned from=it->second->tick;
		unsigned to=it->first;
		unsigned ticks_per_measure=sigmap.ticksMeasure(it->second->tick);
		
		if (to > unsigned(SONG_LENGTH))
		{
			cout << "time signature's end-of-validness is outside of our song, limiting it." << endl;
			to=SONG_LENGTH;
		}
		
		cout << "new signature from tick "<<from<<" to " << to << ": "<<it->second->sig.z<<"/"<<it->second->sig.n<<"; ticks per measure = "<<ticks_per_measure<<endl;
		result.insert(pair<unsigned, FloEvent>(from,  FloEvent(from, FloEvent::TIME_SIG, it->second->sig.z, it->second->sig.n) ) );
		for (unsigned t=from; t<to; t+=ticks_per_measure)
			result.insert(pair<unsigned, FloEvent>(t,  FloEvent(t,0,0,ticks_per_measure,FloEvent::BAR) ) );
	}


	//TODO FINDMICH MARKER
	result.insert(pair<unsigned, FloEvent>(0,  FloEvent(0,FloEvent::KEY_CHANGE, C ) ) );
	
	
	// phase two: deal with overlapping notes ---------------------------
	ScoreEventList::iterator it, it2;
	
	//iterate through all note_on - events
	for (it=result.begin(); it!=result.end(); it++)
		if (it->second.type==FloEvent::NOTE_ON)
		{
			int end_tick=it->first + it->second.len;
			
			//iterate though all (relevant) later note_ons which are
			//at the same pitch. if there's a collision, shorten it's len
			for (it2=it, it2++; it2!=result.end() && it2->first < end_tick; it2++)
				if ((it2->second.type==FloEvent::NOTE_ON) && (it2->second.pitch == it->second.pitch))
					it->second.len=it2->first - it->first;
		}
		
		
		// phase three: eliminate zero-length-notes -------------------------
		for (it=result.begin(); it!=result.end();)
			if ((it->second.type==FloEvent::NOTE_ON) && (it->second.len<=0))
				result.erase(it++);
			else
				it++;
	
	return result;
}


bool is_sharp_key(tonart_t t)
{
	return ((t>=SHARP_BEGIN) && (t<=SHARP_END));
}
bool is_b_key(tonart_t t)
{
	return ((t>=B_BEGIN) && (t<=B_END));
}

int n_accidentials(tonart_t t)
{
	if (is_sharp_key(t))
		return t-SHARP_BEGIN-1;
	else
		return t-B_BEGIN-1;
}


//note needs to be 0..11
//always assumes violin clef
//only for internal use
note_pos_t ScoreCanvas::note_pos_(int note, tonart_t key)
{
	note_pos_t result;
	           //C CIS D DIS E F FIS G GIS A AIS H
	int foo[12]={0,-1, 1,-1, 2,3,-1, 4,-1, 5, -1,6};
	
	if ((note<0) || (note>=12))
		cout << "WARNING: ILLEGAL FUNCTION CALL (note_pos, note out of range)" << endl;
	
	if (foo[note]!=-1)
	{
		result.height=foo[note];
		result.vorzeichen=NONE;
	}
	else
	{
		if (is_sharp_key(key))
		{
			result.height=foo[note-1];
			result.vorzeichen=SHARP;
		}
		else // if is_b_key
		{
			result.height=foo[note+1];
			result.vorzeichen=B;			
		}
	}
	
	// Special cases for GES / FIS keys
	if (key==GES)
	{
		// convert a H to a Ces
		if (note==11)
		{
			result.height=12; 
			result.vorzeichen=B;
		}
	}
	else if (key==FIS)
	{
		// convert a F to an Eis
		if (note==5)
		{
			result.height=2;
			result.vorzeichen=SHARP;
		}
	}

	return result;
}


//  V   --------------------------  <-- height=10
//  I C --------------------------  <-- height=8
//  O L --------------------------  <-- height=6
//  L E --------------------------  <-- height=4
//  I F --------------------------  <-- height=2
//  N    --o--                      <-- this is C4. height=0

// the "spaces" in between the lines have odd numbers.
// that is, the space between line 2 and 4 is numbered 3.

// these numbers do not change when clef changes. line 2
// is always the "bottom line" of the system.
// in violin clef, line 2 is E4
// in bass clef, line 2 is G2

note_pos_t ScoreCanvas::note_pos (int note, tonart_t key, clef_t clef)
{
	int octave=(note/12)-1; //integer division
	note=note%12;
	
	//now octave contains the octave the note is in
	//(A4 is the 440Hz tone. C4 is the "low C" in the violin clef
	//and the "high C" in the bass clef.
	//note contains 0 for C, 1 for Cis, ..., 11 for H (or B if you're not german)
	
	note_pos_t pos=note_pos_(note,key);
	
	switch (clef) //CLEF_MARKER
	{
		case VIOLIN:
			pos.height=pos.height + (octave-4)*7;
			break;
			
		case BASS:
			pos.height=pos.height + (octave-3)*7 + 5;
			break;
	}
	
	return pos;
}


int ScoreCanvas::calc_len(int l, int d)
{
	// l=0,1,2 -> whole, half, quarter (think of 2^0, 2^1, 2^2)
	// d=number of dots
	
	int tmp=0;
	for (int i=0;i<=d;i++)
		tmp+=TICKS_PER_WHOLE / pow(2, l+i);
	
	return tmp;
}

bool operator< (const note_len_t& a,const note_len_t& b) //TODO sane sorting order
{
	if (a.len<b.len) return true;
	else if (a.dots<b.dots) return true;
	else return false;
}



int calc_measure_len(const list<int>& nums, int denom)
{
	int sum=0;
	
	for (list<int>::const_iterator it=nums.begin(); it!=nums.end(); it++)
		sum+=*it;
	
	return 64* sum/denom;
}

vector<int> create_emphasize_list(const list<int>& nums, int denom)
{
	cout << "creating emphasize list for ";
	for (list<int>::const_iterator it=nums.begin(); it!=nums.end(); it++)
		cout << *it << " ";
	cout << "/ "<<denom;
	
	//        |----- 8th -----|
	int foo[]={4,7,6,7,5,7,6,7}; //if 64 changes, this also must change
	int pos=0;
	int len=calc_measure_len(nums, denom);

	vector<int> result(len);
	
	for (int i=0;i<len;i++)
		result[i]=foo[i%8];
	
	for (list<int>::const_iterator it=nums.begin(); it!=nums.end(); it++)
	{
		result[pos]=1;
		for (int i=1;i<*it;i++)
			result[pos + i*64/denom]=2;
		pos+= *it * 64 / denom;
	}
	
	result[0]=0;
	
	for (int i=0;i<len;i++)
	{
		if (i%8==0)
			cout << endl<<i<<":\t";
		cout << result[i]<<" ";
	}
	cout << endl;
	
	return result;
}

vector<int> create_emphasize_list(int num, int denom) //TODO FINDMICH
{
	list<int> nums;
	
	if (num%3 ==0)
	{
		for (int i=0;i<num/3;i++)
			nums.push_back(3);
	}
	else if (num%2 ==0)
	{
		for (int i=0;i<num/2;i++)
			nums.push_back(2);
	}
	else // num is odd
	{
		for (int i=0;i<(num-3)/2;i++)
			nums.push_back(2);
		
		nums.push_back(3);
	}
	
	return create_emphasize_list(nums, denom);
}

//quant_max must be in log(len), that is
//whole, half, quarter, eighth = 0,1,2,3
//NOT:  1,2,4,8! (think of 2^foo)
//len is in ticks
list<note_len_t> ScoreCanvas::parse_note_len(int len_ticks, int begin_tick, vector<int>& foo, bool allow_dots, bool allow_normal)
{
	list<note_len_t> retval;
	
	if (allow_normal)
	{
		int dot_max = allow_dots ? quant_max : 0;
		
		for (int i=0;i<=quant_max;i++)
			for (int j=0;j<=dot_max-i;j++)
				if (calc_len(i,j) == len_ticks)
				{
					retval.push_back(note_len_t (i,j));
					return retval;
				}
	}
	
	//if !allow_normal or if the above failed
	
	int begin=begin_tick * 64 / TICKS_PER_WHOLE;
	int len=len_ticks * 64 / TICKS_PER_WHOLE;
	
	int pos=begin;
	int len_done=0;
	
	while (len_done<len)
	{
		int len_now=0;
		int last_number=foo[pos];
		
		do {pos++;len_done++;len_now++;} while (! ((foo[pos]<=last_number) || (len_done==len) || (pos==foo.size())) );

		len_now=len_now*TICKS_PER_WHOLE/64;

		cout << "add " << len_now << " ticks" << endl;
		if (allow_dots)
		{
			int dot_max = quant_max;
			
			for (int i=0;i<=quant_max;i++)
				for (int j=0;j<=dot_max-i;j++)
					if (calc_len(i,j) == len_now)
					{
						retval.push_back(note_len_t (i,j));
						len_now=0;
					}
		}
			
		if (len_now) //the above failed or allow_dots=false
		{
			for (int i=0; i<=quant_max; i++)
			{
				int tmp=calc_len(i,0);
				if (tmp <= len_now)
				{
					retval.push_back(note_len_t(i));
					len_now-=tmp;
					if (len_now==0) break;
				}
			}
		}
		
		if (len_now!=0)
			cout << "WARNING: THIS SHOULD NEVER HAPPEN. wasn't able to split note len properly; len_now="<<len_now << endl;

		if (pos==foo.size()) //we cross measure boundaries?
			pos=0;
	}

	
	return retval;
}


#define USED_CLEF VIOLIN

#define YLEN 10
#define YDIST (2*YLEN)
#define NOTE_XLEN 10
#define NOTE_SHIFT 3
#define PIXELS_PER_WHOLE (320) //how many px are between two wholes?
#define PIXELS_PER_NOTEPOS (PIXELS_PER_WHOLE/quant_max_fraction)  //how many px are between the smallest drawn beats?

//PIXELS_PER_NOTEPOS must be greater or equal to 3*NOTE_XLEN + 2*NOTE_SHIFT
//because if tick 0 is at x=0: the notes can be shifted by NOTE_SHIFT.
//additionally, they can be moved by NOTE_XLEN (collision avoiding)
//then, they have their own width, which is NOTE_XLEN/2 into the x>0-area
//the same thing applies to the x<0-area

//  OOO
//   |
//   ^actual calculated x, without shifting or moving
//  ^ and
//    ^   : moved note (by XLEN)
// additionally, a shift is possible
// total_width = shift + move + note_xlen + move + shift, where move==note_xlen
// total_width = 2*shift + 3*note_xlen
// if total_width is greater than px_per_notepos, there will be collisions!

#define NOTE_MOVE_X (PIXELS_PER_NOTEPOS/2)
//TODO richtige werte finden!
#define REST_AUSWEICH_X 10
#define DOT_XDIST 6
#define DOT_XBEGIN 10
#define DOT_XBEGIN_REST 10

#define TIMESIG_POSADD 20
#define NUMBER_HEIGHT (pix_num[0].height())

//kann 0 oder 1 sein:
//bei notenkollisionen mit ungerader anzahl von kollidierenden
//wird immer so ausgewichen, dass möglichst wenige ausweichen müssen
//wenn die anzahl aber gerade ist, gibt es keine "bessere" lösung
//in dem fall werden immer die geraden (0) bzw. ungeraden (1)
//ausweichen.
#define AUSWEICHEN_BEVORZUGT 0

#define STEM_LEN 30

#define DOTTED_RESTS true
#define UNSPLIT_RESTS false

#define AUX_LINE_LEN 1.5

#define ACCIDENTIAL_DIST 11
#define KEYCHANGE_ACC_DIST 9
#define KEYCHANGE_ACC_LEFTDIST 3
#define KEYCHANGE_ACC_RIGHTDIST 3


#define stdmap std::map

#define no_notepos note_pos_t()

#define TIE_DIST 5
#define TIE_HEIGHT 6
#define TIE_THICKNESS 3

void ScoreCanvas::draw_tie (QPainter& p, int x1, int x4, int yo, bool up, QColor color)
{
	QPainterPath path;

	int y1, y2, y3;

	if (up)
	{
		y1 = yo - TIE_DIST;
		y2 = y1 - TIE_HEIGHT;
		y3=y2-TIE_THICKNESS;
	}
	else 
	{
		y1 = yo + TIE_DIST;
		y2 = y1 + TIE_HEIGHT;
		y3=y2+TIE_THICKNESS;
	}
	
	int x2 = x1 + (x4-x1)/4;
	int x3 = x4 - (x4-x1)/4;
	
	path.moveTo(x1,y1);
	path.cubicTo( x2,y2  ,  x3,y2  ,  x4,y1 );
	path.cubicTo( x3,y3  ,  x2,y3  ,  x1,y1 );
	
	p.setPen(color);
 	p.setBrush(color);

	p.drawPath(path);
}

ScoreItemList ScoreCanvas::create_itemlist(ScoreEventList& eventlist)
{
	ScoreItemList itemlist;
	tonart_t tmp_key=C;
	int lastevent=0;
	int next_measure=-1;
	int last_measure=0;
	vector<int> emphasize_list=create_emphasize_list(4,4); //actually unneccessary, for safety

	for (ScoreEventList::iterator it=eventlist.begin(); it!=eventlist.end(); it++)
	{
		int t, pitch, len, velo, actual_tick;
		FloEvent::typeEnum type;
		t=it->first;
		pitch=it->second.pitch;
		velo=it->second.vel;
		len=it->second.len;
		type=it->second.type;
		actual_tick=it->second.tick;
		if (actual_tick==-1) actual_tick=t;
		
		note_pos_t notepos=note_pos(pitch,tmp_key,USED_CLEF); //TODO einstellmöglichkeiten
		
		printf("FLO: t=%i\ttype=%i\tpitch=%i\tvel=%i\tlen=%i\n",it->first, it->second.type, it->second.pitch, it->second.vel, it->second.len);
		cout << "\tline="<<notepos.height<<"\tvorzeichen="<<notepos.vorzeichen << endl;
		
				
		if (type==FloEvent::BAR)
		{
			// if neccessary, insert rest at between last note and end-of-measure
			int rest=t-lastevent;
			if (rest)
			{
				printf("\tend-of-measure: set rest at %i with len %i\n",lastevent,rest);
				
				list<note_len_t> lens=parse_note_len(rest,lastevent-last_measure,emphasize_list,DOTTED_RESTS,UNSPLIT_RESTS);
				unsigned tmppos=lastevent;
				for (list<note_len_t>::iterator x=lens.begin(); x!=lens.end(); x++)
				{
					cout << "\t\tpartial rest with len="<<x->len<<", dots="<<x->dots<<endl;
					itemlist[tmppos].insert( FloItem(FloItem::REST,notepos,x->len,x->dots) );
					tmppos+=calc_len(x->len,x->dots);
					itemlist[tmppos].insert( FloItem(FloItem::REST_END,notepos,0,0) );
				}
			}
			
			lastevent=t;
			last_measure=t;
			next_measure=t+len;
			
			itemlist[t].insert( FloItem(FloItem::BAR,no_notepos,0,0) );
		}
		else if (type==FloEvent::NOTE_ON)
		{
			int rest=t-lastevent;
			if (rest)
			{
				printf("\tset rest at %i with len %i\n",lastevent,rest);
				// no need to check if the rest crosses measure boundaries;
				// it can't.
				
				list<note_len_t> lens=parse_note_len(rest,lastevent-last_measure,emphasize_list,DOTTED_RESTS,UNSPLIT_RESTS);
				unsigned tmppos=lastevent;
				for (list<note_len_t>::iterator x=lens.begin(); x!=lens.end(); x++)
				{
					cout << "\t\tpartial rest with len="<<x->len<<", dots="<<x->dots<<endl;
					itemlist[tmppos].insert( FloItem(FloItem::REST,notepos,x->len,x->dots) );
					tmppos+=calc_len(x->len,x->dots);
					itemlist[tmppos].insert( FloItem(FloItem::REST_END,notepos,0,0) );
				}
			}
			
			
			
			printf("\tset note at %i with len=%i\n", t, len);

			int tmplen;
			bool tied_note;

			// if the note exceeds the current measure, split it.
			if (t+len>next_measure)
			{
				tmplen=next_measure-t;
				tied_note=true;
				
				//append the "remainder" of the note to our EventList, so that
				//it gets processed again when entering the new measure
				int newlen=len-tmplen;
				eventlist.insert(pair<unsigned, FloEvent>(next_measure, FloEvent(actual_tick,pitch, velo,0,FloEvent::NOTE_OFF, it->second.source_part, it->second.source_event)));
				eventlist.insert(pair<unsigned, FloEvent>(next_measure, FloEvent(actual_tick,pitch, velo,newlen,FloEvent::NOTE_ON, it->second.source_part, it->second.source_event)));

				cout << "\t\tnote was split to length "<<tmplen<<" + " << newlen<<endl;
			}
			else
			{
				tmplen=len;
				tied_note=false;
				
				cout << "\t\tinserting NOTE OFF at "<<t+len<<endl;
				eventlist.insert(pair<unsigned, FloEvent>(t+len,   FloEvent(t+len,pitch, velo,0,FloEvent::NOTE_OFF,it->second.source_part, it->second.source_event)));
			}
							
			list<note_len_t> lens=parse_note_len(tmplen,t-last_measure,emphasize_list,true,true);
			unsigned tmppos=t;
			int n_lens=lens.size();
			int count=0;			
			for (list<note_len_t>::iterator x=lens.begin(); x!=lens.end(); x++)
			{
				cout << "\t\tpartial note with len="<<x->len<<", dots="<<x->dots<<endl;
				count++;
				
				bool tie;
				
				if (count<n_lens)
					tie=true;      // all notes except the last are always tied
				else
					tie=tied_note; // only the last respects tied_note
				
				itemlist[tmppos].insert( FloItem(FloItem::NOTE,notepos,x->len,x->dots, tie, actual_tick, it->second.source_part, it->second.source_event) );
				tmppos+=calc_len(x->len,x->dots);
				itemlist[tmppos].insert( FloItem(FloItem::NOTE_END,notepos,0,0) );
			}
		}
		else if (type==FloEvent::NOTE_OFF)
		{
			lastevent=t;
		}
		else if (type==FloEvent::TIME_SIG)
		{
			cout << "inserting TIME SIGNATURE "<<it->second.num<<"/"<<it->second.denom<<" at "<<t<<endl;
			itemlist[t].insert( FloItem(FloItem::TIME_SIG, it->second.num, it->second.denom) );
			
			emphasize_list=create_emphasize_list(it->second.num, it->second.denom);
		}
		else if (type==FloEvent::KEY_CHANGE)
		{
			cout << "inserting KEY CHANGE ("<<it->second.tonart<<") at "<<t<<endl;
			itemlist[t].insert( FloItem(FloItem::KEY_CHANGE, it->second.tonart) );
			tmp_key=it->second.tonart; // TODO FINDMICH MARKER das muss schöner werden
		}
	}	

	return itemlist;
}

void ScoreCanvas::process_itemlist(ScoreItemList& itemlist)
{
	stdmap<int,int> occupied;
	int last_measure=0;
	vector<int> emphasize_list=create_emphasize_list(4,4); //unneccessary, only for safety

	//iterate through all times with items
	for (ScoreItemList::iterator it2=itemlist.begin(); it2!=itemlist.end(); it2++)
	{
		set<FloItem, floComp>& curr_items=it2->second;
		
		cout << "at t="<<it2->first<<endl;
		
		// phase 0: keep track of active notes, rests -------------------
		//          (and occupied lines) and the last measure
		//          and the current time signature (TODO FINDMICH)
		for (set<FloItem, floComp>::iterator it=curr_items.begin(); it!=curr_items.end(); it++)
		{
			if ((it->type==FloItem::NOTE) || (it->type==FloItem::REST))
				occupied[it->pos.height]++;
			else if ((it->type==FloItem::NOTE_END) || (it->type==FloItem::REST_END))
				occupied[it->pos.height]--;
			else if (it->type==FloItem::BAR)
				last_measure=it2->first;
			else if (it->type==FloItem::TIME_SIG)
				emphasize_list=create_emphasize_list(it->num, it->denom);
		}
		
		cout << "occupied: ";
		for (stdmap<int,int>::iterator i=occupied.begin(); i!=occupied.end(); i++)
			if (i->second) cout << i->first << "("<<i->second<<")   ";
		cout << endl;
		
		
		
		
		
		// phase 1: group rests together -----------------------------------
		int n_groups=0;
		bool dont_group=false;
		
		//iterate through all rests R at that time
		//  iterate through all rests X at that time below R
		//    if something is between X and R ("barrier"), stop
		//    else: group them together
		for (set<FloItem, floComp>::iterator it=curr_items.begin(); it!=curr_items.end();)
		{
			//only operate on rests; ignore rests which are created by this code
			//(can be seen on already_grouped)
			if ((it->type==FloItem::REST) && (it->already_grouped==false))
			{
				cout << "trying to group" << endl;
				
				int lastheight;
				int height_cumulative=0;
				int counter=0;
				
				lastheight=it->pos.height;
				
				set<FloItem, floComp>::iterator tmp;
				for (tmp=it; tmp!=curr_items.end();)
				{
					cout << "checking if we can proceed with an item at height="<<tmp->pos.height<<endl;
					
					for (int i=lastheight+1; i<=tmp->pos.height-1; i++)
						if (occupied[i]!=0)
						{
							cout << "we can NOT, because occ["<<i<<"] != 0" << endl;
							//stop grouping that rest
							goto get_out_here;
						}
					
					lastheight=tmp->pos.height;
					
					// the current item is a rest with equal len? cool!
					if (tmp->type==FloItem::REST && tmp->len==it->len && tmp->dots==it->dots)
					{
						// füge diese pause zur gruppe dazu und entferne sie von diesem set hier
						// entfernen aber nur, wenn sie nicht it, also die erste pause ist, die brauchen wir noch!
						cout << "\tgrouping rest at height="<<tmp->pos.height<<endl;
						height_cumulative+=tmp->pos.height;
						counter++;
						if (tmp!=it)
							curr_items.erase(tmp++);
						else
							tmp++;
					}
					else //it's something else? well, we can stop grouping that rest then
					{
						cout << "we can NOT, because that item is not a rest" << endl;
						//stop grouping that rest
						goto get_out_here;
					}
				}
				cout << "no items to proceed on left, continuing" << endl;
				get_out_here:
				
				n_groups++;
				
				// entferne it vom set und
				// füge eine pause mit dem "mittelwert" ein.
				// occupied und die "_END"-events bleiben unberührt

				FloItem temp=*it;
				temp.already_grouped=true;
				
				// have we grouped all available rests into one single?
				if ( (n_groups==1) && (tmp==curr_items.end()) && !dont_group)
				{
					cout << "wow, we were able to group all rests into one single" << endl;
					if (temp.len==0) //the whole rest is shifted one line (one space and one line)
						temp.pos.height=DEFAULT_REST_HEIGHT+2;
					else
						temp.pos.height=DEFAULT_REST_HEIGHT;
				}
				else
				{
					cout << "creating group #"<<n_groups<<endl;
					temp.pos.height=rint((float)height_cumulative/counter);
				}
				
				// do NOT first insert, then erase, because if temp.height ==
				// it->height, the set considers temp and it equal (it doesn't
				// take already_grouped into account)
				// the result of this: insert does nothing, and erase erases
				// the item. effect: you don't have the rest at all
				curr_items.erase(it++);
				
				cout << "replacing all grouped rests with a rest at height="<<temp.pos.height<<endl;
				
				curr_items.insert(temp);
			}
			else
			{
				if (it->type==FloItem::NOTE)
					dont_group=true;
				
				it++;
			}
		}
	
		
		
		
		
		// phase 2: avoid collisions of items ------------------------------
		set<FloItem, floComp>::iterator lastit, groupbegin, invalid;
		invalid=curr_items.end();
		lastit=invalid;
		groupbegin=invalid;
		int count;
		
		//TODO FINDMICH MARKER: is "grouping" notes and rests together okay?
		// or is it better to ignore rests when grouping?
		for (set<FloItem, floComp>::iterator it=curr_items.begin(); it!=curr_items.end(); it++)
			if ( (it->type==FloItem::NOTE) || (it->type==FloItem::REST) )
			{
				if (lastit != invalid)
				{
					if (it->pos.height == lastit->pos.height+1) // they would collide?
					{
						if (groupbegin==invalid) // we have no group atm?
						{
							groupbegin=lastit;     // start a new group
							count=1; // because lastit has to be taken into account.
							         // for "it", there's a count++ later
						}

						// the following will work even on start-new-group, 
						// because lastit will be "untouched", and that's why 
						// still be initalized to "false"
						it->ausweich=!lastit->ausweich;

						count++;
					}
					else
					{
						if (groupbegin!=invalid) //this is the first item which 
						{												 //doesn't belong to the previous group any more
							if (count%2 == 0) //count is even?
								if (modulo(groupbegin->pos.height, 2) == AUSWEICHEN_BEVORZUGT)
									for (set<FloItem, floComp>::iterator tmp=groupbegin; tmp!=it; tmp++)
										tmp->ausweich=!tmp->ausweich;
							
							groupbegin=invalid;
						}
						// else: everything is ok :)
					}
				}
				
				lastit=it;
			}
			
			// this could be the case if the last processed item before end()
			// still belonged to a group. finalize this last group as well:
			if (groupbegin!=invalid) 
			{
				if (count%2 == 0) //count is even?
					if (modulo(groupbegin->pos.height, 2) == AUSWEICHEN_BEVORZUGT)
						for (set<FloItem, floComp>::iterator tmp=groupbegin; tmp!=curr_items.end(); tmp++)
							tmp->ausweich=!tmp->ausweich;
			}
			// else: everything is ok :)
	
	
	
	
	
		// phase 3: group notes by their length and ------------------------
		//          find out appropriate stem directions
group_them_again:
		stdmap<int, cumulative_t> lengths;
		bool has_whole=false;
		
		// find out which note lengths are present at that time
		for (set<FloItem, floComp>::iterator it=curr_items.begin(); it!=curr_items.end(); it++)
			if (it->type==FloItem::NOTE)
				lengths[it->len].add(it->pos.height);
		
		cout << "note lengths at that time are:";
		for (stdmap<int, cumulative_t>::iterator it=lengths.begin(); it!=lengths.end(); it++)
			cout << it->first << "("<< it->second.mean() <<")  ";
		cout << endl;
		
		if (lengths.erase(0)) // in case "0" is in the set, erase it		
			has_whole=true;     // but remember there were whole notes
		
		if (lengths.size()==0)
		{
			cout << "no notes other than wholes, or no notes at all. we can relax" << endl;
		}
		else if (lengths.size()==1)
		{
			pair<const int, cumulative_t>& group=*(lengths.begin());
			stem_t stem;
			int shift=0;
			cout << "only one non-whole note group (len="<<group.first<<") at height="<<group.second.mean()<< endl;
			
			if (group.second.mean()>=6)
			{
				stem=DOWNWARDS;
				if (has_whole)
					shift=-1;
			}
			else
			{
				stem=UPWARDS;
				if (has_whole)
					shift=1;
			}
			
			// for each note in that group
			for (set<FloItem, floComp>::iterator it=curr_items.begin(); it!=curr_items.end(); it++)
				if ( (it->type==FloItem::NOTE) && (it->len==group.first) )
				{
					it->stem=stem;
					it->shift=shift;
				}
		}
		else if (lengths.size()==2)
		{
			stdmap<int, cumulative_t>::iterator it=lengths.begin();
			pair<const int, cumulative_t>& group1=*it;
			it++;
			pair<const int, cumulative_t>& group2=*it;
			stem_t stem1, stem2;
			int shift1=0, shift2=0;
			cout << "two non-whole note group: len="<<group1.first<<" at height="<<group1.second.mean()<<"  and len="<<group2.first<<" at height="<<group2.second.mean()<< endl;
			
			if (group1.second.mean()<group2.second.mean())
			{
				stem1=DOWNWARDS;
				stem2=UPWARDS;
				shift1=-1;
				if (has_whole)
					shift2=1;
			}
			else
			{
				stem1=UPWARDS;
				stem2=DOWNWARDS;
				shift2=-1;
				if (has_whole)
					shift1=1;
			}
			
			// for each note in group1
			for (set<FloItem, floComp>::iterator it=curr_items.begin(); it!=curr_items.end(); it++)
				if ( (it->type==FloItem::NOTE) && (it->len==group1.first) )
				{
					it->stem=stem1;
					it->shift=shift1;
				}

			// for each note in group2
			for (set<FloItem, floComp>::iterator it=curr_items.begin(); it!=curr_items.end(); it++)
				if ( (it->type==FloItem::NOTE) && (it->len==group2.first) )
				{
					it->stem=stem2;
					it->shift=shift2;
				}
		}
		else //more than 2 groups
		{
			//at this time, there are no iterators pointing to curr_items.
			//this means, we can erase and insert safely into curr_items here.
			
			//group1 contains the longer notes, group2 the shorter
			
			int group1_n=lengths.size()/2; //round down
			int group2_n=lengths.size()-group1_n;
			
			int group1_len, group2_len;
			int group1_len_ticks, group2_len_ticks;
			

			stdmap<int, cumulative_t>::iterator lit=lengths.begin();
			for (int i=0;i<group1_n-1;i++) lit++; //go to the group1_n-th entry
			group1_len=lit->first;
			for (int i=0;i<group2_n;i++) lit++;  //go to the (group1_n+group2_n)-th entry (i.e., the last before end() )
			group2_len=lit->first;
			
			group1_len_ticks=calc_len(group1_len,0);
			group2_len_ticks=calc_len(group2_len,0);
			
			cout << "we have "<<lengths.size()<<" groups. putting the "<<group1_n<<" longest and the "<<group2_n<<"shortest groups together"<<endl;
			cout << "\tgroup1 will have len="<<group1_len<<" ("<<group1_len_ticks<<" ticks), group2 will have len="<<group2_len<<" ("<<group2_len_ticks<<" ticks)"<<endl;
			
			for (set<FloItem, floComp>::iterator it=curr_items.begin(); it!=curr_items.end();)
				if (it->type==FloItem::NOTE)
				{
					//if *it belongs to group1 and has not already its destination length
					cout << "\tprocessing note-item with len="<<it->len<<endl;
					if (it->len<group1_len)
					{
						cout << "\t\thas to be changed to fit into group 1" << endl;
						FloItem tmp=*it;
						curr_items.erase(it++);

						int len_ticks_remaining=calc_len(tmp.len, tmp.dots)-group1_len_ticks;
						bool tied_note=tmp.tied;
						
						
						//shorten the current item to it's group's length
						tmp.len=group1_len;
						tmp.dots=0;
						tmp.tied=true;
						curr_items.insert(tmp);
						
						//create items for the remaining lengths (and a note_END for the just created shortened note)
						int t=it2->first+group1_len_ticks;

						itemlist[t].insert( FloItem(FloItem::NOTE_END,tmp.pos,0,0) );
						
						list<note_len_t> lens=parse_note_len(len_ticks_remaining,t-last_measure,emphasize_list,true,true);
						unsigned tmppos=t;
						int n_lens=lens.size();
						int count=0;			
						for (list<note_len_t>::iterator x=lens.begin(); x!=lens.end(); x++)
						{
							cout << "\t\twhile regrouping: partial note with len="<<x->len<<", dots="<<x->dots<<endl;
							count++;
							
							bool tie;
							
							if (count<n_lens)
								tie=true;      // all notes except the last are always tied
							else
								tie=tied_note; // only the last respects tied_note
							
							itemlist[tmppos].insert( FloItem(FloItem::NOTE, tmp.pos,x->len,x->dots, tie, tmp.begin_tick, tmp.source_part, tmp.source_event) );
							tmppos+=calc_len(x->len,x->dots);
							itemlist[tmppos].insert( FloItem(FloItem::NOTE_END, tmp.pos,0,0) );
						}

					}
					//else if *it belongs to group2 and has not already its destination length
					else if ((it->len<group2_len) && (it->len>group1_len))
					{
						cout << "\t\thas to be changed to fit into group 2" << endl;
						
						FloItem tmp=*it;
						curr_items.erase(it++);

						int len_ticks_remaining=calc_len(tmp.len, tmp.dots)-group2_len_ticks;
						bool tied_note=tmp.tied;
						
						
						//shorten the current item to it's group's length
						tmp.len=group2_len;
						tmp.dots=0;
						tmp.tied=true;
						curr_items.insert(tmp);
						
						//create items for the remaining lengths (and a note_END for the just created shortened note)
						int t=it2->first+group2_len_ticks;

						itemlist[t].insert( FloItem(FloItem::NOTE_END,tmp.pos,0,0) );
						
						list<note_len_t> lens=parse_note_len(len_ticks_remaining,t-last_measure,emphasize_list,true,true);
						unsigned tmppos=t;
						int n_lens=lens.size();
						int count=0;			
						for (list<note_len_t>::iterator x=lens.begin(); x!=lens.end(); x++)
						{
							cout << "\t\twhile regrouping: partial note with len="<<x->len<<", dots="<<x->dots<<endl;
							count++;
							
							bool tie;
							
							if (count<n_lens)
								tie=true;      // all notes except the last are always tied
							else
								tie=tied_note; // only the last respects tied_note
							
							itemlist[tmppos].insert( FloItem(FloItem::NOTE,tmp.pos,x->len,x->dots, tie, tmp.begin_tick, tmp.source_part, tmp.source_event) );
							tmppos+=calc_len(x->len,x->dots);
							itemlist[tmppos].insert( FloItem(FloItem::NOTE_END,tmp.pos,0,0) );
						}

					}
					else //nothing to do?
					{
						cout << "\t\tnothing to do" << endl;
						it++;
					}
				}
				else
					it++;
			
			goto group_them_again; //do it again
		}

	}
}

//draw a pixmap centered
void ScoreCanvas::draw_pixmap(QPainter& p, int x, int y, const QPixmap& pm)
{
	cout << "drawing pixmap width size="<<pm.width()<<"/"<<pm.height()<<" at "<<x<<"/"<<y<<endl;
	p.drawPixmap(x-pm.width()/2,y-pm.height()/2,pm);
}

QRect bbox_center(int x, int y, const QSize& size)
{
	//why x-foo/2+foo? because due to integer divisions,
	// x-foo/2+foo can be smaller than x+foo/2!
	return QRect(x-size.width()/2,y-size.height()/2,size.width(),size.height());
}

QRect FloItem::bbox() const
{
	return bbox_center(x,y,pix->size());
}

void ScoreCanvas::draw_note_lines(QPainter& p)
{
	int xend=width();
	
	p.setPen(Qt::black);
	
	for (int i=0;i<5;i++)
		p.drawLine(0,YDIST+i*YLEN,xend,YDIST+i*YLEN);
}


void ScoreCanvas::calc_item_pos(ScoreItemList& itemlist)
{
	tonart_t curr_key=C;
	int pos_add=0;
	
	for (ScoreItemList::iterator it2=itemlist.begin(); it2!=itemlist.end(); it2++)
	{
		for (set<FloItem, floComp>::iterator it=it2->second.begin(); it!=it2->second.end();it++)
		{
			//if this changes, also change the line(s) with Y_MARKER
			it->x=it2->first * PIXELS_PER_WHOLE/TICKS_PER_WHOLE  +pos_add;
			it->y=YDIST+4*YLEN  -  (it->pos.height-2)*YLEN/2;
			
			if (it->type==FloItem::NOTE)
			{
				it->x+=NOTE_MOVE_X + it->shift*NOTE_SHIFT;
				
				switch (it->len)
				{
					case 0: it->pix=pix_whole; break;
					case 1: it->pix=pix_half; break;
					default: it->pix=pix_quarter; break;
				}
				
				it->stem_x=it->x;
				
				if (it->ausweich)
				{
					if ((it->stem==UPWARDS) || (it->len==0))
						it->x += it->pix->width()-1; //AUSWEICH_X
					else
						it->x -= it->pix->width()-1; //AUSWEICH_X
				}
				
				//if there's a tie, try to find the tie's destination and set is_tie_dest
				if (it->tied)
				{
					set<FloItem, floComp>::iterator dest;
					set<FloItem, floComp>& desttime = itemlist[it2->first+calc_len(it->len,it->dots)];
					for (dest=desttime.begin(); dest!=desttime.end();dest++)
						if ((dest->type==FloItem::NOTE) && (dest->pos==it->pos))
						{
							dest->is_tie_dest=true;
							dest->tie_from_x=it->x;
							break;
						}
					
					if (dest==desttime.end())
						cout << "THIS SHOULD NEVER HAPPEN: did not find destination note for tie!" << endl;		
				}
			}
			else if (it->type==FloItem::REST)
			{
				switch (it->len)
				{
					case 0: it->pix=&pix_r1; break;
					case 1: it->pix=&pix_r2; break;
					case 2: it->pix=&pix_r4; break;
					case 3: it->pix=&pix_r8; break;
					case 4: it->pix=&pix_r16; break;
				}
				
				it->x+=NOTE_MOVE_X + (it->ausweich ? REST_AUSWEICH_X : 0); //AUSWEICH_X
			}
			else if (it->type==FloItem::BAR)
			{
				//nothing to do :)
			}
			else if (it->type==FloItem::TIME_SIG)
			{
				int add=calc_timesig_width(it->num, it->denom);
				pos_add+=add;
				pos_add_list[it2->first]+=add;
				//+= is used instead of =, because a key- and time-
				//change can occur at the same time.
			}
			else if (it->type==FloItem::KEY_CHANGE)
			{
				tonart_t new_key=it->tonart;
				
				list<int> aufloes_list=calc_accidentials(curr_key, USED_CLEF, new_key);
				list<int> new_acc_list=calc_accidentials(new_key, USED_CLEF);

				int n_acc_drawn=aufloes_list.size() + new_acc_list.size();
				pos_add+=n_acc_drawn*KEYCHANGE_ACC_DIST+ KEYCHANGE_ACC_LEFTDIST+ KEYCHANGE_ACC_RIGHTDIST;
				pos_add_list[it2->first]+=n_acc_drawn*KEYCHANGE_ACC_DIST+ KEYCHANGE_ACC_LEFTDIST+ KEYCHANGE_ACC_RIGHTDIST;
				//+= is used instead of =, because a key- and time-
				//change can occur at the same time.
				
				curr_key=new_key;
			}
		}
	}		
}

void ScoreCanvas::draw_items(QPainter& p, ScoreItemList& itemlist, int x1, int x2)
{
	int from_tick, to_tick;
	ScoreItemList::iterator from_it, to_it;

	//in general: drawing too much isn't bad. drawing too few is.

	from_tick=x_to_tick(x1);
	from_it=itemlist.lower_bound(from_tick);
	//from_it now contains the first time which is fully drawn
	//however, the previous beat could still be relevant, when it's
	//partly drawn. so we decrement from_it
	if (from_it!=itemlist.begin()) from_it--;

	//decrement until we're at a time with a bar
	//otherwise, drawing accidentials will be broken
	while (from_it!=itemlist.begin() && from_it->second.find(FloItem(FloItem::BAR))==from_it->second.end())
		from_it--;
	
	
	to_tick=x_to_tick(x2);
	to_it=itemlist.upper_bound(to_tick);
	//to_it now contains the first time which is not drawn at all any more
	//however, a tie from 1:04 to 2:01 is stored in 2:01, not in 1:04,
	//so for drawing ties, we need to increment to_it, so that the
	//"first time not drawn at all any more" is the last which gets
	//actually drawn.
	if (to_it!=itemlist.end()) to_it++; //do one tick more than neccessary. this will draw ties

	draw_items(p,itemlist,from_it, to_it);	
}

void ScoreCanvas::draw_items(QPainter& p, ScoreItemList& itemlist)
{
	draw_items(p,itemlist,x_pos,x_pos+width());
}

void ScoreCanvas::draw_items(QPainter& p, ScoreItemList& itemlist, ScoreItemList::iterator from_it, ScoreItemList::iterator to_it)
{
	vorzeichen_t curr_accidential[7];
	vorzeichen_t default_accidential[7];
	for (int i=0;i<7;i++) default_accidential[i]=NONE;
	tonart_t curr_key=C;

	for (ScoreItemList::iterator it2=from_it; it2!=to_it; it2++)
	{
		cout << "at t="<<it2->first << endl;
		
		int upstem_y1 = -1, upstem_y2=-1, upstem_x=-1, upflag=-1;
		int downstem_y1 = -1, downstem_y2=-1, downstem_x=-1, downflag=-1;
		
		for (set<FloItem, floComp>::iterator it=it2->second.begin(); it!=it2->second.end();it++)
		{
			if (it->type==FloItem::NOTE)
			{
				cout << "\tNOTE at line"<<it->pos.height<<" with acc.="<<it->pos.vorzeichen<<", len="<<pow(2,it->len);
				for (int i=0;i<it->dots;i++) cout << ".";
				cout << " , stem=";
				if (it->stem==UPWARDS)
					cout << "UPWARDS";
				else
					cout << "DOWNWARDS";
				
				cout << " , shift="<<it->shift<<", ausweich="<<it->ausweich<<", ";
				if (!it->tied)	cout << "un";
				cout << "tied, is_tie_dest="<<it->is_tie_dest<<endl;

				if (it->len!=0) //only for non-whole notes the stems are relevant!
				{
					if (it->stem==UPWARDS)
					{
						if (upstem_y1 == -1)
							upstem_y1=it->y;
						
						upstem_y2=it->y;
						
						
						if ((upflag!=-1) && (upflag!=it->len))
							cout << "WARNING: THIS SHOULD NEVER HAPPEN: upflag != this->flag" << endl;
						upflag=it->len;
						
						if ((upstem_x!=-1) && (upstem_x!=it->stem_x ))
							cout << "WARNING: THIS SHOULD NEVER HAPPEN: upstem_x != x_result" << endl;
						upstem_x=it->stem_x;
					}
					else
					{
						if (downstem_y1 == -1)
							downstem_y1=it->y;
						
						downstem_y2=it->y;
						

						if ((downflag!=-1) && (downflag!=it->len))
							cout << "WARNING: THIS SHOULD NEVER HAPPEN: downflag != this->flag" << endl;
						downflag=it->len;
						
						if ((downstem_x!=-1) && (downstem_x!=it->stem_x))
							cout << "WARNING: THIS SHOULD NEVER HAPPEN: downstem_x != x_result" << endl;
						downstem_x=it->stem_x; //important: before the below calculation!
					}
				}					

		
				if (it->pos.height <= 0) //we need auxiliary lines on the bottom?
				{ //Y_MARKER
					p.setPen(Qt::black);
					for (int i=0; i>=it->pos.height; i-=2)
						p.drawLine(it->x-it->pix->width()*AUX_LINE_LEN/2 -x_pos,YDIST+4*YLEN  -  (i-2)*YLEN/2,it->x+it->pix->width()*AUX_LINE_LEN/2-x_pos,YDIST+4*YLEN  -  (i-2)*YLEN/2);
				}
				else if (it->pos.height >= 12) //we need auxiliary lines on the top?
				{ //Y_MARKER
					p.setPen(Qt::black);
					for (int i=12; i<=it->pos.height; i+=2)
						p.drawLine(it->x-it->pix->width()*AUX_LINE_LEN/2 -x_pos,YDIST+4*YLEN  -  (i-2)*YLEN/2,it->x+it->pix->width()*AUX_LINE_LEN/2-x_pos,YDIST+4*YLEN  -  (i-2)*YLEN/2);
				}
				
								
				draw_pixmap(p,it->x -x_pos,it->y,it->pix[it->source_part->colorIndex()]);
				//TODO FINDMICH draw a margin around bright colors
				//maybe draw the default color in black?
				
				//draw dots
				
				int x_dot=DOT_XBEGIN;
				int y_dot;
				if (modulo(it->pos.height, 2) == 0) //note is on a line?
					y_dot=YLEN * 0.33; // actually 0.5, but that would be _exactly_ in the space
				else //note is between two lines?
					y_dot=YLEN * 0.1;
				
				if (it->stem==DOWNWARDS)
					y_dot=-y_dot;
				//else y_dot=y_dot;
				
				for (int i=0;i<it->dots;i++)
				{
					draw_pixmap(p,it->x+x_dot -x_pos,it->y+y_dot,pix_dot[it->source_part->colorIndex()]);
					x_dot+=DOT_XDIST;
				}


				
				//draw accidentials
				if (it->pos.vorzeichen != curr_accidential[modulo(it->pos.height,7)])
				{
					QPixmap* acc_pix;
					switch (it->pos.vorzeichen)
					{
						case NONE: acc_pix=pix_noacc; break;
						case SHARP: acc_pix=pix_sharp; break;
						case B: acc_pix=pix_b; break;
					}
					draw_pixmap(p,it->x-ACCIDENTIAL_DIST -x_pos,it->y, acc_pix[it->source_part->colorIndex()]);

					curr_accidential[modulo(it->pos.height,7)]=it->pos.vorzeichen;
				}

				
				//if needed, draw tie
				if (it->is_tie_dest)
				{
					cout << "drawing tie" << endl;
					draw_tie(p,it->tie_from_x-x_pos,it->x -x_pos,it->y, (it->len==0) ? true : (it->stem==DOWNWARDS) , config.partColors[it->source_part->colorIndex()]);
					// in english: "if it's a whole note, tie is upwards (true). if not, tie is upwards if
					//              stem is downwards and vice versa"
				}
			}
			else if (it->type==FloItem::REST)
			{
				cout << "\tREST at line"<<it->pos.height<<" with len="<<pow(2,it->len);
				for (int i=0;i<it->dots;i++) cout << ".";
				cout << " , ausweich="<<it->ausweich<<endl;
				
				draw_pixmap(p,it->x -x_pos,it->y,*it->pix);
				

				//draw dots
				
				int x_dot=DOT_XBEGIN_REST;
				int y_dot;
				if (modulo(it->pos.height, 2) == 0) //rest is on a line?
					y_dot=YLEN * 0.33; // actually 0.5, but that would be _exactly_ in the space
				else //note is between two lines?
					y_dot=YLEN * 0.1;
				
				if (it->len!=0) // all rests except the whole are treated as 
					y_dot=-y_dot; // if they had a downwards stem
				
				for (int i=0;i<it->dots;i++)
				{
					draw_pixmap(p,it->x+x_dot -x_pos,it->y+y_dot,pix_dot[BLACK_PIXMAP]);
					x_dot+=DOT_XDIST;
				}
			}
			else if (it->type==FloItem::BAR)
			{
				cout << "\tBAR" << endl;
				
				p.setPen(Qt::black);
				p.drawLine(it->x -x_pos,YDIST,it->x -x_pos,YDIST+4*YLEN);
				
				for (int i=0;i<7;i++)
					curr_accidential[i]=default_accidential[i];
			}
			else if (it->type==FloItem::TIME_SIG)
			{
				cout << "\tTIME SIGNATURE: "<<it->num<<"/"<<it->denom<<endl;

				int y_coord=YDIST+2*YLEN;
				draw_timesig(p,  it->x - x_pos, y_coord, it->num, it->denom);
			}
			else if (it->type==FloItem::KEY_CHANGE)
			{
				tonart_t new_key=it->tonart;
				cout << "\tKEY CHANGE: from "<<curr_key<<" to "<<new_key<<endl;
								
				int n_acc_drawn=0;
				
				list<int> aufloes_list=calc_accidentials(curr_key, USED_CLEF, new_key);
				list<int> new_acc_list=calc_accidentials(new_key, USED_CLEF);
				
				// vorzeichen aus curr_key auflösen
				for (list<int>::iterator acc_it=aufloes_list.begin(); acc_it!=aufloes_list.end(); acc_it++)
				{
					int y_coord=YDIST+4*YLEN  -  ( *acc_it -2)*YLEN/2; //Y_MARKER
					draw_pixmap(p,it->x + n_acc_drawn*KEYCHANGE_ACC_DIST + KEYCHANGE_ACC_LEFTDIST -x_pos,y_coord,pix_noacc[BLACK_PIXMAP]);
					n_acc_drawn++;
				}
				
				QPixmap* pix = is_sharp_key(new_key) ? &pix_sharp[BLACK_PIXMAP] : &pix_b[BLACK_PIXMAP];
				vorzeichen_t new_accidential = is_sharp_key(new_key) ? SHARP : B;
				
				for (int i=0;i<7;i++)
					default_accidential[i]=NONE;
				
				// alle vorzeichen aus new_key zeichnen
				for (list<int>::iterator acc_it=new_acc_list.begin(); acc_it!=new_acc_list.end(); acc_it++)
				{
					int y_coord=YDIST+4*YLEN  -  ( *acc_it -2)*YLEN/2; //Y_MARKER
					draw_pixmap(p,it->x + n_acc_drawn*KEYCHANGE_ACC_DIST + KEYCHANGE_ACC_LEFTDIST -x_pos,y_coord,*pix);
					n_acc_drawn++;
					
					default_accidential[*acc_it % 7]=new_accidential;
				}
				
				curr_key=new_key;
				for (int i=0;i<7;i++)
					curr_accidential[i]=default_accidential[i];
			}
		}
		
		p.setPen(Qt::black);
		//note: y1 is bottom, y2 is top!
		if (upstem_x!=-1)
		{
			upstem_x=upstem_x-pix_quarter[0].width()/2 +pix_quarter[0].width() -1;
			p.drawLine(upstem_x -x_pos, upstem_y1, upstem_x -x_pos, upstem_y2-STEM_LEN);
			
			if (upflag>=3) //if the note needs a flag
				p.drawPixmap(upstem_x -x_pos,upstem_y2-STEM_LEN,pix_flag_up[upflag-3]);
		}
		if (downstem_x!=-1)
		{
			downstem_x=downstem_x-pix_quarter[0].width()/2;
			p.drawLine(downstem_x -x_pos, downstem_y1+STEM_LEN, downstem_x -x_pos, downstem_y2);

			if (downflag>=3) //if the note needs a flag
				p.drawPixmap(downstem_x -x_pos,downstem_y1+STEM_LEN-pix_flag_down[downflag-3].height(),pix_flag_down[downflag-3]);
		}
	}		
}

#define TIMESIG_LEFTMARGIN 5
#define TIMESIG_RIGHTMARGIN 5
#define DIGIT_YDIST 9
#define DIGIT_WIDTH 12

void ScoreCanvas::draw_timesig(QPainter& p, int x, int y, int num, int denom)
{
	int num_width=calc_number_width(num);
	int denom_width=calc_number_width(denom);
	int width=((num_width > denom_width) ? num_width : denom_width);
	int num_indent=(width-num_width)/2 + TIMESIG_LEFTMARGIN;
	int denom_indent=(width-denom_width)/2 + TIMESIG_LEFTMARGIN;
	
	draw_number(p, x+num_indent, y-DIGIT_YDIST, num);
	draw_number(p, x+denom_indent, y+DIGIT_YDIST, denom);
}

int ScoreCanvas::calc_timesig_width(int num, int denom)
{
	int num_width=calc_number_width(num);
	int denom_width=calc_number_width(denom);
	int width=((num_width > denom_width) ? num_width : denom_width);
	return width+TIMESIG_LEFTMARGIN+TIMESIG_RIGHTMARGIN;
}

int ScoreCanvas::calc_number_width(int n)
{
	string str=IntToStr(n);
	return (str.length()*DIGIT_WIDTH);
}

void ScoreCanvas::draw_number(QPainter& p, int x, int y, int n)
{
	string str=IntToStr(n);
	int curr_x=x+DIGIT_WIDTH/2;
	
	for (int i=0;i<str.length(); i++)
	{
		draw_pixmap(p, curr_x, y, pix_num[str[i]-'0']);
		curr_x+=DIGIT_WIDTH;
	}
}


void ScoreCanvas::draw(QPainter& p, const QRect& rect)
{
	cout <<"now in ScoreCanvas::draw"<<endl;

	

	p.setPen(Qt::black);
	
	draw_note_lines(p);
	draw_items(p, itemlist);
}


list<int> ScoreCanvas::calc_accidentials(tonart_t key, clef_t clef, tonart_t next_key)
{
	list<int> result;
	
	int violin_sharp_pos[]={10,7,11,8,5,9,6}; //CLEF_MARKER
	int violin_b_pos[]={6,9,5,8,4,7,3};
	int bass_sharp_pos[]={8,5,9,6,3,7,4};
	int bass_b_pos[]={4,7,3,6,2,5,1};
	
	int* accidential_pos;
	
	switch (clef)
	{
		case VIOLIN: accidential_pos = is_sharp_key(key) ? violin_sharp_pos : violin_b_pos; break;
		case BASS: accidential_pos = is_sharp_key(key) ? bass_sharp_pos : bass_b_pos; break;
	}

	int begin=0;
	
	if (is_sharp_key(key)==is_sharp_key(next_key)) //same kind of key (both b or both #)?
		begin=n_accidentials(next_key);
	else
		begin=0;
	
	
	int end=n_accidentials(key);
	
	for (int i=begin; i<end; i++)
		result.push_back(accidential_pos[i]);
	
	return result;
}




int ScoreCanvas::tick_to_x(int t)
{
	int x=t*PIXELS_PER_WHOLE/TICKS_PER_WHOLE;
	
	for (std::map<int,int>::iterator it=pos_add_list.begin(); it!=pos_add_list.end() && it->first<=t; it++)
		x+=it->second;
	
	return x;
}

int ScoreCanvas::calc_posadd(int t)
{
	int result=0;

	for (std::map<int,int>::iterator it=pos_add_list.begin(); it!=pos_add_list.end() && it->first<t; it++)
		result+=it->second;	

	return result;
}

//doesn't round mathematically correct, but i don't think this
//will be a problem, because a tick is pretty small
int ScoreCanvas::x_to_tick(int x)
{
	int t=TICKS_PER_WHOLE * x/PIXELS_PER_WHOLE;
	int min_t=0;
	
	cout << "t="<<t<<endl;
	
	for (std::map<int,int>::iterator it=pos_add_list.begin(); it!=pos_add_list.end() && it->first<t; it++)
	{
		cout << "at pos_add event at t="<<it->first<<", add="<<it->second<<endl;
		min_t=it->first;
		x-=it->second;
		t=TICKS_PER_WHOLE * x/PIXELS_PER_WHOLE;
	}
	
	return t > min_t ? t : min_t;
}

tonart_t ScoreCanvas::key_at_tick(int t)
{
	tonart_t tmp;
	for (ScoreEventList::iterator it=eventlist.begin(); it!=eventlist.end() && it->first<=t; it++)
		if (it->second.type==FloEvent::KEY_CHANGE)
			tmp=it->second.tonart;
	
	return tmp;
}

int ScoreCanvas::height_to_pitch(int h, clef_t clef)
{
	int foo[]={0,2,4,5,7,9,11};
	
	switch(clef) //CLEF_MARKER
	{
		case VIOLIN:	return foo[modulo(h,7)] + ( (h/7)*12 ) + 60;
		case BASS:		return foo[modulo((h-5),7)] + ( ((h-5)/7)*12 ) + 48;
		default:
			cout << "WARNING: THIS SHOULD NEVER HAPPEN: unknown clef in height_to_pitch" << endl;
			return 60;
	}
}

int ScoreCanvas::height_to_pitch(int h, clef_t clef, tonart_t key)
{
	int add=0;
	
	list<int> accs=calc_accidentials(key,USED_CLEF);
	
	for (list<int>::iterator it=accs.begin(); it!=accs.end(); it++)
	{
		if (modulo(*it,7) == modulo(h,7))
		{
			add=is_sharp_key(key) ? 1 : -1;
			break;
		}
	}
	
	return height_to_pitch(h,clef)+add;
}

int ScoreCanvas::y_to_height(int y)
{
	//Y_MARKER
	return int(rint(float(YDIST+4*YLEN  - y)*2/YLEN))+2 ;
}

int ScoreCanvas::y_to_pitch(int y, int t, clef_t clef)
{
	return height_to_pitch(y_to_height(y), clef, key_at_tick(t));
}


#define DRAG_INIT_DISTANCE 5

void ScoreCanvas::mousePressEvent (QMouseEvent* event)
{
	// den errechneten tick immer ABrunden!
	// denn der "bereich" eines schlags geht von schlag_begin bis nächsterschlag_begin-1
	// noten werden aber genau in die mitte dieses bereiches gezeichnet

	int y=event->y();
	int x=event->x()+x_pos;
	int tick=flo_quantize_floor(x_to_tick(x));
						//TODO quantizing must (maybe?) be done with the proper functions


	cout << "mousePressEvent at "<<x<<"/"<<y<<"; tick="<<tick<<endl;
	set<FloItem, floComp>::iterator it;
	for (it=itemlist[tick].begin(); it!=itemlist[tick].end(); it++)
		if (it->type==FloItem::NOTE)
			if (it->bbox().contains(x,y))
				break;
	
	if (it!=itemlist[tick].end()) //we found something?
	{
		mouse_down_pos=event->pos();
		mouse_operation=NO_OP;
		
		int t=tick;
		set<FloItem, floComp>::iterator found;
		
		do
		{
			found=itemlist[t].find(FloItem(FloItem::NOTE, it->pos));
			if (found == itemlist[t].end())
			{
				cout << "FATAL: THIS SHOULD NEVER HAPPEN: could not find the note's tie-destination" << endl;
				break;
			}
			else
			{
				t+=calc_len(found->len, found->dots);
			}
		} while (found->tied);
		
		int total_begin=it->begin_tick;
		int total_end=t;
		
		int this_begin=tick;
		int this_end=this_begin+calc_len(it->len, it->dots);
		
		//that's the only note corresponding to the event?
		if (this_begin==total_begin && this_end==total_end)
		{
			if (x < it->x)
				mouse_x_drag_operation=BEGIN;
			else
				mouse_x_drag_operation=LENGTH;
		}
		//that's NOT the only note?
		else
		{
			if (this_begin==total_begin)
				mouse_x_drag_operation=BEGIN;
			else if (this_end==total_end)
				mouse_x_drag_operation=LENGTH;
			else
				mouse_x_drag_operation=NO_OP;
		}
		
		cout << "you clicked at a note with begin at "<<it->begin_tick<<" and end at "<<t<<endl;
		cout << "x-drag-operation will be "<<mouse_x_drag_operation<<endl;
		cout << "pointer to part is "<<it->source_part;
		if (!it->source_part) cout << " (WARNING! THIS SHOULD NEVER HAPPEN!)";
		cout << endl;
		
		dragged_event=*it->source_event;
		dragged_event_part=it->source_part;
		dragged_event_original_pitch=dragged_event.pitch();
		
		if ((mouse_erases_notes) || (event->button()==Qt::MidButton)) //erase?
		{
			audio->msgDeleteEvent(dragged_event, dragged_event_part, true, false, false);
		}
		else if (event->button()==Qt::LeftButton) //edit?
		{
			setMouseTracking(true);		
			dragging=true;
			song->startUndo();
		}
	}
	else //we found nothing?
	{
		if ((event->button()==Qt::LeftButton) && (mouse_inserts_notes))
		{
			song->startUndo();
			//stopping undo at the end of this function is unneccessary
			//because we'll begin a drag right after it. finishing
			//this drag will stop undo as well (in mouseReleaseEvent)
			
			Event newevent(Note);
			newevent.setPitch(y_to_pitch(y,tick, USED_CLEF));
			newevent.setVelo(64); //TODO
			newevent.setVeloOff(64); //TODO
			newevent.setTick(tick);
			newevent.setLenTick((new_len>0)?new_len:last_len);
			
			audio->msgAddEvent(newevent, curr_part, false, false, false);
			
			dragged_event_part=curr_part;
			dragged_event=newevent;
			dragged_event_original_pitch=newevent.pitch();

			mouse_down_pos=event->pos();
			mouse_operation=NO_OP;
			mouse_x_drag_operation=LENGTH;

			setMouseTracking(true);	
			dragging=true;
			//song->startUndo(); unneccessary because we have started it already above
		}
	}

}

void ScoreCanvas::mouseReleaseEvent (QMouseEvent* event)
{
	if (event->button()==Qt::LeftButton)
	{
		if (dragging)
		{
			if (mouse_operation==LENGTH)
			{
				if (flo_quantize(dragged_event.lenTick()) <= 0)
				{
					cout << "new length <= 0, erasing item" << endl;
					audio->msgDeleteEvent(dragged_event, dragged_event_part, false, false, false);
				}
				else
				{
					last_len=flo_quantize(dragged_event.lenTick());
				}
			}
			
			song->endUndo(SC_EVENT_MODIFIED);
			setMouseTracking(false);
			dragging=false;
		}
	}
}

#define PITCH_DELTA 5

void ScoreCanvas::mouseMoveEvent (QMouseEvent* event)
{
	if (dragging)
	{
		int dx=event->x()-mouse_down_pos.x();
		int dy=event->y()-mouse_down_pos.y();

		int y=event->y();
		int x=event->x()+x_pos;
		int tick=flo_quantize_floor(x_to_tick(x));

		if (mouse_operation==NO_OP)
		{		
			if ((abs(dx)>DRAG_INIT_DISTANCE) && (mouse_x_drag_operation!=NO_OP))
			{
				cout << "mouse-operation is now "<<mouse_x_drag_operation<<endl;
				mouse_operation=mouse_x_drag_operation;
			}
			else if (abs(dy)>DRAG_INIT_DISTANCE)
			{
				cout << "mouse-operation is now PITCH" << endl;
				mouse_operation=PITCH;
			}
		}

		int new_pitch;
		
		switch (mouse_operation)
		{
			case NONE:
				break;
				
			case PITCH:
				cout << "changing pitch, delta="<<dy/PITCH_DELTA<<endl;
				new_pitch=dragged_event_original_pitch - dy/PITCH_DELTA;

				if (dragged_event.pitch()!=new_pitch)
				{
					Event tmp=dragged_event.clone();
					tmp.setPitch(dragged_event_original_pitch- dy/PITCH_DELTA);
					
					audio->msgChangeEvent(dragged_event, tmp, dragged_event_part, false, false, false);
					dragged_event=tmp;
					
					song_changed(0);	
				}
				
				break;
			
			case BEGIN:
				if (dragged_event.tick() != tick)
				{
					Event tmp=dragged_event.clone();
					tmp.setTick(tick);
					
					audio->msgChangeEvent(dragged_event, tmp, dragged_event_part, false, false, false);
					dragged_event=tmp;
					
					song_changed(0);	
				}
				
				break;

			case LENGTH:
				tick+=FLO_QUANT;
				if (dragged_event.tick()+dragged_event.lenTick() != tick)
				{
					Event tmp=dragged_event.clone();
					tmp.setLenTick(tick-dragged_event.tick());
					
					audio->msgChangeEvent(dragged_event, tmp, dragged_event_part, false, false, false);
					dragged_event=tmp;
					
					song_changed(0);	
				}
				
				break;
		}	
	}
}

void ScoreCanvas::scroll_event(int x)
{
	cout << "SCROLL EVENT: x="<<x<<endl;
	x_pos=x;
	redraw();
}


// TODO: testen: kommen die segfaults von muse oder von mir? [ von mir ]
// TODO: testen, ob das noten-splitten korrekt arbeitet [ scheint zu klappen ]
// TODO: testen, ob shift immer korrekt gesetzt wird [ scheint zu klappen ]

//the following assertions are made:
//  pix_quarter.width() == pix_half.width()


// pix->width()-1 + 1/2*pix->width() + SHIFT + ADD_SPACE
// 10-1+5+3+3=20 <- um so viel wird der taktstrich verschoben
// um das doppelte (20*2=40) werden die kleinsten schläge gegeneinander versetzt



// bei änderung
// takt- oder tonartänderung:
//     alles ab der betreffenden position löschen
//     alles ab dort neu berechnen
// notenänderung:
//     alle von der note betroffenen takte löschen und neuberechnen
//     aus erstem betroffenen takt müssen tie-infos gesichert werden
//     im takt nach dem letzten betroffenen müssen die tie-infos
//     geupdated werden. ggf. löschen ist unnötig, da ties nur wandern,
//     aber nicht verschwinden oder neu dazukommen werden.


//hint: recalculating event- and itemlists "from zero"
//      could happen in realtime, as it is pretty fast.
//      however, this adds unneccessary cpu usage.
//      it is NO problem to recalc the stuff "from zero"
//      every time something changes.


/* BUGS and potential bugs
 *   o when dividing, use a function which always rounds downwards
 *     operator/ rounds towards zero. (-5)/7=0, but should be -1
 * 
 * IMPORTANT TODO
 *   o removing the part the score's working on isn't handled
 *   o let the user select the currently edited part
 *   o let the user select between "colors after the parts",
 *     "colors after selected/unselected part" and "all black"
 *   o draw clef, maybe support clef changes
 *   o support violin and bass at one time
 *   o use correct scrolling bounds
 *   o automatically scroll when playing
 *   o support selections
 *
 * less important stuff
 *   o draw a margin about notes which are in a bright color
 *   o maybe override color 0 with "black"?
 *   o create nice functions for drawing keychange-accidentials
 *   o check if "moving away" works for whole notes [seems to NOT work properly]
 *   o use bars instead of flags over groups of 8ths / 16ths etc
 *   o (change ItemList into map< pos_t , mutable_stuff_t >) [no]
 *   o deal with expanding parts or clip (expanding is better)
 *   o check if making the program clef-aware hasn't broken anything
 *     e.g. accidentials, creating notes, rendering etc.
 *   o replace all kinds of "full-measure-rests" with the whole rest
 *     in the middle of the measure
 *
 * stuff for the other muse developers
 *   o check if dragging notes is done correctly
 *   o after doing the undo stuff right, the "pianoroll isn't informed
 *     about score-editor's changes"-bug has vanished. did it vanish
 *     "by accident", or is that the correct solution for this?
 *   o process key from muse's event list (has to be implemented first in muse)
 *   o process accurate timesignatures from muse's list (has to be implemented first in muse)
 *      ( (2+2+3)/4 or (3+2+2)/4 instead of 7/4 )
 *   o maybe do expanding parts inside the msgChangeEvent or
 *     msgNewEvent functions (see my e-mail)
 *
 * GUI stuff
 *   o offer a button for bool mouse_erases_notes and mouse_inserts_notes
 *   o offer dropdown-boxes for lengths of the inserted note
 *     (select between 16th, 8th, ... whole and "last used length")
 *   o offer a dropdown-box for the clef to use
 *   o offer some way to setup the colorizing method to be used
 */



