// =========================================================
// MusE
// Linux Music Editor
// xtick.cpp, flo
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

#include "xtick.h"
#include <cmath>

using namespace std;

namespace MusECore
{

	XTick XTick::from_double(double d)
	{
		XTick tmp;
		tmp.tick = floor(d);
		tmp.subtick = d - tmp.tick;
		return tmp;

	}
	double XTick::to_double() const
	{
		return tick + subtick;
	}
	unsigned XTick::to_uint() const
	{
		return tick + (subtick >= 0.5 ? 1 : 0);
	}

	XTick XTick::operator+(const XTick& t2) const
	{
		XTick tmp(this->tick + t2.tick, this->subtick + t2.subtick);
		if (tmp.subtick >= 1.0)
		{
			tmp.subtick -= 1.0;
			tmp.tick += 1;
		}
		return tmp;
	}

	XTick XTick::operator-(const XTick& t2) const
	{
		XTick tmp(this->tick - t2.tick, this->subtick - t2.subtick);
		if (tmp.subtick < 0)
		{
			tmp.subtick += 1.0;
			tmp.tick -= 1;
		}
		return tmp;
	}

	bool XTick::operator>(const XTick& t2) const
	{
		if (this->tick > t2.tick)
			return true;
		else if (this->tick == t2.tick)
			return (this->subtick > t2.subtick);
		else
			return false;
	}

	bool XTick::operator<(const XTick& t2) const
	{
		return t2 > (*this);
	}
	
	bool XTick::operator>=(const XTick& t2) const
	{
		return !((*this) < t2);
	}
	
	bool XTick::operator<=(const XTick& t2) const
	{
		return !((*this) > t2);
	}
	
	bool XTick::operator==(const XTick& t2) const
	{
		return (this->tick == t2.tick) && (this->subtick == t2.subtick); // TODO floating point == is evil :(
	}
	

} // namespace MusECore