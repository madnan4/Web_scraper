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
#include "compare.h"

using namespace std;

void printToFile(string major, set<tuple<string, string>, CompareTuples> &finalList, set<tuple<string, string, int>, CompareBySecondPrereq> &finalPrereqs, vector<tuple<string, string, string>> &courseOfferings, string uni)
{
    string majorL = major;
    transform(majorL.begin(), majorL.end(), majorL.begin(), ::tolower);

    string courseFolderName = "masterFiles/";

    string filepath = courseFolderName + uni + "/mastercourselist_" + majorL + ".txt";
    ofstream outfile(filepath);
    if (outfile.is_open())
    {
        for (const auto &course : finalList)
        {
            outfile << get<0>(course) << "\t" << get<1>(course) << endl;
        }
        outfile.close();
        cout << "mastercourselist for " << major << " have been written to mastercourselist_" << majorL << ".txt" << endl;
    }
    else
    {
        cerr << "Unable to open " + filepath + " for writing" << endl;
    }

    filepath = courseFolderName + uni + "/prerequisites_" + majorL + ".txt";
    ofstream out(filepath);
    if (out.is_open())
    {
        for (const auto &prereq : finalPrereqs)
        {
            out << get<0>(prereq) << "\t"
                << get<1>(prereq) << "\t"
                << get<2>(prereq) << endl;
        }
        out.close();
        cout << "Prerequisites for " << major << " have been written to prerequisites_" << majorL << ".txt" << endl;
    }
    else
    {
        cerr << "Unable to open " + filepath + " for writing" << endl;
    }

    filepath = courseFolderName + uni + "/courseofferings_" + majorL + ".txt";
    ofstream outs(filepath);
    if (outs.is_open())
    {
        for (const auto &course : courseOfferings)
        {
            outs << get<0>(course) << "\t"
                 << get<1>(course) << "\t"
                 << get<2>(course) << endl;
        }
        outs.close();
        cout << "courseofferings for " << major << " have been written to courseofferings_" << majorL << ".txt" << endl;
    }
    else
    {
        cerr << "Unable to open " + filepath + " for writing" << endl;
    }
}