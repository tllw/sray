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
#include <assert.h>
#include <pthread.h>

template <typename D, typename K>
int DataCache<D, K>::get_index() const
{
	if(data.size() == 1) return 0;

#ifndef NO_THREADS
	pthread_t thread = pthread_self();

	hash_table::iterator iter = thread_id_map.find(thread);
	if(iter == thread_id_map.end()) {
		static pthread_mutex_t resize_mutex = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_lock(&resize_mutex);

		int count = (int)data.size();

		((DataCache<D, K>*)this)->data.resize(count + 1);
		((DataCache<D, K>*)this)->key.resize(count + 1, invalid_key);

		thread_id_map[thread] = count;

		pthread_mutex_unlock(&resize_mutex);
		return count;
	}
	return iter->second;
#else
	if(data.empty()) {
		((DataCache<D, K>*)this)->data.resize(1);
		((DataCache<D, K>*)this)->key.resize(1, invalid_key);
	}
	return 0;
#endif
}

template <typename D, typename K>
void DataCache<D, K>::set_invalid_key(const K &inval)
{
	invalid_key = inval;
}

template <typename D, typename K>
bool DataCache<D, K>::is_valid(const K &key) const
{
	if(data.empty()) {
		return false;
	}

	int idx = get_index();
	assert(idx < (int)data.size());

	return key == this->key[idx];
}

template <typename D, typename K>
void DataCache<D, K>::invalidate()
{
	for(size_t i=0; i<key.size(); i++) {
		key[i] = invalid_key;
	}
}


template <typename D, typename K>
const D &DataCache<D, K>::get_data() const
{
	int idx = get_index();
	assert(idx < (int)data.size());

	return data[idx];
}

template <typename D, typename K>
void DataCache<D, K>::set_data(const D &data, const K &key)
{
	int idx = get_index();
	assert(idx < (int)this->data.size());

	this->data[idx] = data;
	this->key[idx] = key;
}
