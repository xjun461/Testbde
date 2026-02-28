// mypcache_expirycache.t.cpp                                         -*-C++-*-

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

#include <mypcache_expirycache.h>

#include <iostream>
#include <string>
#include <unistd.h>  // usleep

using namespace BloombergLP;

// ============================================================================
//                     STANDARD BDE ASSERT TEST MACROS
// ----------------------------------------------------------------------------

static int testStatus = 0;

static void aSsErT(bool condition, const char *message, int line)
{
    if (condition) {
        std::cout << "Error " __FILE__ "(" << line << "): " << message
                  << "    (failed)" << std::endl;
        if (testStatus >= 0 && testStatus <= 100) {
            ++testStatus;
        }
    }
}

#define ASSERT(X) aSsErT(!(X), #X, __LINE__)

// ============================================================================
//                  STANDARD BDE LOOP-ASSERT TEST MACROS
// ----------------------------------------------------------------------------

#define LOOP_ASSERT(I, X)                                                     \
    if (!(X)) {                                                               \
        std::cout << #I << ": " << I << "\n";                                 \
        aSsErT(true, #X, __LINE__);                                           \
    }

// ============================================================================
//                  SEMI-STANDARD TEST OUTPUT MACROS
// ----------------------------------------------------------------------------

#define P(X)  std::cout << #X " = " << (X) << std::endl;
#define Q(X)  std::cout << "<| " #X " |>" << std::endl;
#define P_(X) std::cout << #X " = " << (X) << ", " << std::flush;
#define L_    __LINE__

// ============================================================================
//                        GLOBAL TYPEDEFS FOR TESTING
// ----------------------------------------------------------------------------

typedef mypcache::ExpiryCache<int, int>                 IntCache;
typedef mypcache::ExpiryCache<std::string, std::string> StrCache;

// ============================================================================
//                              MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int  test    = argc > 1 ? atoi(argv[1]) : 0;
    bool verbose = argc > 2;

    std::cout << "TEST " << __FILE__ << " CASE " << test << std::endl;

    switch (test) { case 0:

      case 5: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE
        //   Verify the usage example from the component header compiles and
        //   produces the expected results.
        //
        // Concerns:
        //: 1 The usage example shown in the header compiles and runs.
        //: 2 'put' followed by 'get' returns the stored value.
        //
        // Plan:
        //: 1 Reproduce the usage example verbatim and assert the results.
        //    (C-1, C-2)
        //
        // Testing:
        //   USAGE EXAMPLE
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nUSAGE EXAMPLE"
                               << "\n=============" << std::endl;

        mypcache::ExpiryCache<std::string, std::string> cache(1000, 5000);

        cache.put("user:42", "Alice");

        std::string result;
        bool found = cache.get("user:42", &result);
        ASSERT(found);
        ASSERT(result == "Alice");

      } break;

      case 4: {
        // --------------------------------------------------------------------
        // KEY UPDATE
        //   Verify that re-inserting an existing key replaces its value and
        //   moves it to the back (newest) position without consuming extra
        //   capacity.
        //
        // Concerns:
        //: 1 Re-putting an existing key updates its value.
        //: 2 The updated entry receives a fresh TTL.
        //: 3 A pure key update does not evict any other entry (net size
        //:   change is zero).
        //: 4 The updated entry moves to the back, so a subsequent capacity
        //:   eviction removes a *different* (older) key.
        //
        // Plan:
        //: 1 Fill a cache of capacity 3 with keys 1, 2, 3.  Re-put key 1
        //:   with a new value; verify all three keys survive.  (C-1, C-2, C-3)
        //: 2 Insert a fourth key; verify key 2 (now the oldest) is evicted
        //:   while keys 1, 3, and 4 remain.  (C-4)
        //
        // Testing:
        //   void put(const KEY&, const VALUE&);
        //   bool get(const KEY&, VALUE *);
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nKEY UPDATE"
                               << "\n==========" << std::endl;

        IntCache cache(3, 60000);  // maxSize=3, ttl=60s

        cache.put(1, 10);
        cache.put(2, 20);
        cache.put(3, 30);
        ASSERT(cache.size() == 3);

        // Re-put key 1: old entry removed, new entry appended at back.
        // List order becomes: [2, 3, 1].  Size stays 3.
        cache.put(1, 99);
        ASSERT(cache.size() == 3);

        int v = 0;
        ASSERT(cache.get(1, &v));  ASSERT(v == 99);
        ASSERT(cache.get(2, &v));  ASSERT(v == 20);
        ASSERT(cache.get(3, &v));  ASSERT(v == 30);

        // Inserting key 4 evicts the front (key 2, now the oldest).
        // List becomes: [3, 1, 4].
        cache.put(4, 40);
        ASSERT(cache.size() == 3);

        ASSERT(!cache.get(2, &v));           // evicted
        ASSERT( cache.get(3, &v));  ASSERT(v == 30);
        ASSERT( cache.get(1, &v));  ASSERT(v == 99);
        ASSERT( cache.get(4, &v));  ASSERT(v == 40);

      } break;

      case 3: {
        // --------------------------------------------------------------------
        // CAPACITY EVICTION
        //   Verify that the oldest entry is evicted when the cache is full.
        //
        // Concerns:
        //: 1 Inserting beyond 'maxSize' entries evicts the oldest one.
        //: 2 'size()' never exceeds 'maxSize'.
        //: 3 Entries inserted after the eviction are accessible.
        //: 4 Multiple sequential inserts evict in FIFO order.
        //
        // Plan:
        //: 1 Fill a cache of capacity 3 with keys 1, 2, 3.  Insert key 4
        //:   and verify key 1 is gone and keys 2, 3, 4 are present.  (C-1-3)
        //: 2 Insert keys 5 and 6; verify keys 2 and 3 are gone and keys
        //:   4, 5, 6 are present.  (C-4)
        //
        // Testing:
        //   void put(const KEY&, const VALUE&);
        //   bool get(const KEY&, VALUE *);
        //   std::size_t size() const;
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nCAPACITY EVICTION"
                               << "\n=================" << std::endl;

        IntCache cache(3, 60000);  // maxSize=3, ttl=60s

        cache.put(1, 100);
        cache.put(2, 200);
        cache.put(3, 300);
        ASSERT(cache.size() == 3);

        // Inserting a 4th entry must evict key 1 (oldest).
        cache.put(4, 400);
        ASSERT(cache.size() == 3);

        int v = 0;
        ASSERT(!cache.get(1, &v));           // evicted
        ASSERT( cache.get(2, &v));  ASSERT(v == 200);
        ASSERT( cache.get(3, &v));  ASSERT(v == 300);
        ASSERT( cache.get(4, &v));  ASSERT(v == 400);

        // Two more inserts evict keys 2 and 3.
        cache.put(5, 500);
        cache.put(6, 600);
        ASSERT(cache.size() == 3);

        ASSERT(!cache.get(2, &v));           // evicted
        ASSERT(!cache.get(3, &v));           // evicted
        ASSERT( cache.get(4, &v));  ASSERT(v == 400);
        ASSERT( cache.get(5, &v));  ASSERT(v == 500);
        ASSERT( cache.get(6, &v));  ASSERT(v == 600);

      } break;

      case 2: {
        // --------------------------------------------------------------------
        // TTL EXPIRY
        //   Verify that entries become inaccessible once their TTL elapses,
        //   and that the lazy eviction sweep correctly reclaims them.
        //
        // Concerns:
        //: 1 'get' returns 'true' for live (non-expired) entries.
        //: 2 'get' returns 'false' and removes the entry after TTL elapses.
        //: 3 After a 'put' triggers the lazy sweep, 'size()' reflects only
        //:   unexpired entries.
        //: 4 New entries inserted after expiry of old ones are accessible.
        //
        // Plan:
        //: 1 Insert three entries with a 100 ms TTL; verify they are live
        //:   immediately.  (C-1)
        //: 2 Sleep 200 ms to ensure expiry.  Verify each 'get' returns
        //:   'false'.  (C-2)
        //: 3 Insert a new entry; the ensuing lazy sweep must clear the
        //:   expired entries; verify 'size() == 1'.  (C-3)
        //: 4 Verify the new entry is accessible.  (C-4)
        //
        // Testing:
        //   void put(const KEY&, const VALUE&);
        //   bool get(const KEY&, VALUE *);
        //   std::size_t size() const;
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nTTL EXPIRY"
                               << "\n==========" << std::endl;

        IntCache cache(10, 100);  // maxSize=10, ttl=100ms

        cache.put(1, 11);
        cache.put(2, 22);
        cache.put(3, 33);
        ASSERT(cache.size() == 3);

        int v = 0;
        ASSERT(cache.get(1, &v));  ASSERT(v == 11);
        ASSERT(cache.get(2, &v));  ASSERT(v == 22);
        ASSERT(cache.get(3, &v));  ASSERT(v == 33);

        ::usleep(200000);  // sleep 200ms; entries (100ms TTL) are now expired

        ASSERT(!cache.get(1, &v));
        ASSERT(!cache.get(2, &v));
        ASSERT(!cache.get(3, &v));

        // Inserting a new entry triggers evictExpiredFront; the three expired
        // entries are swept from the list, leaving only the new one.
        cache.put(4, 44);
        ASSERT(cache.size() == 1);

        ASSERT(cache.get(4, &v));  ASSERT(v == 44);

      } break;

      case 1: {
        // --------------------------------------------------------------------
        // BASIC FUNCTIONALITY
        //   Verify the fundamental put/get/size/clear contract.
        //
        // Concerns:
        //: 1 A freshly constructed cache has size 0.
        //: 2 'put' increases 'size()' by one for a new key.
        //: 3 'get' retrieves the value stored by 'put'.
        //: 4 'get' returns 'false' for a key that was never inserted.
        //: 5 Multiple independent entries coexist correctly.
        //: 6 'clear' removes all entries and resets 'size()' to 0.
        //
        // Plan:
        //: 1 Construct an 'IntCache'; verify initial size.  (C-1)
        //: 2 Insert one entry and verify size and retrieval.  (C-2, C-3)
        //: 3 Attempt to retrieve an absent key; verify 'false'.  (C-4)
        //: 4 Insert two more entries; verify all three coexist.  (C-5)
        //: 5 Call 'clear'; verify size drops to 0 and entries are gone.(C-6)
        //
        // Testing:
        //   ExpiryCache(std::size_t, int);
        //   void put(const KEY&, const VALUE&);
        //   bool get(const KEY&, VALUE *);
        //   void clear();
        //   std::size_t size() const;
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nBASIC FUNCTIONALITY"
                               << "\n===================" << std::endl;

        IntCache cache(100, 60000);  // maxSize=100, ttl=60s

        ASSERT(cache.size() == 0);

        cache.put(42, 1234);
        ASSERT(cache.size() == 1);

        int v = 0;
        ASSERT( cache.get(42, &v));  ASSERT(v == 1234);
        ASSERT(!cache.get(99, &v));  // absent key

        cache.put(7, 777);
        cache.put(8, 888);
        ASSERT(cache.size() == 3);

        ASSERT(cache.get( 7, &v));  ASSERT(v == 777);
        ASSERT(cache.get( 8, &v));  ASSERT(v == 888);
        ASSERT(cache.get(42, &v));  ASSERT(v == 1234);

        cache.clear();
        ASSERT(cache.size() == 0);
        ASSERT(!cache.get(42, &v));

      } break;

      default: {
        std::cerr << "WARNING: CASE `" << test << "' NOT FOUND." << std::endl;
        testStatus = -1;
      }
    }

    if (testStatus > 0) {
        std::cerr << "Error, non-zero test status = " << testStatus << "."
                  << std::endl;
    }

    return testStatus;
}
