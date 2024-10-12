#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include <gumbo.h>
#include <fstream>
#include <algorithm>
#include <regex>
#include <sstream>
#include <set>
#include <tuple>
#include "printToFile.cpp"
#include "compare.h"
using namespace std;

void getMasterFiles(string major, vector<tuple<string, string>> &springList, vector<tuple<string, string>> &fallList, vector<tuple<string, string, int>> &prereqSpring, vector<tuple<string, string, int>> &prereqFall, string uni)
{
    set<tuple<string, string>, CompareTuples> finalList;
    vector<tuple<string, string, string>> courseOfferings;
    set<tuple<string, string, int>, CompareBySecondPrereq> finalPrereqs;
    // set<tuple<string, string, int>> finalPrereqs;

    map<string, pair<string, string>> courseMap;

    // Process springList
    for (const auto &course : springList)
    {
        finalList.insert(course);
        string courseName = get<0>(course);
        courseMap[courseName].second = "1";
    }

    // Process fallList
    for (const auto &course : fallList)
    {
        finalList.insert(course);
        string courseName = get<0>(course);
        courseMap[courseName].first = "1";
    }

    // Process prereqSpring
    for (const auto &prereq : prereqSpring)
    {
        finalPrereqs.insert(prereq);
    }

    // Process prereqFall
    for (const auto &prereq : prereqFall)
    {
        finalPrereqs.insert(prereq);
    }

    // get course offerings
    for (const auto &entry : courseMap)
    {
        const string &courseName = entry.first;
        const string &springOffer = entry.second.second;
        const string &fallOffer = entry.second.first;

        if (springOffer == "1" && fallOffer.empty())
        {
            courseOfferings.emplace_back(courseName, "0", "1");
        }
        else if (springOffer.empty() && fallOffer == "1")
        {
            courseOfferings.emplace_back(courseName, "1", "0");
        }
    }

    printToFile(major, finalList, finalPrereqs, courseOfferings, uni);
    // cout << "Final List:" << endl;
    // for (const auto &course : finalList)
    // {
    //     cout << get<0>(course) << "\t" << get<1>(course) << endl;
    // }

    // cout << "\nCourse Offerings (Only in one semester):" << endl;
    // for (const auto &course : courseOfferings)
    // {
    //     cout << get<0>(course) <<"\t"
    //          << get<1>(course) <<"\t"
    //          << get<2>(course) << endl;
    // }

    // cout << "\nFinal Prerequisites:" << endl;
    // for (const auto &prereq : finalPrereqs)
    // {
    //     cout << get<0>(prereq) << "\t"
    //          << get<1>(prereq) << "\t"
    //          << get<2>(prereq) << endl;
    // }
}