//=========================================================
//  MusE
//  Linux Music Editor
//  scoreedit.h
//  (C) Copyright 2011 Florian Jung (flo93@users.sourceforge.net)
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
#include <QComboBox>
#include <QSignalMapper>
#include <QAction>
#include <QActionGroup>
#include <QGridLayout>
#include <QByteArray>
#include <QToolButton>

#include <limits.h>
#include "noteinfo.h"
#include "cobject.h"
#include "event.h"
#include "view.h"
#include "gconfig.h"
#include "part.h"
#include "keyevent.h"
#include "mtscale_flo.h"
#include "steprec.h"
#include "cleftypes.h"
#include "helper.h"
#include "spinbox.h"

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



#define TICKS_PER_WHOLE (MusEGlobal::config.division*4) 
#define SONG_LENGTH (MusEGlobal::song->len())



enum {CMD_COLOR_BLACK, CMD_COLOR_VELO, CMD_COLOR_PART,
      CMD_SET_NAME,
      CMD_NOTELEN_1, CMD_NOTELEN_2, CMD_NOTELEN_4, CMD_NOTELEN_8,
      CMD_NOTELEN_16, CMD_NOTELEN_32, CMD_NOTELEN_LAST,
      
      CMD_QUANTIZE, CMD_VELOCITY, CMD_CRESCENDO, CMD_NOTELEN, CMD_TRANSPOSE,
      CMD_ERASE, CMD_MOVE, CMD_FIXED_LEN, CMD_DELETE_OVERLAPS, CMD_LEGATO,
      CMD_CUT, CMD_COPY, CMD_COPY_RANGE, CMD_PASTE, CMD_PASTE_DIALOG, CMD_DEL,
      CMD_SELECT_ALL, CMD_SELECT_NONE, CMD_SELECT_INVERT,
      CMD_SELECT_ILOOP, CMD_SELECT_OLOOP};



namespace MusEGui {
class EditToolBar;
class ScoreCanvas;


//---------------------------------------------------------
//   ScoreEdit
//---------------------------------------------------------

class ScoreEdit : public TopWin
{
	Q_OBJECT
	private:
		virtual void closeEvent(QCloseEvent*);
		
		void init_name();

		QGridLayout* mainGrid;
		QWidget* mainw;
		
		MusEGui::EditToolBar* edit_tools;
		SpinBox* velo_spinbox;
		SpinBox* velo_off_spinbox;
		
		QComboBox* quant_combobox;
		SpinBox* px_per_whole_spinbox;
		
		QAction* preamble_keysig_action;
		QAction* preamble_timesig_action;
		
		QActionGroup* len_actions;
		QAction* n1_action;
		QAction* n2_action;
		QAction* n4_action;
		QAction* n8_action;
		QAction* n16_action;
		QAction* n32_action;
		QAction* nlast_action;
		
		QActionGroup* color_actions;
		QAction* color_black_action;
		QAction* color_velo_action;
		QAction* color_part_action;

		QMenu* color_menu;
		
		QAction* cut_action;
		QAction* copy_action;
		QAction* copy_range_action;
		QAction* paste_action;
		QAction* paste_dialog_action;
		QAction* del_action;
		
		QAction* select_all_action;
		QAction* select_none_action;
		QAction* select_invert_action;
		QAction* select_iloop_action;
		QAction* select_oloop_action;

		QAction* func_quantize_action;
		QAction* func_notelen_action;
		QAction* func_velocity_action;
		QAction* func_cresc_action;
		QAction* func_transpose_action;
		QAction* func_erase_action;
		QAction* func_move_action;
		QAction* func_fixed_len_action;
		QAction* func_del_overlaps_action;
		QAction* func_legato_action;

		QToolButton* srec;
		
		QScrollBar* xscroll;
		QScrollBar* yscroll;
		ScoreCanvas* score_canvas;
		MusEGui::MTScaleFlo* time_bar;
		
		QLabel* apply_velo_to_label;
		bool apply_velo;
		
		static set<QString> names;
		
		QString name;
		
		QSignalMapper* menu_mapper;
				
		bool set_name(QString newname, bool emit_signal=true, bool emergency_name=false);

		virtual void keyPressEvent(QKeyEvent*);
		
	private slots:
		void menu_command(int);
		void velo_box_changed();
		void velo_off_box_changed();
		void quant_combobox_changed(int);
		void init_shortcuts();
		void selection_changed();
		void clipboard_changed();
		
	signals:
		void isDeleting(MusEGui::TopWin*);
		void name_changed();
		void velo_changed(int);
		void velo_off_changed(int);

	public slots:
		void canvas_width_changed(int);
		void viewport_width_changed(int);
		void canvas_height_changed(int);
		void viewport_height_changed(int);
		void song_changed(int);
		void focusCanvas();
		
	public:
		ScoreEdit(QWidget* parent = 0, const char* name = 0, unsigned initPos = INT_MAX);
		~ScoreEdit();

		void writeStatus(int level, MusECore::Xml& xml) const;
		void readStatus(MusECore::Xml& xml);
		static void read_configuration(MusECore::Xml&);
		static void write_configuration(int, MusECore::Xml&);
		
		void add_parts(MusECore::PartList* pl, bool all_in_one=false);
		QString get_name() { return name; }
		bool get_apply_velo() { return apply_velo; }
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
		MusECore::Part* source_part;
		MusECore::Event* source_event;
		
		int pitch;
		mutable int vel;
		mutable int len;
		
		int num;
		int denom;
		
		MusECore::key_enum key;
		
		
		FloEvent(unsigned ti, int p,int v,int l,typeEnum t, MusECore::Part* part=NULL, MusECore::Event* event=NULL)
		{
			pitch=p;
			vel=v;
			len=l;
			type= t;
			tick=ti;
			source_event=event;
			source_part=part;
			
			num=denom=0xdeadbeef; //unused, but valgrind complains if uninited
			key=MusECore::KEY_C;
		}
		FloEvent(unsigned ti, typeEnum t, int num_, int denom_)
		{
			type=t;
			num=num_;
			denom=denom_;
			tick=ti;
			source_event=NULL;
			source_part=NULL;
			
			len=vel=pitch=0xdeadbeef; //unused, but valgrind complains if uninited
			key=MusECore::KEY_C;
		}
		FloEvent(unsigned ti, typeEnum t, MusECore::key_enum k)
		{
			type=t;
			key=k;
			tick=ti;
			source_event=NULL;
			source_part=NULL;
			
			pitch=vel=len=num=denom=0xdeadbeef; //unused, but valgrind complains if uninited
		}
};
class FloItem
{
	public:
		enum typeEnum { NOTE=21, REST=22, NOTE_END=01, REST_END=02, BAR =10, KEY_CHANGE=13, TIME_SIG=16}; //the order matters!
		typeEnum type;
		unsigned begin_tick;
		MusECore::Event* source_event;
		MusECore::Part* source_part;
		
		note_pos_t pos;
		int len;
		int dots;
		bool tied;
		bool already_grouped;
		
		int num;
		int denom;
		
		MusECore::key_enum key;
		
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
		

		
		FloItem(typeEnum t, note_pos_t p, int l=0,int d=0, bool ti=false, unsigned beg=0, MusECore::Part* part=NULL, MusECore::Event* event=NULL)
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
			is_active=false;
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
		
		FloItem(typeEnum t, MusECore::key_enum k)
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
#define SELECTED_PIXMAP (NUM_PARTCOLORS+2)
#define NUM_MYCOLORS (NUM_PARTCOLORS+3 + 128)
#define VELO_PIXMAP_BEGIN (NUM_PARTCOLORS+3)

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
	set<MusECore::Part*> parts;
	set<int> part_indices;
	ScoreEventList eventlist;
	ScoreItemList itemlist;
	
	int y_top;
	int y_draw;
	int y_bottom;
	
	int min_y_coord;
	int max_y_coord;
	
	staff_type_t type;
	clef_t clef;
	
	ScoreCanvas* parent;
	
	void create_appropriate_eventlist();
	void create_itemlist();
	void process_itemlist();
	void calc_item_pos();
	
	void apply_lasso(QRect rect, set<MusECore::Event*>& already_processed);
	
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
	
	staff_t (ScoreCanvas* parent_, staff_type_t type_, clef_t clef_, set<MusECore::Part*> parts_)
	{
		type=type_;
		clef=clef_;
		parts=parts_;
		parent=parent_;
		update_part_indices();
	}
	
	bool cleanup_parts();
	
	set<MusECore::Part*> parts_at_tick(unsigned tick);
	
	void read_status(MusECore::Xml& xml);
	void write_status(int level, MusECore::Xml& xml) const;
	
	void update_parts(); //re-populates the set<MusECore::Part*> from the set<int>
	void update_part_indices(); //re-populates the set<int> from the set<MusECore::Part*>
};

list<int> calc_accidentials(MusECore::key_enum key, clef_t clef, MusECore::key_enum next_key=MusECore::KEY_C);
note_pos_t note_pos_(int note, MusECore::key_enum key);
note_pos_t note_pos (unsigned note, MusECore::key_enum key, clef_t clef);

int calc_len(int l, int d);
list<note_len_t> parse_note_len(int len_ticks, int begin_tick, vector<int>& foo, bool allow_dots=true, bool allow_normal=true);

int clef_height(clef_t clef);


int calc_timesig_width(int num, int denom);
int calc_number_width(int n);


class ScoreCanvas : public MusEGui::View
{
	Q_OBJECT
	private:
		static void init_pixmaps();
		static void draw_pixmap(QPainter& p, int x, int y, const QPixmap& pm);
		static void draw_tie (QPainter& p, int x1, int x4, int yo, bool up=true, QColor color=Qt::black);
		static void draw_akkolade (QPainter& p, int x, int y);

		static void draw_accidentials(QPainter& p, int x, int y_offset, const list<int>& acc_list, const QPixmap& pix);

		static void draw_timesig(QPainter& p, int x, int y_offset, int num, int denom);

		static void draw_number(QPainter& p, int x, int y, int n);





		static int height_to_pitch(int h, clef_t clef, MusECore::key_enum key);
		static int height_to_pitch(int h, clef_t clef);
		static int y_to_height(int y);
		int y_to_pitch(int y, int t, clef_t clef);




		void draw_note_lines(QPainter& p, int y, bool reserve_akkolade_space=false);
		void draw_preamble(QPainter& p, int y, clef_t clef, bool reserve_akkolade_space=false, bool with_akkolade=false);
		void draw_items(QPainter& p, int y, staff_t& staff, ScoreItemList::iterator from_it, ScoreItemList::iterator to_it);
		void draw_items(QPainter& p, int y, staff_t& staff, int x1, int x2);
		void draw_items(QPainter& p, int y, staff_t& staff);
		void calc_pos_add_list();
		
		
		void recalc_staff_pos();
		list<staff_t>::iterator staff_at_y(int y);



		bool need_redraw_for_hilighting(ScoreItemList::iterator from_it, ScoreItemList::iterator to_it);
		bool need_redraw_for_hilighting(ScoreItemList& itemlist, int x1, int x2);
		bool need_redraw_for_hilighting(ScoreItemList& itemlist);
		bool need_redraw_for_hilighting();


		void set_staffmode(list<staff_t>::iterator it, staff_mode_t mode);
		void remove_staff(list<staff_t>::iterator it);
		void merge_staves(list<staff_t>::iterator dest, list<staff_t>::iterator src);
		void move_staff_above(list<staff_t>::iterator dest, list<staff_t>::iterator src);
		void move_staff_below(list<staff_t>::iterator dest, list<staff_t>::iterator src);
		void cleanup_staves();
		void maybe_close_if_empty();

// defaults  ----------------------------------------------------------
	public:
		enum coloring_mode_t {COLOR_MODE_BLACK, COLOR_MODE_PART, COLOR_MODE_VELO};
		static int _quant_power2_init;
		static int _pixels_per_whole_init;
		static int note_velo_init, note_velo_off_init;
		static int new_len_init;
		static coloring_mode_t coloring_mode_init;
		static bool preamble_contains_timesig_init;
		static bool preamble_contains_keysig_init;

// member variables ---------------------------------------------------
	private:
		int _quant_power2;
		int _pixels_per_whole;

		int note_velo;
		int note_velo_off;
		
		std::map<int,int> pos_add_list;
		
		list<staff_t> staves;
		
		MusECore::StepRec* steprec;
		
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

		MusECore::Part* selected_part;
		int selected_part_index;
		
		int last_len;
		int new_len; //when zero or negative, last_len is used

		Qt::KeyboardModifiers keystate;
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
		
		bool inserting;
		bool dragging;
		bool drag_cursor_changed;
		MusECore::Part* dragged_event_part;
		int dragged_event_part_index;
		MusECore::Event dragged_event;
		MusECore::Event original_dragged_event;
		MusECore::Event* clicked_event_ptr;
		
		int old_pitch;
		unsigned old_dest_tick;
		int old_len;
		
		bool have_lasso;
		QPoint lasso_start;
		QRect lasso;

		bool undo_started;
		bool temp_undo;
		
		bool srec;
		bool held_notes[128];

		coloring_mode_t coloring_mode;
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
		
		void deselect_all();
		void midi_note(int pitch, int velo);
		
		void add_new_parts(const std::map< MusECore::Part*, std::set<MusECore::Part*> >&);

	public slots:
		void x_scroll_event(int);
		void y_scroll_event(int);
		void song_changed(int);
		void fully_recalculate();
		void goto_tick(int,bool);
		void pos_changed(int i, unsigned u, bool b);
		void heartbeat_timer_event();

		void set_tool(int);
		void set_quant(int);
		void menu_command(int);
		void preamble_keysig_slot(bool);
		void preamble_timesig_slot(bool);
		void set_pixels_per_whole(int);

		void set_velo(int);
		void set_velo_off(int);

		void set_steprec(bool);

		void update_parts(); //re-populates the set<MusECore::Part*>s from the set<int>s
	signals:
		void xscroll_changed(int);
		void yscroll_changed(int);
		void viewport_width_changed(int);
		void canvas_width_changed(int);
		void preamble_width_changed(int);
		void viewport_height_changed(int);
		void canvas_height_changed(int);
		void pixels_per_whole_changed(int);
		void pos_add_changed();
			
	protected:
		virtual void draw(QPainter& p, const QRect& rect);
		ScoreEdit* parent;
		
		virtual void mousePressEvent (QMouseEvent* event);
		virtual void mouseMoveEvent (QMouseEvent* event);
		virtual void mouseReleaseEvent (QMouseEvent* event);
		virtual void resizeEvent(QResizeEvent*);
		
	public:
		ScoreCanvas(ScoreEdit*, QWidget*);
		~ScoreCanvas(){};

		void add_staves(MusECore::PartList* pl, bool all_in_one);
		void push_back_staff(staff_t& staff) { staves.push_back(staff); } //FINDMICH dirty. very dirty.

		int canvas_width();
		int canvas_height();
		int viewport_width();
		int viewport_height();
		
		int quant_power2() { return _quant_power2; }
		int quant_len() { return (1<<_quant_power2); }
		int quant_ticks() { return TICKS_PER_WHOLE / (1<<_quant_power2); }
		int pixels_per_whole() { return _pixels_per_whole; }
		int note_x_indent() { return pixels_per_whole()/quant_len()/2; }
		
		int get_last_len() {return last_len;}
		void set_last_len(int l) {last_len=l;}
		
		MusECore::Part* get_selected_part() {return selected_part;}
		void set_selected_part(MusECore::Part* p) {selected_part=p; if (selected_part) selected_part_index=selected_part->sn();}
		MusECore::Part* get_dragged_event_part() {return dragged_event_part;}
		void set_dragged_event_part(MusECore::Part* p) {dragged_event_part=p; if (dragged_event_part) dragged_event_part_index=dragged_event_part->sn();}
		
		set<MusECore::Part*> get_all_parts();
		
		void write_staves(int level, MusECore::Xml& xml) const;

		timesig_t timesig_at_tick(int t);
		MusECore::key_enum key_at_tick(int t);
		int tick_to_x(int t);
		int delta_tick_to_delta_x(int t);
		int x_to_tick(int x);
		int calc_posadd(int t);
};

int calc_measure_len(const list<int>& nums, int denom);
vector<int> create_emphasize_list(const list<int>& nums, int denom);
vector<int> create_emphasize_list(int num, int denom);

} // namespace MusEGui
#endif

