// mypalg_twosum.t.cpp                                                -*-C++-*-

#include <mypalg_twosum.h>

#include <iostream>
#include <stdexcept>
#include <vector>

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

typedef mypalg::TwoSum Obj;

// ============================================================================
//                              MAIN PROGRAM
// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int test    = argc > 1 ? atoi(argv[1]) : 0;
    bool verbose = argc > 2;

    std::cout << "TEST " << __FILE__ << " CASE " << test << std::endl;

    switch (test) { case 0:

      case 5: {
        // --------------------------------------------------------------------
        // USAGE EXAMPLE
        //   Verify the usage example from the component header compiles and
        //   produces correct results.
        //
        // Concerns:
        //: 1 The usage example shown in the header produces correct output.
        //
        // Plan:
        //: 1 Copy and run the usage example, asserting expected output. (C-1)
        //
        // Testing:
        //   USAGE EXAMPLE
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nUSAGE EXAMPLE"
                               << "\n=============" << std::endl;

        std::vector<int> nums   = {2, 7, 11, 15};
        int              target = 9;

        std::vector<int> result = Obj::find(nums, target);
        ASSERT(result[0] == 0);
        ASSERT(result[1] == 1);

      } break;

      case 4: {
        // --------------------------------------------------------------------
        // NO SOLUTION: EXCEPTION THROWN
        //   Ensure that 'find' throws 'std::invalid_argument' when no valid
        //   pair exists.
        //
        // Concerns:
        //: 1 'find' throws 'std::invalid_argument' when no solution exists.
        //
        // Plan:
        //: 1 Call 'find' with an input where no pair sums to the target and
        //:   verify that the expected exception is thrown. (C-1)
        //
        // Testing:
        //   static std::vector<int> find(const std::vector<int>&, int);
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nNO SOLUTION: EXCEPTION THROWN"
                               << "\n=============================" << std::endl;

        std::vector<int> nums   = {1, 2, 3};
        int              target = 100;

        bool exceptionThrown = false;
        try {
            Obj::find(nums, target);
        }
        catch (const std::invalid_argument&) {
            exceptionThrown = true;
        }
        ASSERT(exceptionThrown);

      } break;

      case 3: {
        // --------------------------------------------------------------------
        // NEGATIVE NUMBERS
        //   Verify 'find' works correctly when 'nums' contains negative values.
        //
        // Concerns:
        //: 1 Negative integers are handled correctly.
        //: 2 A negative target is handled correctly.
        //
        // Plan:
        //: 1 Supply inputs that include negative numbers and verify the
        //:   returned indices are correct. (C-1, C-2)
        //
        // Testing:
        //   static std::vector<int> find(const std::vector<int>&, int);
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nNEGATIVE NUMBERS"
                               << "\n================" << std::endl;

        {
            // nums[1] + nums[3] = -3 + (-4) = -7
            std::vector<int> nums   = {1, -3, 5, -4};
            int              target = -7;

            std::vector<int> result = Obj::find(nums, target);
            ASSERT(result.size() == 2);
            ASSERT(nums[result[0]] + nums[result[1]] == target);
        }

        {
            // nums[0] + nums[2] = -1 + 1 = 0
            std::vector<int> nums   = {-1, 3, 1, 7};
            int              target = 0;

            std::vector<int> result = Obj::find(nums, target);
            ASSERT(result.size() == 2);
            ASSERT(nums[result[0]] + nums[result[1]] == target);
        }

      } break;

      case 2: {
        // --------------------------------------------------------------------
        // BOUNDARY CONDITIONS
        //   Test edge cases such as a two-element array and duplicate values.
        //
        // Concerns:
        //: 1 A two-element array whose values sum to target returns [0, 1].
        //: 2 Duplicate values that together form the target are handled
        //:   correctly.
        //
        // Plan:
        //: 1 Call 'find' with a two-element array and verify the indices. (C-1)
        //: 2 Call 'find' with duplicate values summing to target and verify
        //:   the returned indices are distinct. (C-2)
        //
        // Testing:
        //   static std::vector<int> find(const std::vector<int>&, int);
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nBOUNDARY CONDITIONS"
                               << "\n===================" << std::endl;

        {
            // Minimal two-element input
            std::vector<int> nums   = {3, 5};
            int              target = 8;

            std::vector<int> result = Obj::find(nums, target);
            ASSERT(result.size() == 2);
            ASSERT(result[0] == 0);
            ASSERT(result[1] == 1);
        }

        {
            // Duplicate values: nums[0] + nums[1] = 3 + 3 = 6
            std::vector<int> nums   = {3, 3};
            int              target = 6;

            std::vector<int> result = Obj::find(nums, target);
            ASSERT(result.size() == 2);
            ASSERT(result[0] != result[1]);
            ASSERT(nums[result[0]] + nums[result[1]] == target);
        }

      } break;

      case 1: {
        // --------------------------------------------------------------------
        // BASIC FUNCTIONALITY
        //   Verify that 'find' returns correct indices for straightforward
        //   inputs.
        //
        // Concerns:
        //: 1 The returned vector has exactly two elements.
        //: 2 The values at the returned indices sum to 'target'.
        //: 3 The lower index is returned first.
        //: 4 The function works for multiple distinct inputs.
        //
        // Plan:
        //: 1 Call 'find' with known inputs and verify the size and values of
        //:   the result. (C-1, C-2, C-3, C-4)
        //
        // Testing:
        //   static std::vector<int> find(const std::vector<int>&, int);
        // --------------------------------------------------------------------

        if (verbose) std::cout << "\nBASIC FUNCTIONALITY"
                               << "\n===================" << std::endl;

        struct {
            std::vector<int> d_nums;
            int              d_target;
            int              d_expectedI;
            int              d_expectedJ;
        } DATA[] = {
            // nums              target  i  j
            { {2, 7, 11, 15},    9,      0, 1 },
            { {3, 2, 4},         6,      1, 2 },
            { {3, 3},            6,      0, 1 },
            { {1, 5, 3, 7, 2},   9,      3, 4 },
            { {0, 4, 3, 0},      0,      0, 3 },
        };
        const int NUM_DATA = sizeof DATA / sizeof *DATA;

        for (int ti = 0; ti < NUM_DATA; ++ti) {
            const std::vector<int>& nums     = DATA[ti].d_nums;
            const int               target   = DATA[ti].d_target;
            const int               expI     = DATA[ti].d_expectedI;
            const int               expJ     = DATA[ti].d_expectedJ;

            std::vector<int> result = Obj::find(nums, target);

            LOOP_ASSERT(ti, result.size() == 2);
            LOOP_ASSERT(ti, result[0] == expI);
            LOOP_ASSERT(ti, result[1] == expJ);
            LOOP_ASSERT(ti, nums[result[0]] + nums[result[1]] == target);
        }

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
