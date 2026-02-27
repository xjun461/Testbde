// mypalg_twosum.cpp                                                  -*-C++-*-
#include <mypalg_twosum.h>

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace BloombergLP {
namespace mypalg {

                              // ------------
                              // class TwoSum
                              // ------------

// CLASS METHODS
std::vector<int> TwoSum::find(const std::vector<int>& nums, int target)
{
    std::unordered_map<int, int> seen;  // value -> index

    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        int complement = target - nums[i];

        std::unordered_map<int, int>::iterator it = seen.find(complement);
        if (it != seen.end()) {
            return {it->second, i};
        }
        seen[nums[i]] = i;
    }

    throw std::invalid_argument("No two-sum solution found");
}

}  // close package namespace
}  // close enterprise namespace
