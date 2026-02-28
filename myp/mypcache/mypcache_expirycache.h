// mypcache_expirycache.h                                             -*-C++-*-

// ----------------------------------------------------------------------------
// Copyright 2026 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------------------------------------------------------

#ifndef INCLUDED_MYPCACHE_EXPIRYCACHE
#define INCLUDED_MYPCACHE_EXPIRYCACHE

//@PURPOSE: Provide a fixed-size key-value cache with cache-level TTL expiry.
//
//@CLASSES:
//  mypcache::ExpiryCache: fixed-capacity cache with configurable TTL
//
//@DESCRIPTION: This component provides 'mypcache::ExpiryCache<KEY,VALUE>', a
// fixed-capacity, key-value cache in which every entry shares a single,
// cache-level time-to-live (TTL).  When the cache is at capacity, the oldest
// (and therefore nearest-to-expiry) entry is evicted in O(1) time.  Expired
// entries are also reclaimed lazily from the front of an insertion-ordered
// list on each call to 'put', which is correct because uniform TTL guarantees
// that older entries always expire before newer ones.
//
// Data structures:
//: o 'EntryList' -- a 'bsl::list' maintaining entries in insertion order,
//:   oldest at the front and newest at the back.
//: o 'IndexMap' -- a 'bsl::unordered_map' from key to list iterator,
//:   enabling O(1) lookup, targeted removal, and key-update moves.
//
// Complexity:
//: o 'put' -- O(1) amortized; lazy expiry sweep cost is amortized over all
//:   insertions because each entry is swept at most once.
//: o 'get' -- O(1).
//: o Capacity eviction -- O(1): always removes the front of the list.
//
// This component is *not* thread-safe.  Callers must provide external
// synchronization when instances are shared across threads.
//
// Note: This component uses standard-library headers ('<list>',
// '<unordered_map>', '<chrono>') in place of BSL equivalents
// ('bsl_list.h', 'bsl_unordered_map.h', 'bsls_timeutil.h') because the
// BSL library is not installed in this build environment.  All namespace and
// structural conventions follow BDE standards.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Caching Slow Query Results With a Five-Second TTL
///- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Suppose we have a slow upstream query and want to cache up to 1000 results
// for five seconds before they are considered stale.
//
//..
//  mypcache::ExpiryCache<std::string, std::string> cache(1000, 5000);
//
//  cache.put("user:42", "Alice");
//
//  std::string result;
//  bool found = cache.get("user:42", &result);
//  assert(found);
//  assert(result == "Alice");
//..

#include <chrono>
#include <cstddef>
#include <list>
#include <unordered_map>

namespace BloombergLP {
namespace mypcache {

                            // =================
                            // class ExpiryCache
                            // =================

template <class KEY, class VALUE>
class ExpiryCache {
    // A fixed-capacity key-value cache with a uniform, cache-level
    // time-to-live (TTL).  Expired entries are lazily reclaimed from the
    // front of an insertion-ordered list; capacity eviction always removes
    // the oldest (front) entry.  All primary operations are O(1) amortized.
    // This class is *not* thread-safe.

  private:
    // PRIVATE TYPES
    typedef std::chrono::steady_clock               Clock;
    typedef std::chrono::time_point<Clock>          TimePoint;
    typedef std::chrono::milliseconds               Duration;

    struct Entry {
        // Value-semantic aggregate holding a cached key-value pair and its
        // absolute expiry time point.

        KEY       d_key;
        VALUE     d_value;
        TimePoint d_expiry;
    };

    typedef std::list<Entry>                        EntryList;
    typedef typename EntryList::iterator            EntryListIter;
    typedef std::unordered_map<KEY, EntryListIter>  IndexMap;

    // DATA
    std::size_t d_maxSize;  // maximum number of live entries
    Duration    d_ttl;      // lifetime applied uniformly to every entry
    EntryList   d_entries;  // insertion order: front = oldest, back = newest
    IndexMap    d_index;    // key -> list iterator for O(1) access

    // PRIVATE MANIPULATORS
    void evictExpiredFront();
        // Remove all entries from the front of 'd_entries' whose absolute
        // expiry time is at or before 'Clock::now()', erasing them from
        // 'd_index' as well.  Because every entry shares the same TTL the
        // front entry is always the next to expire, so all expired entries
        // form a contiguous prefix of the list.

    void evictOldest();
        // Remove the front (oldest) entry from 'd_entries' and 'd_index'.
        // The behavior is undefined unless '!d_entries.empty()'.

  public:
    // CREATORS
    ExpiryCache(std::size_t maxSize, int ttlMilliseconds);
        // Create an empty 'ExpiryCache' with the specified 'maxSize' maximum
        // number of entries and the specified 'ttlMilliseconds' time-to-live
        // (in milliseconds) applied uniformly to all entries.  The behavior
        // is undefined unless '0 < maxSize' and '0 < ttlMilliseconds'.

    // MANIPULATORS
    void put(const KEY& key, const VALUE& value);
        // Insert or update the entry for the specified 'key' to the specified
        // 'value', resetting its expiry to 'now + ttl'.  If 'key' is already
        // present its old entry is removed first, so a pure key update never
        // triggers an extra capacity eviction.  If the cache is still at
        // capacity after the optional removal and the lazy expiry sweep, the
        // oldest live entry is evicted before the new entry is appended.

    bool get(const KEY& key, VALUE *value);
        // If the specified 'key' is present and its entry has not expired,
        // load the associated value into the specified '*value' and return
        // 'true'.  Otherwise, if 'key' is present but expired, remove that
        // entry and return 'false'.  Return 'false' if 'key' is not present.

    void clear();
        // Remove all entries from the cache.

    // ACCESSORS
    std::size_t size() const;
        // Return the number of entries currently held in the cache.  This
        // count may include entries whose TTL has elapsed but that have not
        // yet been lazily evicted.
};

// ============================================================================
//                      INLINE FUNCTION DEFINITIONS
// ============================================================================

// PRIVATE MANIPULATORS

template <class KEY, class VALUE>
void ExpiryCache<KEY, VALUE>::evictExpiredFront()
{
    const TimePoint now = Clock::now();
    while (!d_entries.empty() && now >= d_entries.front().d_expiry) {
        d_index.erase(d_entries.front().d_key);
        d_entries.pop_front();
    }
}

template <class KEY, class VALUE>
void ExpiryCache<KEY, VALUE>::evictOldest()
{
    d_index.erase(d_entries.front().d_key);
    d_entries.pop_front();
}

// CREATORS

template <class KEY, class VALUE>
ExpiryCache<KEY, VALUE>::ExpiryCache(std::size_t maxSize, int ttlMilliseconds)
: d_maxSize(maxSize)
, d_ttl(ttlMilliseconds)
{
}

// MANIPULATORS

template <class KEY, class VALUE>
void ExpiryCache<KEY, VALUE>::put(const KEY& key, const VALUE& value)
{
    evictExpiredFront();

    typename IndexMap::iterator existing = d_index.find(key);
    if (existing != d_index.end()) {
        d_entries.erase(existing->second);
        d_index.erase(existing);
    }

    if (d_entries.size() >= d_maxSize) {
        evictOldest();
    }

    const TimePoint expiry = Clock::now() + d_ttl;
    d_entries.push_back(Entry{key, value, expiry});
    d_index[key] = --d_entries.end();
}

template <class KEY, class VALUE>
bool ExpiryCache<KEY, VALUE>::get(const KEY& key, VALUE *value)
{
    typename IndexMap::iterator it = d_index.find(key);
    if (it == d_index.end()) {
        return false;                                                 // RETURN
    }

    const EntryListIter entryIt = it->second;
    if (Clock::now() >= entryIt->d_expiry) {
        d_entries.erase(entryIt);
        d_index.erase(it);
        return false;                                                 // RETURN
    }

    *value = entryIt->d_value;
    return true;
}

template <class KEY, class VALUE>
void ExpiryCache<KEY, VALUE>::clear()
{
    d_entries.clear();
    d_index.clear();
}

// ACCESSORS

template <class KEY, class VALUE>
std::size_t ExpiryCache<KEY, VALUE>::size() const
{
    return d_entries.size();
}

}  // close package namespace
}  // close enterprise namespace

#endif  // INCLUDED_MYPCACHE_EXPIRYCACHE
