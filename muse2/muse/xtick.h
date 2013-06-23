// =========================================================
// MusE
// Linux Music Editor
// xtick.h, flo
// 
// (C) Copyright 2013 Florian Jung (flo@windfisch.org)
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

#ifndef __XTICK_H__
#define __XTICK_H__

namespace MusECore
{

	struct XTick				// eXtended tick with subticks.
	{
		unsigned tick;			// do NOT just grab this and ignore subtick if
		// you're doing integer calculations!
		// use to_uint() instead, which takes care of proper rounding.
		double subtick;
		bool invalid;

		XTick(unsigned t = 0, double s = 0.0)
		{
			tick = t;
			subtick = s;
			invalid = false;
		}
		static XTick from_double(double d);
		double to_double() const;
		unsigned to_uint() const; // TODO: possibly add a cast to unsigned later, and find and remove all .to_uint()

		XTick operator+(const XTick& t2) const;
		XTick operator-(const XTick& t2) const;
		bool operator>=(const XTick& t2) const;
		bool operator>(const XTick& t2) const;
		bool operator<=(const XTick& t2) const;
		bool operator<(const XTick& t2) const;
		bool operator==(const XTick& t2) const;
		
		static XTick createInvalidXTick() { XTick tmp; tmp.invalid=true; return tmp; }
	};

} // namespace MusECore

#endif