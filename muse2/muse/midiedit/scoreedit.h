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
#include <QScrollBar>
#include <QSignalMapper>

#include <values.h>
#include "noteinfo.h"
#include "cobject.h"
#include "midieditor.h"
#include "tools.h"
#include "event.h"
#include "view.h"
#include "gconfig.h"
#include "part.h"
#include "keyevent.h"

#include <set>
#include <map>
#include <list>
#include <vector>
#include <string>

using std::set;
using std::pair;
using std::map;
using std::list;
using std::vector;
using std::string;



#define TICKS_PER_WHOLE (config.division*4) 
#define SONG_LENGTH (song->len())



enum {CMD_COLOR_BLACK, CMD_COLOR_VELO, CMD_COLOR_PART,
      CMD_SET_NAME,
      CMD_NOTELEN_1, CMD_NOTELEN_2, CMD_NOTELEN_4, CMD_NOTELEN_8,
      CMD_NOTELEN_16, CMD_NOTELEN_32, CMD_NOTELEN_LAST };

class ScoreCanvas;

//---------------------------------------------------------
//   ScoreEdit
//---------------------------------------------------------

class ScoreEdit : public TopWin
{
	Q_OBJECT

	private:
		virtual void closeEvent(QCloseEvent*);

		QGridLayout* mainGrid;
		QWidget* mainw;
		
		QScrollBar* xscroll;
		QScrollBar* yscroll;
		ScoreCanvas* score_canvas;
		
		static int serial;
		static set<QString> names;
		
		QString name;
		
		QSignalMapper* menu_mapper;
				
		bool set_name(QString newname, bool emit_signal=true, bool emergency_name=false);
		
	private slots:
		void menu_command(int);
		
	signals:
		void deleted(unsigned long);
		void name_changed();

	public slots:
		void canvas_width_changed(int);
		void viewport_width_changed(int);
		void canvas_height_changed(int);
		void viewport_height_changed(int);
		
	public:
		ScoreEdit(QWidget* parent = 0, const char* name = 0, unsigned initPos = MAXINT);
		~ScoreEdit();
		static void readConfiguration(Xml&){}; //TODO does nothing
		static void writeConfiguration(int, Xml&){}; //TODO does nothing
		
		void add_parts(PartList* pl, bool all_in_one=false);
		QString get_name() { return name; }
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
		
		key_enum key;
		
		
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
		FloEvent(unsigned ti, typeEnum t, key_enum k)
		{
			type=t;
			key=k;
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
		
		key_enum key;
		
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
		
		FloItem(typeEnum t, key_enum k)
		{
			type=t;
			key=k;
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
#define NUM_MYCOLORS (NUM_PARTCOLORS+2 + 128)
#define VELO_PIXMAP_BEGIN (NUM_PARTCOLORS+2)

struct timesig_t
{
	int num;
	int denom;
};

enum staff_type_t
{
	NORMAL,
	GRAND_TOP,
	GRAND_BOTTOM
};

enum staff_mode_t
{
	MODE_TREBLE,
	MODE_BASS,
	MODE_BOTH
};

struct staff_t
{
	set<Part*> parts;
	ScoreEventList eventlist;
	ScoreItemList itemlist;
	
	int y_top;
	int y_draw;
	int y_bottom;
	
	staff_type_t type;
	clef_t clef;
	int split_note;
	
	ScoreCanvas* parent;
	
	void create_appropriate_eventlist();
	void create_itemlist();
	void process_itemlist();
	void calc_item_pos();
	
	void recalculate()
	{
		create_appropriate_eventlist();
		create_itemlist();
		process_itemlist();
		calc_item_pos();
	}
	
	staff_t(ScoreCanvas* parent_)
	{
		type=NORMAL;
		clef=VIOLIN;
		parent=parent_;
	}
	
	staff_t (ScoreCanvas* parent_, staff_type_t type_, clef_t clef_, set<Part*> parts_, int split_note_=0)
	{
		type=type_;
		clef=clef_;
		split_note=split_note_;
		parts=parts_;
		parent=parent_;
	}
	
	bool cleanup_parts();
	
	set<Part*> parts_at_tick(unsigned tick);
};

list<int> calc_accidentials(key_enum key, clef_t clef, key_enum next_key=KEY_C);
note_pos_t note_pos_(int note, key_enum key);
note_pos_t note_pos (unsigned note, key_enum key, clef_t clef);

int calc_len(int l, int d);
list<note_len_t> parse_note_len(int len_ticks, int begin_tick, vector<int>& foo, int quant_power2, bool allow_dots=true, bool allow_normal=true);

int clef_height(clef_t clef);


int calc_timesig_width(int num, int denom);
int calc_number_width(int n);


class ScoreCanvas : public View
{
	Q_OBJECT
	private:
		static void init_pixmaps();
		static void draw_pixmap(QPainter& p, int x, int y, const QPixmap& pm);
		static void draw_tie (QPainter& p, int x1, int x4, int yo, bool up=true, QColor color=Qt::black);

		static void draw_accidentials(QPainter& p, int x, int y_offset, const list<int>& acc_list, const QPixmap& pix);

		static void draw_timesig(QPainter& p, int x, int y_offset, int num, int denom);

		static void draw_number(QPainter& p, int x, int y, int n);





		static int height_to_pitch(int h, clef_t clef, key_enum key);
		static int height_to_pitch(int h, clef_t clef);
		static int y_to_height(int y);
		int y_to_pitch(int y, int t, clef_t clef);




		void draw_note_lines(QPainter& p, int y);
		void draw_preamble(QPainter& p, int y, clef_t clef);
		void draw_items(QPainter& p, int y, staff_t& staff, ScoreItemList::iterator from_it, ScoreItemList::iterator to_it);
		void draw_items(QPainter& p, int y, staff_t& staff, int x1, int x2);
		void draw_items(QPainter& p, int y, staff_t& staff);
		void calc_pos_add_list();
		
		
		void recalc_staff_pos();
		list<staff_t>::iterator staff_at_y(int y);

		
		timesig_t timesig_at_tick(int t);
		key_enum key_at_tick(int t);
		int tick_to_x(int t);
		int x_to_tick(int x);
		int calc_posadd(int t);



		bool need_redraw_for_hilighting(ScoreItemList::iterator from_it, ScoreItemList::iterator to_it);
		bool need_redraw_for_hilighting(ScoreItemList& itemlist, int x1, int x2);
		bool need_redraw_for_hilighting(ScoreItemList& itemlist);
		bool need_redraw_for_hilighting();


		void set_staffmode(list<staff_t>::iterator it, staff_mode_t mode);
		void remove_staff(list<staff_t>::iterator it);
		void merge_staves(list<staff_t>::iterator dest, list<staff_t>::iterator src);
		void cleanup_staves();
		void maybe_close_if_empty();
		
// member variables ---------------------------------------------------
		int _quant_power2;
		int _pixels_per_whole;

		int newnote_velo;
		int newnote_velo_off;
		
		std::map<int,int> pos_add_list;
		
		list<staff_t> staves;
		
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
		
		int y_pos;

		//for mouse-scrolling
		float x_scroll_speed;
		float x_scroll_pos;
		float y_scroll_speed;
		float y_scroll_pos;

		Part* selected_part;
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



		enum {COLOR_MODE_BLACK, COLOR_MODE_PART, COLOR_MODE_VELO} coloring_mode;
		bool preamble_contains_keysig;
		bool preamble_contains_timesig;


		//menu stuff
		QAction* staffmode_treble_action;
		QAction* staffmode_bass_action;
		QAction* staffmode_both_action;
		QAction* remove_staff_action;
		
		QMenu* staff_menu;
		list<staff_t>::iterator current_staff;
		bool dragging_staff;


	private slots:
		void staffmode_treble_slot();
		void staffmode_bass_slot();
		void staffmode_both_slot();
		void remove_staff_slot();
		
		void play_changed(bool);
		void config_changed();

   public slots:
      void x_scroll_event(int);
      void y_scroll_event(int);
      void song_changed(int);
			void goto_tick(int,bool);
			void pos_changed(int i, unsigned u, bool b);
			void heartbeat_timer_event();
			
			void set_tool(int);
			void set_quant(int);
			void menu_command(int);
			void preamble_keysig_slot(bool);
			void preamble_timesig_slot(bool);
			void set_pixels_per_whole(int);

			void set_newnote_velo(int);
			void set_newnote_velo_off(int);
	
	signals:
			void xscroll_changed(int);
			void yscroll_changed(int);
			void viewport_width_changed(int);
			void canvas_width_changed(int);
			void viewport_height_changed(int);
			void canvas_height_changed(int);
			void pixels_per_whole_changed(int);
			
	protected:
		virtual void draw(QPainter& p, const QRect& rect);
		ScoreEdit* parent;
		
		virtual void mousePressEvent (QMouseEvent* event);
		virtual void mouseMoveEvent (QMouseEvent* event);
		virtual void mouseReleaseEvent (QMouseEvent* event);
		virtual void resizeEvent(QResizeEvent*);
		
	public:
		ScoreCanvas(ScoreEdit*, QWidget*, int, int);
		~ScoreCanvas(){};

		void add_staves(PartList* pl, bool all_in_one);

		int canvas_width();
		int canvas_height();
		int viewport_width();
		int viewport_height();
		
		int quant_power2() { return _quant_power2; }
		int quant_len() { return (1<<_quant_power2); }
		int quant_ticks() { return TICKS_PER_WHOLE / (1<<_quant_power2); }
		int pixels_per_whole() { return _pixels_per_whole; }
		int note_x_indent() { return pixels_per_whole()/quant_len()/2; }
};

int calc_measure_len(const list<int>& nums, int denom);
vector<int> create_emphasize_list(const list<int>& nums, int denom);
vector<int> create_emphasize_list(int num, int denom);


#endif

