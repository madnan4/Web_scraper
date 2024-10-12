// CompareBySecondPrereq.h

#ifndef COMPARE_H
#define COMPARE_H

#include <tuple>
#include <string>

using namespace std;

// Comparator to sort tuples by the second element in ascending order for prerequisites
struct CompareBySecondPrereq
{
    bool operator()(const tuple<string, string, int> &a, const tuple<string, string, int> &b) const;
};

// Custom comparator function
struct CompareTuples
{
    bool operator()(const tuple<string, string> &a, const tuple<string, string> &b) const
    {
        // Extract the last three characters of the first string in each tuple
        string a_num = get<0>(a).substr(get<0>(a).length() - 3);
        string b_num = get<0>(b).substr(get<0>(b).length() - 3);

        // First, compare by the last three characters
        if (a_num != b_num)
        {
            return a_num < b_num;
        }

        // If they are the same, compare by the full string to ensure uniqueness
        return get<0>(a) < get<0>(b);
    }
};

#endif