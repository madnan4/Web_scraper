#include <iostream>
#include <string>
#include <curl/curl.h>
#include <gumbo.h>
#include <fstream>
#include <algorithm>
#include <regex>
#include <sstream>
#include "fetch.h"

using namespace std;

vector<int> get_credit_hours(const string &text)
{
    // Check for 'or' case
    vector<int> range_values;
    int per = text.find(".");
    string texts = text.substr(0, per);

    if (texts.find("or") != string::npos || texts.find("OR") != string::npos)
    {
        regex number_regex(R"((\d+)\s*(?:or|OR)\s*(\d+))");
        smatch match;
        if (regex_search(texts, match, number_regex))
        {
            // cout << "Found OR case: " << match[1] << " or " << match[2] << endl;
            range_values.push_back(stoi(match[1]));
            range_values.push_back(stoi(match[2]));
            return range_values;
        }
    }

    // Check for 'to' case
    if (texts.find("to") != string::npos || texts.find("TO") != string::npos)
    {
        regex number_regex(R"((\d+)\s*(?:to|TO)\s*(\d+))");
        smatch match;
        if (regex_search(texts, match, number_regex))
        {
            int start = stoi(match[1]);
            int end = stoi(match[2]);
            for (int i = start; i <= end; ++i)
            {
                range_values.push_back(i);
            }
            // cout << "Found TO case: " << match[1] << " to " << match[2] << endl;
            return range_values;
        }
    }

    // Default case: return a single number if found
    regex number_regex(R"((\d+))");
    smatch match;
    if (regex_search(texts, match, number_regex))
    {
        range_values.push_back(stoi(match[1]));
        return range_values;
    }

    return range_values; // Return -1 if no numbers are found
}

void search_for_info(GumboNode *node, string &creditHours, vector<tuple<string, string, int>> &prerequisites, string cl)
{
    vector<int> hours;
    if (node->type != GUMBO_NODE_ELEMENT)
        return;

    GumboVector *children = &node->v.element.children;

    // Check for the "Credit" and prerequisites information
    for (size_t i = 0; i < children->length; ++i)
    {
        GumboNode *child = static_cast<GumboNode *>(children->data[i]);
        if (child->type != GUMBO_NODE_ELEMENT)
            continue;

        if (child->v.element.tag == GUMBO_TAG_P)
        {
            GumboVector *pChildren = &child->v.element.children;

            // Detecting "Credit:" line
            if (pChildren->length > 0)
            {
                GumboNode *firstChild = static_cast<GumboNode *>(pChildren->data[0]);
                if (firstChild->type == GUMBO_NODE_ELEMENT && firstChild->v.element.tag == GUMBO_TAG_B)
                {
                    // Detect "Credit:" node
                    GumboNode *textNode = static_cast<GumboNode *>(pChildren->data[1]);
                    if (textNode->type == GUMBO_NODE_TEXT)
                    {
                        string bText = firstChild->v.element.original_tag.data;
                        if (bText.find("Credit") != string::npos)
                        {
                            string textContent = textNode->v.text.text;
                            size_t pos = textContent.find("hours");
                            if (pos != string::npos)
                            {
                                hours = get_credit_hours(textContent.substr(1, pos - 1));
                                ostringstream oss;

                                // Iterate over the vector and append each element to the stream
                                for (size_t i = 0; i < hours.size(); ++i)
                                {
                                    oss << hours[i];
                                    // Add a comma after each element except the last one
                                    if (i < hours.size() - 1)
                                    {
                                        oss << ",";
                                    }
                                }
                                creditHours = oss.str();
                                // cout << creditHours << endl;
                            }
                        }
                    }
                }
            }

            // Detecting prerequisites line
            string prereqText;
            bool isPrerequisiteSection = false;
            for (size_t j = 0; j < pChildren->length; ++j)
            {
                GumboNode *pChild = static_cast<GumboNode *>(pChildren->data[j]);
                if (pChild->type == GUMBO_NODE_TEXT)
                {
                    string text = pChild->v.text.text;
                    if (text.find("Prerequisite:") != string::npos)
                    {
                        isPrerequisiteSection = true;
                    }
                    if (isPrerequisiteSection)
                    {
                        prereqText += text;
                        // Heuristic check to stop accumulation if encountering unrelated content
                        if (text.find("Restricted to majors") != string::npos || text.find("Urbana, IL") != string::npos)
                        {
                            break;
                        }
                    }
                }
                else if (isPrerequisiteSection && pChild->type == GUMBO_NODE_ELEMENT && pChild->v.element.tag == GUMBO_TAG_A)
                {
                    GumboNode *textNode = static_cast<GumboNode *>(pChild->v.element.children.data[0]);
                    if (textNode && textNode->type == GUMBO_NODE_TEXT)
                    {
                        prereqText += textNode->v.text.text;
                    }
                }
            }

            // Print the extracted prerequisite text for debugging
            // cout << "Extracted Prerequisite Text: " << prereqText << endl;

            // Extract course codes and their conditions
            regex courseRegex("([A-Z]{2,4} \\d{2,3})");
            auto words_begin = sregex_iterator(prereqText.begin(), prereqText.end(), courseRegex);
            auto words_end = sregex_iterator();

            bool nextIsConcurrent = false;
            for (sregex_iterator i = words_begin; i != words_end; ++i)
            {
                string course = (*i).str();
                int condition = -1; // Default condition

                // If the word "concurrent" appears before this course in prereqText, set nextIsConcurrent
                string::size_type pos = prereqText.find(course);
                if (pos != string::npos && prereqText.substr(0, pos).find("concurrent") != string::npos)
                {
                    nextIsConcurrent = true;
                }

                // Set the correct condition based on the nextIsConcurrent flag
                if (nextIsConcurrent)
                {
                    condition = 0;
                    nextIsConcurrent = false; // Reset after assigning
                }

                string major_part, course_number;
                istringstream iss(course);
                iss >> major_part >> course_number;
                int total_length = major_part.length() + course_number.length();
                int underscores_needed = 8 - total_length;
                if (underscores_needed < 0)
                    underscores_needed = 0;
                string formatted_target_course = major_part + string(underscores_needed, '_') + course_number;
                // cout << formatted_target_course << "\t" << cl << endl;
                prerequisites.push_back(make_tuple(formatted_target_course, cl, condition));
            }
        }
    }

    // Recursively handle other elements
    for (size_t i = 0; i < children->length; ++i)
    {
        search_for_info(static_cast<GumboNode *>(children->data[i]), creditHours, prerequisites, cl);
    }
}
vector<tuple<string, string, int>> UIUC_getContent(map<string, string> subjects, vector<tuple<string, string>> &courses, string major)
{
    vector<tuple<string, string, int>> prerequisites;
    map<string, string> x;
    map<string, string> classes;

    if (subjects.find(major) != subjects.end())
    {
        classes = content_UIUC(subjects[major], x);

        for (auto &cl : classes)
        {
            string major_part, course_number;
            istringstream target_stream(cl.first);
            target_stream >> major_part >> course_number;
            int target_length = major_part.length() + course_number.length();
            int target_underscores_needed = 8 - target_length;
            string formatted_target_course = major_part + string(target_underscores_needed > 0 ? target_underscores_needed : 0, '_') + course_number;
            string first = formatted_target_course;

            string classone = cl.second;
            // cout << cl.first << endl;
            string htmlContent = fetch_html(classone);

            GumboOutput *output = gumbo_parse(htmlContent.c_str());

            string creditHours;

            search_for_info(output->root, creditHours, prerequisites, first);

            gumbo_destroy_output(&kGumboDefaultOptions, output);
            courses.push_back(make_pair(first, creditHours));
            // cout << first << " " << creditHours << endl;
        }
        int target_underscores_needed = 8 - (major.length() + 3);
        string final_course = major + string(target_underscores_needed > 0 ? target_underscores_needed : 0, '_') + "XXX";
        courses.push_back(make_pair(final_course, "0"));
        return prerequisites;
    }
    else
    {
        cerr << "check UIUC_getContent" << endl;
        return {};
    }
    // cout << "Prerequisites: " << endl;
    // for (const auto &cl : courses)
    // {
    //     cout << get<0>(cl) << "  " << get<1>(cl) << endl;
    // }
    // cout << endl;
    // cout << "Prerequisites: ";
    // for (const auto &prereq : prerequisites)
    // {
    //     cout << get<0>(prereq) << "\t" << get<1>(prereq) << "\t" << get<2>(prereq) << endl;
    // }
    // cout << endl;
}