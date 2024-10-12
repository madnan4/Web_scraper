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
#include "UIC_getContent.cpp"
#include "UIC_Majors.cpp"
#include "masterFiles.cpp"
#include "UIUC_Majors.cpp"
#include "UIUC_getContent.cpp"
#include "UIS_Majors.cpp"
#include "UIS_getContent.cpp"
using namespace std;

int main()
{
    cout << "Which Uni do you want to scrape, UIC, UIUC, UIS" << endl;
    string input;
    cin >> input;
    transform(input.begin(), input.end(), input.begin(), ::toupper);
    set<string> uniqueMajors;
    map<string, string> springMajors;
    map<string, string> fallMajors;

    map<string, string> majorList;

    vector<tuple<string, string, int>> prerequisitesFall;
    vector<tuple<string, string>> courseListFall;

    vector<tuple<string, string, int>> prerequisitesSpring;
    vector<tuple<string, string>> courseListSpring;
    if (input == "UIC")
    {
        string fallurl = "https://webcs7.osss.uic.edu/schedule-of-classes/static/schedules/fall-2024/index.html";
        fallMajors = getMajorsAndSave("fall", fallurl, "2024", majorList);

        string springurl = "https://webcs7.osss.uic.edu/schedule-of-classes/static/schedules/spring-2024/index.html";
        springMajors = getMajorsAndSave("spring", springurl, "2024", majorList);

        // Insert majors from fallMajors into the set
        for (const auto &entry : fallMajors)
        {
            // cout << entry.first << " " << entry.second << endl;
            uniqueMajors.insert(entry.first);
        }
        // Insert majors from springMajors into the set
        for (const auto &entry : springMajors)
        {
            uniqueMajors.insert(entry.first);
        }

        // for (const auto &entry : fallmajorList)
        // {
        //     pair<string, string> x = make_pair(entry.first, entry.second);
        //     uniqueMajorsList.insert(x);
        // }
        // // Insert majors from springMajors into the set
        // for (const auto &entry : springmajorList)
        // {
        //     pair<string, string> x = make_pair(entry.first, entry.second);
        //     uniqueMajorsList.insert(x);
        // }
        // cout << majorList.size() << endl;
        // cout << uniqueMajors.size() << endl;
        string filename = "masterFiles/majorlist_uic.txt";
        ofstream outfile(filename);
        if (outfile.is_open())
        {
            for (auto entry : majorList)
            {
                string key = entry.first;
                if (key.length() <= 2)
                {
                    key += "  ";
                }
                if (key.length() == 3)
                {
                    key += " ";
                }
                outfile << key << "\t" << entry.second << endl;
            }
            outfile.close();
        }
        else
        {
            cerr << "Unable to open file for writing" << endl;
        }
    }
    else if (input == "UIUC")
    {
        string fallurlUIUC = "https://courses.illinois.edu/schedule/2024/fall";
        string springurlUIUC = "https://courses.illinois.edu/schedule/2024/spring";

        fallMajors = content_UIUC(fallurlUIUC, majorList);
        for (const auto &entry : fallMajors)
        {
            // cout << entry.first << " " << entry.second << endl;
            uniqueMajors.insert(entry.first);
        }
        // cout << cs << endl;
        springMajors = content_UIUC(springurlUIUC, majorList);

        for (const auto &entry : springMajors)
        {
            // cout << entry.first << " " << entry.second << endl;
            uniqueMajors.insert(entry.first);
        }
        string filename = "masterFiles/majorlist_uiuc.txt";
        ofstream outfile(filename);
        if (outfile.is_open())
        {
            for (auto entry : majorList)
            {
                string key = entry.first;
                if (key.length() <= 2)
                {
                    key += "  ";
                }
                if (key.length() == 3)
                {
                    key += " ";
                }
                outfile << key << "\t" << entry.second << endl;
            }
            outfile.close();
        }
        else
        {
            cerr << "Unable to open file for writing" << endl;
        }

        // cout << majorList.size() << endl;
        // cout << uniqueMajors.size() << endl;
    }
    else if (input == "UIS")
    {
        string fallUrl = "https://catalog.uis.edu/coursedescriptions/";
        fallMajors = UISMajorContent(fallUrl, majorList);

        string filename = "masterFiles/majorlist_uis.txt";
        ofstream outfile(filename);
        if (outfile.is_open())
        {
            for (auto entry : majorList)
            {
                string key = entry.first;
                if (key.length() <= 2)
                {
                    key += "  ";
                }
                if (key.length() == 3)
                {
                    key += " ";
                }
                outfile << key << "\t" << entry.second << endl;
            }
            outfile.close();
        }
        else
        {
            cerr << "Unable to open file for writing" << endl;
        }
        for (const auto &entry : fallMajors)
        {
            // cout << entry.first << " " << entry.second << endl;
            uniqueMajors.insert(entry.first);
        }
    }
    else
    {
        cout << "choose a uni" << endl;
    }

    int count = 0;
    for (const auto &major : uniqueMajors)
    {
        cout << "-------------- " << count << " " << major << " --------------" << endl;
        if (input == "UIC")
        {
            getClasses("fall", fallMajors, prerequisitesFall, courseListFall, major);
            getClasses("spring", springMajors, prerequisitesSpring, courseListSpring, major);
            getMasterFiles(major, courseListSpring, courseListFall, prerequisitesSpring, prerequisitesFall, "UIC");
        }

        else if (input == "UIUC")
        {
            prerequisitesFall = UIUC_getContent(fallMajors, courseListFall, major);
            prerequisitesSpring = UIUC_getContent(springMajors, courseListSpring, major);
            getMasterFiles(major, courseListSpring, courseListFall, prerequisitesSpring, prerequisitesFall, "UIUC");
        }
        else if (input == "UIS")
        {
            prerequisitesFall = UISGetContent(fallMajors, courseListFall, major);
            getMasterFiles(major, courseListFall, courseListFall, prerequisitesFall, prerequisitesFall, "UIS");
        }
        prerequisitesFall.clear();
        courseListFall.clear();

        prerequisitesSpring.clear();
        courseListSpring.clear();
        count++;
    }
    cout << "-------------------DONE-------------------" << endl;
}
