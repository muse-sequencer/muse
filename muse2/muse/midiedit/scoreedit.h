//=========================================================
//  MusE
//  Linux Music Editor
//  scoreedit.h
//  (C) Copyright 2011 Florian Jung (florian.a.jung@web.de)
//=========================================================

#ifndef __SCOREEDIT_H__
#define __SCOREEDIT_H__

#include <QCloseEvent>
#include <QResizeEvent>
#include <QLabel>
#include <QKeyEvent>
#include <QPainter>
#include <QPixmap>
#include <QTimer>

#include <values.h>
#include "noteinfo.h"
#include "cobject.h"
#include "midieditor.h"
#include "tools.h"
#include "event.h"
#include "view.h"
#include "gconfig.h"

#include <set>
#include <map>
#include <list>
#include <vector>

using std::set;
using std::pair;
using std::map;
using std::list;
using std::vector;

class MidiPart;
class TimeLabel;
class PitchLabel;
class QLabel;
class PianoCanvas;
class MTScale;
class Track;
class QToolButton;
class QToolBar;
class QPushButton;
class CtrlEdit;
class Splitter;
class PartList;
class Toolbar1;
class Xml;
class QuantConfig;
class ScrollScale;
class Part;
class SNode;
class QMenu;
class QAction;
class QWidget;
class QScrollBar;
class MidiTrackInfo;
class QScrollArea;




//---------------------------------------------------------
//   ScoreEdit
//---------------------------------------------------------

class ScoreEdit : public MidiEditor
{
	Q_OBJECT

	private:
		virtual void closeEvent(QCloseEvent*);
		
		QScrollBar* hscroll;
		
	private slots:
		

	signals:
		void deleted(unsigned long);

	public slots:
		void canvas_width_changed(int);
		void viewport_width_changed(int);
		
	public:
		ScoreEdit(PartList*, QWidget* parent = 0, const char* name = 0, unsigned initPos = MAXINT);
		~ScoreEdit();
		static void readConfiguration(Xml&){}; //TODO does nothing
		static void writeConfiguration(int, Xml&){}; //TODO does nothing
	};





enum tonart_t
{
	SHARP_BEGIN,
	C,   // C or am, uses # for "black keys"
	G,
	D,
	A,
	E,
	H,
	FIS, //produces a #E (sounds like a F)
	SHARP_END,
	B_BEGIN,
	C_B,  // the same as C, but uses b for "black keys"
	F,
	ES,
	AS,
	DES,
	GES, //sounds like FIS, but uses b instead of #
	B_END
};

enum stem_t
{
	UPWARDS,
	DOWNWARDS
};

enum vorzeichen_t
{
	B=-1,
	NONE=0,
	SHARP=1
};

struct note_pos_t
{
	int height; // 0 means "C-line", 1 "D-line" and so on
	vorzeichen_t vorzeichen;
	
	bool operator== (const note_pos_t& that) const
	{
		return (this->height==that.height) && (this->vorzeichen == that.vorzeichen);
	}
};

bool operator< (const note_pos_t& a, const note_pos_t& b);


class FloEvent
{
	public:
		enum typeEnum { NOTE_ON = 30, NOTE_OFF = 10, BAR = 20, KEY_CHANGE=23, TIME_SIG=26 }; //the order matters!
		typeEnum type;
		unsigned tick;
		Part* source_part;
		Event* source_event;
		
		int pitch;
		mutable int vel;
		mutable int len;
		
		int num;
		int denom;
		
		tonart_t tonart;
		
		
		FloEvent(unsigned ti, int p,int v,int l,typeEnum t, Part* part=NULL, Event* event=NULL)
		{
			pitch=p;
			vel=v;
			len=l;
			type= t;
			tick=ti;
			source_event=event;
			source_part=part;
		}
		FloEvent(unsigned ti, typeEnum t, int num_, int denom_)
		{
			type=t;
			num=num_;
			denom=denom_;
			tick=ti;
			source_event=NULL;
			source_part=NULL;
		}
		FloEvent(unsigned ti, typeEnum t, tonart_t k)
		{
			type=t;
			tonart=k;
			tick=ti;
			source_event=NULL;
			source_part=NULL;
		}
};
class FloItem
{
	public:
		enum typeEnum { NOTE=21, REST=22, NOTE_END=01, REST_END=02, BAR =10, KEY_CHANGE=13, TIME_SIG=16}; //the order matters!
		typeEnum type;
		unsigned begin_tick;
		Event* source_event;
		Part* source_part;
		
		note_pos_t pos;
		int len;
		int dots;
		bool tied;
		bool already_grouped;
		
		int num;
		int denom;
		
		tonart_t tonart;
		
		mutable stem_t stem;
		mutable int shift;
		mutable bool ausweich;
		mutable bool is_tie_dest;
		mutable int tie_from_x;
		
		mutable int x;
		mutable int y;
		mutable int stem_x;
		mutable QPixmap* pix;
		
		mutable bool is_active;
		
		QRect bbox() const;
		

		
		FloItem(typeEnum t, note_pos_t p, int l=0,int d=0, bool ti=false, unsigned beg=0, Part* part=NULL, Event* event=NULL)
		{
			pos=p;
			dots=d;
			len=l;
			type=t;
			already_grouped=false;
			tied=ti;
			shift=0;
			ausweich=false;
			is_tie_dest=false;
			begin_tick=beg;
			source_event=event;
			source_part=part;
		}
		
		FloItem(typeEnum t, int num_, int denom_)
		{
			type=t;
			num=num_;
			denom=denom_;
			begin_tick=-1;
			source_event=NULL;
			source_part=NULL;
		}
		
		FloItem(typeEnum t, tonart_t k)
		{
			type=t;
			tonart=k;
			begin_tick=-1;
			source_event=NULL;
			source_part=NULL;
		}
		
		FloItem(typeEnum t)
		{
			type=t;

			already_grouped=false;
			tied=false;
			shift=0;
			ausweich=false;
			is_tie_dest=false;
			begin_tick=-1;
			source_event=NULL;
			source_part=NULL;
		}
		
		FloItem()
		{
			already_grouped=false;
			tied=false;
			shift=0;
			ausweich=false;
			is_tie_dest=false;
			begin_tick=-1;
			source_event=NULL;
			source_part=NULL;
		}
		
		bool operator==(const FloItem& that)
		{
			if (this->type != that.type) return false;
			
			switch(type)
			{
				case NOTE:
				case REST:
				case NOTE_END:
				case REST_END:
					return (this->pos == that.pos);
				
				//the following may only occurr once in a set
				//so we don't search for "the time signature with 4/4
				//at t=0", but only for "some time signature at t=0"
				//that's why true is returned, and not some conditional
				//expression
				case BAR:
				case KEY_CHANGE:
				case TIME_SIG:
					return true;
			}
		}
};
struct floComp
{
	bool operator() (const pair<unsigned, FloEvent>& a, const pair<unsigned, FloEvent>& b )
	{
		if (a.first < b.first) return true;
		if (a.first > b.first) return false;

		if (a.second.type<b.second.type) return true;
		if (a.second.type>b.second.type) return false;
				
		return (a.second.pitch<b.second.pitch);
	}
	bool operator() (const FloItem& a, const FloItem& b )
	{
		if (a.type < b.type) return true;
		if (a.type > b.type) return false;

		switch(a.type)
		{
			case FloItem::NOTE:
			case FloItem::REST:
			case FloItem::NOTE_END:
			case FloItem::REST_END:
				return (a.pos < b.pos);
			
			//the following may only occurr once in a set
			//so we don't search for "the time signature with 4/4
			//at t=0", but only for "some time signature at t=0"
			//that's why true is returned, and not some conditional
			//expression
			case FloItem::BAR:
			case FloItem::KEY_CHANGE:
			case FloItem::TIME_SIG:
				return false;
		}
		return (a.pos < b.pos);
	}
};

typedef set< pair<unsigned, FloEvent>, floComp > ScoreEventList;
typedef map< unsigned, set<FloItem, floComp> > ScoreItemList;

enum clef_t
{
	VIOLIN,
	BASS
};


struct note_len_t
{
	int len;
	int dots;
	
	note_len_t(int l, int d)
	{
		len=l; dots=d;
	}
	
	note_len_t(int l)
	{
		len=l; dots=0;
	}
};

bool operator< (const note_len_t& a,const note_len_t& b);

struct cumulative_t
{
	int count;
	int cumul;
	
	cumulative_t()
	{
		count=0;
		cumul=0;
	}
	
	void add(int v)
	{
		count++;
		cumul+=v;
	}
	
	float mean()
	{
		return (float)cumul/count;
	}
};

#define BLACK_PIXMAP (NUM_PARTCOLORS)
#define HIGHLIGHTED_PIXMAP (NUM_PARTCOLORS+1)

struct timesig_t
{
	int num;
	int denom;
};

class ScoreCanvas : public View
{
	Q_OBJECT
	private:
		void load_pixmaps();
		ScoreEventList create_appropriate_eventlist(PartList* pl);
		note_pos_t note_pos_(int note, tonart_t key);
		note_pos_t note_pos (unsigned note, tonart_t key, clef_t clef);
		int calc_len(int l, int d);
		list<note_len_t> parse_note_len(int len_ticks, int begin_tick, vector<int>& foo, bool allow_dots=true, bool allow_normal=true);
		void draw_tie (QPainter& p, int x1, int x4, int yo, bool up=true, QColor color=Qt::black);
		ScoreItemList create_itemlist(ScoreEventList& eventlist);
		void process_itemlist(ScoreItemList& itemlist);
		void draw_pixmap(QPainter& p, int x, int y, const QPixmap& pm);
		void draw_note_lines(QPainter& p);
		void draw_preamble(QPainter& p);
		void draw_items(QPainter& p, ScoreItemList& itemlist, ScoreItemList::iterator from_it, ScoreItemList::iterator to_it);
		void draw_items(QPainter& p, ScoreItemList& itemlist, int x1, int x2);
		void draw_items(QPainter& p, ScoreItemList& itemlist);
		void calc_item_pos(ScoreItemList& itemlist);
		list<int> calc_accidentials(tonart_t key, clef_t clef, tonart_t next_key=C);
		void draw_accidentials(QPainter& p, int x, const list<int>& acc_list, const QPixmap& pix);
		
		void draw_timesig(QPainter& p, int x, int num, int denom);
		int calc_timesig_width(int num, int denom);
		void draw_number(QPainter& p, int x, int y, int n);
		int calc_number_width(int n);

		int clef_height(clef_t clef);

		int y_to_pitch(int y, int t, clef_t clef);
		int y_to_height(int y);
		int height_to_pitch(int h, clef_t clef, tonart_t key);
		int height_to_pitch(int h, clef_t clef);
		
		timesig_t timesig_at_tick(int t);
		tonart_t key_at_tick(int t);
		int tick_to_x(int t);
		int x_to_tick(int x);
		int calc_posadd(int t);



		bool need_redraw_for_hilighting(ScoreItemList::iterator from_it, ScoreItemList::iterator to_it);
		bool need_redraw_for_hilighting(int x1, int x2);
		bool need_redraw_for_hilighting();

		int canvas_width();
		int viewport_width();
		
		QPixmap pix_whole[NUM_PARTCOLORS+2], pix_half[NUM_PARTCOLORS+2], pix_quarter[NUM_PARTCOLORS+2];
		QPixmap pix_r1, pix_r2, pix_r4, pix_r8, pix_r16;
		QPixmap pix_dot[NUM_PARTCOLORS+2], pix_flag_up[4], pix_flag_down[4];
		QPixmap pix_b[NUM_PARTCOLORS+2], pix_sharp[NUM_PARTCOLORS+2], pix_noacc[NUM_PARTCOLORS+2];
		QPixmap pix_num[10];
		QPixmap pix_clef_violin, pix_clef_bass;
		
		std::map<int,int> pos_add_list;
		ScoreEventList eventlist;
		ScoreItemList itemlist;
		
		// the drawing area is split into a "preamble" containing clef,
		// key and time signature, and the "item's area" containing the
		// actual items (notes, bars, rests, etc.)
		// x_pos is responsible for scrolling. an item with item->x==x_pos
		// will be drawn exactly at the left beginning of the item's area
		// x_left could also be called "preamble's width". it defines
		// where the item's area begins
		// when multiple note systems are drawn into one window, the
		// preamble's length is the same for each system
		int x_pos;
		int x_left;

		//for mouse-scrolling
		float scroll_speed;
		float scroll_pos;

		Part* curr_part;
		int last_len;
		int new_len; //when zero or negative, last_len is used

		QPoint mouse_down_pos;
		bool mouse_down;
		enum operation_t
		{
			NO_OP=0,
			BEGIN=1,
			LENGTH=2,
			PITCH=3
		};
		operation_t mouse_operation;
		operation_t mouse_x_drag_operation;
		bool mouse_erases_notes;
		bool mouse_inserts_notes;
		
		bool dragging;
		Part* dragged_event_part;
		Event dragged_event;
		int dragged_event_original_pitch;


   public slots:
      void scroll_event(int);
      void song_changed(int);
			void goto_tick(int,bool);
			void pos_changed(int i, unsigned u, bool b);
			void heartbeat_timer_event();
	
	signals:
			void xpos_changed(int);
			void viewport_width_changed(int);
			void canvas_width_changed(int);
			
	protected:
		virtual void draw(QPainter& p, const QRect& rect);
		MidiEditor* editor;
		
		virtual void mousePressEvent (QMouseEvent* event);
		virtual void mouseMoveEvent (QMouseEvent* event);
		virtual void mouseReleaseEvent (QMouseEvent* event);
		virtual void resizeEvent(QResizeEvent*);
		
	public:
		ScoreCanvas(MidiEditor*, QWidget*, int, int);
		~ScoreCanvas(){};

};

int calc_measure_len(const list<int>& nums, int denom);
vector<int> create_emphasize_list(const list<int>& nums, int denom);
vector<int> create_emphasize_list(int num, int denom);


#endif

