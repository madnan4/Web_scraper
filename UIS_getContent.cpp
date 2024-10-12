#include <iostream>
#include <string>
#include <curl/curl.h>
#include <gumbo.h>
#include <fstream>
#include <algorithm>
#include <regex>
#include <sstream>
#include <set>
#include "fetch.h"

using namespace std;

// Function to remove punctuation
string removePunctuation(const string &str)
{
    string result;
    remove_copy_if(str.begin(), str.end(), back_inserter(result), [](unsigned char c)
                   { return ispunct(c); });
    return result;
}

// Function to trim leading and trailing white spaces
string trim(const string &str)
{
    auto start = str.begin();
    while (start != str.end() && isspace(*start))
    {
        start++;
    }

    auto end = str.end();
    do
    {
        end--;
    } while (distance(start, end) > 0 && isspace(*end));

    return string(start, end + 1);
}
// Function to replace non-breaking spaces with regular spaces
void replaceNonBreakingSpaces(string &str)
{
    replace(str.begin(), str.end(), static_cast<char>(0xA0), 'A');
}

bool areLetters(const string &str)
{
    if (str.length() < 3)
    {
        return false; // Not enough characters
    }
    return isalpha(str[0]) && isalpha(str[1]) && isalpha(str[2]);
}

string getTexts(GumboNode *node)
{
    if (node->type == GUMBO_NODE_TEXT)
    {
        return string(node->v.text.text);
    }
    else if (node->type == GUMBO_NODE_ELEMENT && node->v.element.tag != GUMBO_TAG_SCRIPT && node->v.element.tag != GUMBO_TAG_STYLE)
    {
        string contents = "";
        GumboVector *children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i)
        {
            const string text = getTexts(static_cast<GumboNode *>(children->data[i]));
            if (!text.empty())
            {
                if (!contents.empty())
                {
                    contents.append(" ");
                }
                contents.append(text);
            }
        }
        return contents;
    }
    else
    {
        return "";
    }
}

string getHours(string text)
{
    string creditHours;
    if (text.size() == 1 || text.size() == 2)
    {
        creditHours = text;
        return creditHours;
    }
    if (text.find("-") != string::npos)
    {
        size_t pos = text.find("-");
        int start = stoi(text.substr(0, pos));
        int end = stoi(text.substr(pos + 1));
        for (int i = start; i < end; i++)
        {
            creditHours += to_string(i);
            if (i < end - 1)
            {
                creditHours += ",";
            }
        }
        return creditHours;
    }
    if (text.find(",") != string::npos)
    {
        creditHours = text;
        return creditHours;
    }
    return "";
}

// Function to find the course information
void findCourseInfo(GumboNode *node, vector<tuple<string, string>> &courses, string major, set<tuple<string, string, int>> &pre)
{
    if (node->type != GUMBO_NODE_ELEMENT)
    {
        return;
    }

    GumboAttribute *classAttr = gumbo_get_attribute(&node->v.element.attributes, "class");
    if (classAttr && string(classAttr->value) == "courseblock")
    {
        string courseTitle = getTexts(node);
        string formatted_course_name;

        // get first line
        size_t nl = courseTitle.find("\n");
        string info = courseTitle.substr(0, nl - 7);

        // get hours
        size_t hoursPos = info.rfind('.');
        string hours = info.substr(hoursPos + 1);
        hours = trim(hours);
        // cout << hours << endl;
        string hoursList = getHours(hours);

        // get name
        size_t namePos = info.find(".");
        string name = info.substr(0, namePos);
        string major_part = name.substr(0, 3);
        string courseNum = name.substr(namePos - 3);

        // Create the formatted course name
        formatted_course_name = major_part + string(2, '_') + courseNum;

        // cout << formatted_course_name << "\t" << hoursList << endl;
        if (!hoursList.empty())
        {
            courses.emplace_back(formatted_course_name, hoursList);
        }
        else
        {
            // Extract course code and hours
            regex courseRegex(R"((\w+\s*\d+)\.\s*.*?(\d+)\s*Hours)");
            smatch courseMatch;
            if (regex_search(courseTitle, courseMatch, courseRegex))
            {

                string creditHours = courseMatch[2].str();
                // cout << creditHours << endl;
                courses.emplace_back(formatted_course_name, creditHours);
            }
        }
        // Extract prerequisites
        regex prereqRegex(R"(Prerequisites?:\s*(.*))");
        smatch prereqMatch;
        if (regex_search(courseTitle, prereqMatch, prereqRegex))
        {
            string prereqs = prereqMatch[1].str();
            // cout << prereqs << endl;
            regex prereqSplitRegex(R"(\s*(?:\.|and)\s*)");
            sregex_token_iterator iter(prereqs.begin(), prereqs.end(), prereqSplitRegex, -1);
            sregex_token_iterator end;
            bool majorname = false;
            // cout << "Prerequisites: " << endl;
            regex courseCodeRegex(R"((\w+)\s*(\d+))");
            for (; iter != end; ++iter)
            {
                string prereq = iter->str();
                prereq = removePunctuation(prereq);
                prereq = trim(prereq);
                bool concurrent = prereq.find("concurrent") != string::npos || prereq.find("Corequisite") != string::npos;

                vector<string> words;

                // Using istringstream to break the string into words
                istringstream iss(prereq);
                string word;
                while (iss >> word)
                {
                    if (word == major)
                    {
                        majorname = true;
                    }
                    // cout << word << endl;
                    smatch courseCodeMatch;
                    if (regex_search(word, courseCodeMatch, courseCodeRegex))
                    {
                        // cout << "here" << endl;
                        string courseNumber = courseCodeMatch[0].str(); // Extract course number
                        courseNumber = trim(courseNumber);
                        // cout << "course Name " << formatted_course_name << endl;
                        // cout << "Major: " << word.substr(0, 3) << endl;
                        // cout << "Course Number:" << courseNumber << endl;
                        string formatted_pre_req = word.substr(0, 3) + "__" + courseNumber;
                        if (concurrent)
                        {
                            if (areLetters(formatted_pre_req) && areLetters(formatted_course_name) && formatted_pre_req.size() == 8 && formatted_course_name.size() == 8)
                            {
                                pre.emplace(make_tuple(formatted_pre_req, formatted_course_name, 0));
                            }
                            // cout << "relation:  0" << endl;
                        }
                        else
                        {
                            if (areLetters(formatted_pre_req) && areLetters(formatted_course_name) && formatted_pre_req.size() == 8 && formatted_course_name.size() == 8)
                            {
                                pre.emplace(make_tuple(formatted_pre_req, formatted_course_name, -1));
                            }
                            // cout << "relation:  -1" << endl;
                        }
                        if (majorname)
                        {
                            formatted_pre_req = major + "__" + courseNumber;
                            if (concurrent)
                            {
                                if (areLetters(formatted_pre_req) && areLetters(formatted_course_name) && formatted_pre_req.size() == 8 && formatted_course_name.size() == 8)
                                {
                                    pre.emplace(make_tuple(formatted_pre_req, formatted_course_name, 0));
                                }
                                // cout << "relation:  0" << endl;
                            }
                            else
                            {
                                if (areLetters(formatted_pre_req) && areLetters(formatted_course_name) && formatted_pre_req.size() == 8 && formatted_course_name.size() == 8)
                                {
                                    pre.emplace(make_tuple(formatted_pre_req, formatted_course_name, -1));
                                }
                                // cout << "relation:  -1" << endl;
                            }
                            majorname = false;
                        }
                    }
                }
            }
        }

        // Extract coreqs
        regex coreqsRegex(R"(Co-requisites?:\s*(.*))");
        smatch coreqsMatch;
        if (regex_search(courseTitle, coreqsMatch, coreqsRegex))
        {
            string coreqs = coreqsMatch[1].str();
            regex coreqSplitRegex(R"(\s*(?:\.|and)\s*)");
            sregex_token_iterator iter(coreqs.begin(), coreqs.end(), coreqSplitRegex, -1);
            sregex_token_iterator end;
            bool prereq;
            regex courseCodeRegex(R"((\w+)\s*(\d+))");
            for (; iter != end; ++iter)
            {
                string coreq = iter->str();
                coreq = removePunctuation(coreq);
                coreq = trim(coreq);
                prereq = coreq.find("Prerequisites") != string::npos;

                vector<string> words;

                // Using istringstream to break the string into words
                istringstream iss(coreq);
                string word;
                while (iss >> word)
                {
                    smatch courseCodeMatch;
                    if (regex_search(word, courseCodeMatch, courseCodeRegex))
                    {
                        string courseNumber = courseCodeMatch[0].str(); // Extract course number
                        courseNumber = trim(courseNumber);
                        string formatted_co_req = word.substr(0, 3) + "__" + courseNumber;
                        if (!prereq)
                        {
                            if (areLetters(formatted_co_req) && areLetters(formatted_course_name) && formatted_co_req.size() == 8 && formatted_course_name.size() == 8)
                            {
                                pre.emplace(make_tuple(formatted_co_req, formatted_course_name, 0));
                            }
                        }
                    }
                }
            }
        }
    }

    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        findCourseInfo(static_cast<GumboNode *>(children->data[i]), courses, major, pre);
    }
}

void filterSet(set<tuple<string, string, int>> &pre)
{
    set<tuple<string, string, int>> toAdd;
    set<tuple<string, string, int>> toRemove;
    map<pair<string, string>, tuple<string, string, int>> seen;

    for (const auto &t : pre)
    {
        string str1 = get<0>(t);
        string str2 = get<1>(t);
        int num = get<2>(t);

        auto key = make_pair(str1, str2);

        if (seen.find(key) == seen.end())
        {
            // First time seeing this key
            seen[key] = t;
        }
        else
        {
            // Compare with the existing tuple
            if (num == 0)
            {
                // Replace with the tuple having 0 as the integer
                toRemove.insert(seen[key]);
                seen[key] = t;
            }
        }
    }

    // Insert the final filtered tuples
    for (const auto &[key, value] : seen)
    {
        toAdd.insert(value);
    }

    // Perform the removal
    for (const auto &t : toRemove)
    {
        pre.erase(t);
    }

    // Perform the addition
    for (const auto &t : toAdd)
    {
        pre.insert(t);
    }
}

vector<tuple<string, string, int>> UISGetContent(map<string, string> subjects, vector<tuple<string, string>> &courses, string major)
{
    vector<tuple<string, string, int>> preFinal;
    set<tuple<string, string, int>> pre;
    string majorU = major;
    transform(majorU.begin(), majorU.end(), majorU.begin(), ::toupper);
    if (subjects.find(major) != subjects.end())
    {
        string html_content = fetch_html(subjects[major]);

        GumboOutput *output = gumbo_parse(html_content.c_str());
        findCourseInfo(output->root, courses, majorU, pre);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    string finalCourse = major + "__XXX";
    courses.emplace_back(make_tuple(finalCourse, "0"));
    filterSet(pre);
    for (auto course : pre)
    {
        preFinal.push_back(make_tuple(get<0>(course), get<1>(course), get<2>(course)));
        // cout << get<0>(course) << "\t" << get<1>(course) << "\t" << get<2>(course) << endl;
    }
    return preFinal;
}