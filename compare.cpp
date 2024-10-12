// CompareBySecondPrereq.cpp

#include "compare.h"

bool CompareBySecondPrereq::operator()(const tuple<string, string, int> &a, const tuple<string, string, int> &b) const
{
    if (get<1>(a) == get<1>(b))
    {
        return get<0>(a) < get<0>(b); // Compare by the first element (Course) if the second elements are the same
    }
    return get<1>(a) < get<1>(b);
}