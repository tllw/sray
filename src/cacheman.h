#ifndef CACHEMAN_H_
#define CACHEMAN_H_

#include <vector>

#ifndef NO_THREADS
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

#include <pthread.h>
#endif

/** DataCache implements the concept of a cached bit of data
 * per thread. The cached data has an associated key, if the
 * key supplied to is_valid, is different than the key stored
 * with the cache, then the cache needs updating. Otherwise, the
 * value returned by get_data can be used.
 */
template <typename D, typename K = long>
class DataCache {
private:
	std::vector<D> data;
	std::vector<K> key;
	K invalid_key;

#ifndef NO_THREADS
#ifdef __GNUC__
	typedef __gnu_cxx::hash_map<pthread_t, int> hash_table;
#else
	typedef std::hash_map<pthread_t, int> hash_table;
#endif
	mutable hash_table thread_id_map;
#endif

	int get_index() const;

public:
	/** set the key value to be considered invalid */
	void set_invalid_key(const K &inval);

	/** returns true if the key matches the key of the last set_data */
	bool is_valid(const K &key) const;

	/** after a call to invalidate, the next is_valid will return false */
	void invalidate();

	/** retrieve the data part of the cache */
	const D &get_data() const;

	/** cache a data item with its corresponding key */
	void set_data(const D &data, const K &key);
};

#include "cacheman.inl"

#endif	// CACHEMAN_H_
