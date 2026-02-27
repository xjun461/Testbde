// mypalg_twosum.h                                                    -*-C++-*-
#ifndef INCLUDED_MYPALG_TWOSUM
#define INCLUDED_MYPALG_TWOSUM

//@PURPOSE: Provide a utility for finding two indices that sum to a target.
//
//@CLASSES:
//  mypalg::TwoSum: utility class for the two-sum problem
//
//@DESCRIPTION: This component provides a single utility class, 'mypalg::TwoSum',
// that solves the classic "two sum" problem: given an array of integers and a
// target value, find the indices of the two numbers that add up to the target.
//
// The implementation uses an unordered hash map to achieve O(n) time complexity
// and O(n) auxiliary space.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Finding Indices That Sum to a Target
///- - - - - - - - - - - - - - - - - - - - - - - -
// Suppose we have an array of integers and wish to find the two indices whose
// values sum to a given target.
//
//..
//  bsl::vector<int> nums = {2, 7, 11, 15};
//  int              target = 9;
//
//  bsl::vector<int> result = mypalg::TwoSum::find(nums, target);
//  assert(result[0] == 0);
//  assert(result[1] == 1);
//..

#include <unordered_map>
#include <vector>
#include <stdexcept>

namespace BloombergLP {
namespace mypalg {

                              // ============
                              // class TwoSum
                              // ============

struct TwoSum {
    // This utility struct provides a class method for solving the two-sum
    // problem.  Given a vector of integers and a target sum, 'find' returns
    // the indices of the exactly two elements that add up to 'target'.
    //
    // Preconditions:
    //: o Exactly one valid solution exists in 'nums'.
    //: o The same element may not be used twice.

    // CLASS METHODS
    static std::vector<int> find(const std::vector<int>& nums, int target);
        // Return a vector containing the two indices 'i' and 'j' (where
        // 'i < j') such that 'nums[i] + nums[j] == target'.  The behavior is
        // undefined unless exactly one such pair exists within 'nums'.  Throw
        // 'std::invalid_argument' if no solution is found.
};

}  // close package namespace
}  // close enterprise namespace

#endif  // INCLUDED_MYPALG_TWOSUM
