// =========================================================
// MusE
// Linux Music Editor
// $Id: pos.cpp,v 1.11.2.1 2006/09/19 19:07:08 spamatica Exp $
// 
// (C) Copyright 2000 Werner Schweer (ws@seh.de)
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of
// the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.
// 
// =========================================================

#include <cmath>

#include "pos.h"
#include "xml.h"
#include "tempo.h"
#include "globals.h"
#include "al/sig.h"

namespace MusEGlobal
{
	extern int mtcType;
}

namespace MusECore
{

	// ---------------------------------------------------------
	// Pos
	// ---------------------------------------------------------

	Pos::Pos()
	{
		_tick = XTick(0,0);
		recalc_frames();
		sn = -1;
	}

	Pos::Pos(const Pos& p)
	{
		sn = p.sn;
		_tick = p._tick;
		_frame = p._frame;
	}

	Pos::Pos(XTick t)
	{
		_tick = t;
		recalc_frames();
	}

	Pos::Pos(unsigned t) // TODO: use XTicks instead
	{
		_tick = XTick(t);
		recalc_frames();
		
		sn = -1;
	}

	Pos::Pos(const QString& s)
	{
		int m, b, t;
		sscanf(s.toLatin1(), "%04d.%02d.%03d", &m, &b, &t);
		_tick = XTick(AL::sigmap.bar2tick(m, b, t));
		recalc_frames();
		sn = -1;
	}

	Pos::Pos(int measure, int beat, int tick, float subtick)
	{
		_tick = XTick(AL::sigmap.bar2tick(measure, beat, tick), subtick);
		recalc_frames();
		sn = -1;
	}

	Pos::Pos(int min, int sec, int frame, int subframe)
	{
		double time = min * 60.0 + sec;

		double f = frame + subframe / 100.0;
		switch (MusEGlobal::mtcType)
		{
			case 0:			// 24 frames sec
				time += f * 1.0 / 24.0;
				break;
			case 1:			// 25
				time += f * 1.0 / 25.0;
				break;
			case 2:			// 30 drop frame
				time += f * 1.0 / 30.0;
				break;
			case 3:			// 30 non drop frame
				time += f * 1.0 / 30.0;
				break;
		}
		setFrame(lrint(time * MusEGlobal::sampleRate));
		recalc_frames();
		sn = -1;
	}


	void Pos::recalc_frames()
	{
		_frame = MusEGlobal::tempomap.tick2frame(_tick, _frame, &sn);
	}


	// ---------------------------------------------------------
	// operator+=
	// ---------------------------------------------------------

	Pos& Pos::operator+=(Pos a)
	{

		//_tick += a.xtick(); // TODO implement operator+= and replace!
		_tick = _tick + a.xtick();

		sn = -1;				// invalidate cached values
		return *this;
	}

	// ---------------------------------------------------------
	// operator+=
	// ---------------------------------------------------------

	Pos& Pos::operator+=(int a)
	{
		//_tick += a; // TODO implement operator+= and replace!
		_tick = _tick + XTick(a);

		sn = -1;				// invalidate cached values
		return *this;
	}

	Pos operator+(Pos a, int b)
	{
		Pos c = a;
		return c += b;
	}

	Pos operator+(Pos a, Pos b)
	{
		Pos c = a;
		return c += b;
	}

	bool Pos::operator>=(const Pos& s) const
	{
		return _tick >= s.tick();
	}

	bool Pos::operator>(const Pos& s) const
	{
		return _tick > s.tick();
	}

	bool Pos::operator<(const Pos& s) const
	{
		return _tick < s.tick();
	}

	bool Pos::operator<=(const Pos& s) const
	{
		return _tick <= s.tick();
	}

	bool Pos::operator==(const Pos& s) const
	{
		return _tick == s.xtick();
	}

	// ---------------------------------------------------------
	// tick
	// ---------------------------------------------------------

	unsigned Pos::tick() const
	{
		return _tick.tick;
	}

	XTick Pos::xtick() const
	{
		return _tick;
	}

	// ---------------------------------------------------------
	// frame
	// ---------------------------------------------------------

	unsigned Pos::frame() const
	{
		_frame = MusEGlobal::tempomap.tick2frame(_tick, _frame, &sn);
		return _frame;
	}

	// ---------------------------------------------------------
	// setTick
	// ---------------------------------------------------------

	void Pos::setTick(unsigned pos)
	{
		setTick(XTick(pos));
	}

	void Pos::setTick(XTick pos)
	{
		_tick = pos;
		sn = -1;
		
		recalc_frames();
	}

	// ---------------------------------------------------------
	// setFrame
	// ---------------------------------------------------------

	void Pos::setFrame(unsigned pos)
	{
		_frame = pos;
		sn = -1;
		_tick = MusEGlobal::tempomap.frame2xtick(pos, &sn);
	}

	// ---------------------------------------------------------
	// write
	// ---------------------------------------------------------

	void Pos::write(int level, Xml& xml, const char* name) const
	{
		xml.nput(level++, "<%s ", name);
		xml.nput("tick=\"%d\" subtick=\"%f\"", _tick.tick, _tick.subtick);
		xml.put(" />", name);
	}

	// ---------------------------------------------------------
	// read
	// ---------------------------------------------------------

	void Pos::read(Xml& xml, const char* name)
	{
		sn = -1;
		for (;;)
		{
			Xml::Token token = xml.parse();
			const QString& tag = xml.s1();
			switch (token)
			{
				case Xml::Error:
				case Xml::End:
					return;

				case Xml::TagStart:
					xml.unknown(name);
					break;

				case Xml::Attribut:
					if (tag == "tick")
					{
						_tick = XTick(xml.s2().toInt());
					}
					else if (tag == "subtick")
					{
						_tick.subtick = xml.s2().toDouble();
					}
					else if (tag == "sample" || tag == "frame") // obsolete
					{
						printf("DEBUG: Pos::read wants type=FRAMES, but this is denied.\n");
						setFrame(xml.s2().toInt());
					}
					else
						xml.unknown(name);
					break;

				case Xml::TagEnd:
					if (tag == name)
						return;
				default:
					break;
			}
		}
	}

	// ---------------------------------------------------------
	// PosLen
	// ---------------------------------------------------------

	PosLen::PosLen()
	{
		_lenTick = XTick(0);
		_lenFrame = 0;
		sn = -1;
		_lenType = Pos::TICKS;
	}

	PosLen::PosLen(const PosLen& p) : Pos(p)
	{
		_lenTick = p._lenTick;
		_lenFrame = p._lenFrame;
		_lenType = p._lenType;
		sn = -1;
	}

	// ---------------------------------------------------------
	// dump
	// ---------------------------------------------------------

	void PosLen::dump(int n) const
	{
		Pos::dump(n);
		printf("  Len(");
		switch (lenType())
		{
			case FRAMES:
				printf("samples=%d)\n", _lenFrame);
				break;
			
			case TICKS:
				printf("ticks=%d + %f)\n", _lenTick.tick, _lenTick.subtick);
				break;
		}
	}

	void Pos::dump(int /* n */ ) const
	{
		printf("Pos(sn=%d, ticks=%d + %f)", sn, _tick.tick, _tick.subtick);
	}

	// ---------------------------------------------------------
	// write
	// ---------------------------------------------------------

	void PosLen::write(int level, Xml& xml, const char* name) const
	{
		xml.nput(level++, "<%s ", name);
		
		if (_lenType==Pos::TICKS)
			xml.nput("tick=\"%d\" subtick=\"%f\" len=\"%d\" lensubtick=\"%f\"", xtick().tick, xtick().subtick, _lenTick.tick, _lenTick.subtick);
		else
			xml.nput("tick=\"%d\" subtick=\"%f\" lenframes=\"%d\"", xtick().tick, xtick().subtick, _lenFrame);
		
		xml.put(" />", name);
	}

	// ---------------------------------------------------------
	// read
	// ---------------------------------------------------------

	void PosLen::read(Xml& xml, const char* name)
	{
		/* Unfortunately, old versions of MusE have used the "len=..." attribute,
		   regardless whether type was TICKS or FRAMES. Thus, we must use this
		   flag to check whether "len" means "tick length" as today, or actually
		   means "lenframes", but was stored using an old MusE version. */
		bool in_deprecated_sample_mode = false;
		
		setLenType(TICKS);
		sn = -1;
		for (;;)
		{
			Xml::Token token = xml.parse();
			const QString& tag = xml.s1();
			switch (token)
			{
				case Xml::Error:
				case Xml::End:
					return;

				case Xml::TagStart:
					xml.unknown(name);
					break;

				case Xml::Attribut:
					if (tag == "tick")
					{
						setTick(xml.s2().toInt());
					}
					if (tag == "subtick")
					{
						XTick temp = xtick();
						temp.subtick=xml.s2().toDouble();
						setTick(temp);
					}
					else if (tag == "sample")
					{
						printf("PosLen::read wants FRAMES but this is denied.\n");
						in_deprecated_sample_mode = true;
						setFrame(xml.s2().toInt());
					}
					else if ((tag == "len") && !in_deprecated_sample_mode)
					{
						_lenType=Pos::TICKS;
						setLenTick(xml.s2().toInt());
					}
					else if (tag == "lensubtick")
					{
						if (in_deprecated_sample_mode)
							printf("THIS CANNOT HAPPEN in PosLen::read: lensubtick was read, but in_deprecated_sample_mode==true!\n");
						else
						{
							_lenTick.subtick = xml.s2().toDouble();
							setLenTick(_lenTick); // this is necessary!
						}
					}
					else if ((tag == "lenframes") || ((tag == "len") && in_deprecated_sample_mode))
					{
						_lenType=Pos::FRAMES;
						setLenFrame(xml.s2().toInt());
					}
					else
						xml.unknown(name);
					break;

				case Xml::TagEnd:
					if (tag == name)
						return;
				default:
					break;
			}
		}
	}

	// ---------------------------------------------------------
	// setLenTick
	// ---------------------------------------------------------

	void PosLen::setLenTick(unsigned len)
	{
		setLenTick(XTick(len));
	}

	void PosLen::setLenTick(XTick len)
	{
		_lenTick = len;
		sn = -1;
		_lenFrame = MusEGlobal::tempomap.deltaTick2frame(xtick(), xtick() + len, &sn);
	}

	// ---------------------------------------------------------
	// setLenFrame
	// ---------------------------------------------------------

	void PosLen::setLenFrame(unsigned len)
	{
		_lenFrame = len;
		sn = -1;
		_lenTick = MusEGlobal::tempomap.deltaFrame2xtick(frame(), frame() + len, &sn);
	}

	// ---------------------------------------------------------
	// lenTick
	// ---------------------------------------------------------

	unsigned PosLen::lenTick() const
	{
		return lenXTick().tick;
	}

	XTick PosLen::lenXTick() const
	{
		if (lenType() == FRAMES)
			_lenTick = MusEGlobal::tempomap.deltaFrame2xtick(frame(), frame() + _lenFrame, &sn);
		return _lenTick;
	}

	// ---------------------------------------------------------
	// lenFrame
	// ---------------------------------------------------------

	unsigned PosLen::lenFrame() const
	{
		if (lenType() == TICKS)
			_lenFrame = MusEGlobal::tempomap.deltaTick2frame(xtick(), xtick() + _lenTick, &sn);
		return _lenFrame;
	}

	// ---------------------------------------------------------
	// end
	// ---------------------------------------------------------

	Pos PosLen::end() const
	{
		Pos pos(*this);
		pos.invalidSn();
		
		switch (lenType())
		{
			case FRAMES:
				pos.setFrame(pos.frame() + _lenFrame);
				break;
			
			case TICKS:
				pos.setTick(pos.xtick() + _lenTick);
				break;
		}
		return pos;
	}
	
	void PosLen::setLenType(TType t)
	{
		if (_lenType==t) return;
		
		if (_lenType==FRAMES)
			_lenTick = MusEGlobal::tempomap.deltaFrame2xtick(frame(), frame() + _lenFrame, &sn);
		else
			_lenFrame = MusEGlobal::tempomap.deltaTick2frame(xtick(), xtick() + _lenTick, &sn);
		
		_lenType=t;
	}

	// ---------------------------------------------------------
	// setPos
	// ---------------------------------------------------------

	void PosLen::setPos(const Pos& pos)
	{
		setTick(pos.xtick());
	}

	// ---------------------------------------------------------
	// mbt
	// ---------------------------------------------------------

	void Pos::mbt(int* bar, int* beat, int* tk) const
	{
		AL::sigmap.tickValues(tick(), bar, beat, (unsigned*) tk);
	}

	// ---------------------------------------------------------
	// msf
	// ---------------------------------------------------------

	void Pos::msf(int* min, int* sec, int* fr, int* subFrame) const
	{
		double time = double(frame()) / double(MusEGlobal::sampleRate);
		*min = int(time) / 60;
		*sec = int(time) % 60;
		double rest = time - (*min * 60 + *sec);
		switch (MusEGlobal::mtcType)
		{
			case 0:			// 24 frames sec
				rest *= 24;
				break;
			case 1:			// 25
				rest *= 25;
				break;
			case 2:			// 30 drop frame
				rest *= 30;
				break;
			case 3:			// 30 non drop frame
				rest *= 30;
				break;
		}
		*fr = int(rest);
		*subFrame = int((rest - *fr) * 100);
	}

	// ---------------------------------------------------------
	// isValid
	// ---------------------------------------------------------

	bool Pos::isValid(int, int, int)
	{
		return true;
	}

	// ---------------------------------------------------------
	// isValid
	// ---------------------------------------------------------

	bool Pos::isValid(int, int, int, int)
	{
		return true;
	}

} // namespace MusECore
