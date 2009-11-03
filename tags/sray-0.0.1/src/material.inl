/*
This file is part of the s-ray renderer <http://code.google.com/p/sray>.
Copyright (C) 2009 John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
inline float MatAttrib::get_value(const Vector2 &tc, unsigned int time) const
{
	return get_color(tc, time).x;
}

inline Color MatAttrib::get_color(const Vector2 &tc, unsigned int time) const
{
	if(tex) {
		return tex->lookup(tc, time) * col;
	}
	return col;
}

inline bool MatAttrib::operator <(const MatAttrib &rhs) const
{
	return name < rhs.name;
}
