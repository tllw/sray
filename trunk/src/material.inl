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
