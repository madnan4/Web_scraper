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
#include <filesystem>
#include "fetch.h"

using namespace std;
namespace fs = std::__fs::filesystem;

// Function to extract the first number from a string (credit hours)
vector<int> extract_credit_hours(const string &text)
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

// Function to recursively search the parsed HTML tree for <h2> tags and their associated <p> tags
void search_for_h2_tags(GumboNode *node, vector<tuple<string, string>> &courses, vector<tuple<string, string, int>> &prerequisites, vector<tuple<string, string, int>> &prerequisitesfinal)
{
    vector<tuple<string, string, int>> prerequisitesFinal;
    if (node->type != GUMBO_NODE_ELEMENT)
        return;
    if (node->v.element.tag == GUMBO_TAG_H2)
    {
        if (node->v.element.children.length > 0)
        {
            GumboNode *text_node = static_cast<GumboNode *>(node->v.element.children.data[0]);
            if (text_node->type == GUMBO_NODE_TEXT)
            {
                string course_name = text_node->v.text.text;
                vector<int> credit_hours;
                string prerequisite_text;

                // Find the <p> tag that is the sibling of the current <h2> tag
                GumboNode *parent = node->parent;
                if (parent->type == GUMBO_NODE_ELEMENT)
                {
                    bool found_current = false;
                    for (unsigned int i = 0; i < parent->v.element.children.length; ++i)
                    {
                        GumboNode *sibling = static_cast<GumboNode *>(parent->v.element.children.data[i]);
                        if (sibling == node)
                        {
                            found_current = true;
                        }
                        else if (found_current && sibling->type == GUMBO_NODE_ELEMENT && sibling->v.element.tag == GUMBO_TAG_P)
                        {
                            if (sibling->v.element.children.length > 0)
                            {
                                GumboNode *p_text_node = static_cast<GumboNode *>(sibling->v.element.children.data[0]);
                                if (p_text_node->type == GUMBO_NODE_TEXT)
                                {
                                    string p_text = p_text_node->v.text.text;
                                    // Extract credit hours and prerequisites from <p> text
                                    credit_hours = extract_credit_hours(p_text);
                                    size_t prereq_pos = p_text.find("Prerequisite");
                                    if (prereq_pos != string::npos)
                                    {
                                        prerequisite_text = p_text.substr(prereq_pos + 12); // 17 to skip "Prerequisite(s):"
                                        size_t period_pos = prerequisite_text.find('.');
                                        if (period_pos != string::npos)
                                        {
                                            prerequisite_text = prerequisite_text.substr(0, period_pos);
                                        }
                                    }
                                }
                            }
                            break; // Found the <p> tag, no need to continue
                        }
                    }
                }
                ostringstream oss;

                // Iterate over the vector and append each element to the stream
                for (size_t i = 0; i < credit_hours.size(); ++i)
                {
                    oss << credit_hours[i];
                    // Add a comma after each element except the last one
                    if (i < credit_hours.size() - 1)
                    {
                        oss << ",";
                    }
                }

                string major_part, course_number;
                istringstream iss(course_name);
                iss >> major_part >> course_number;
                // Calculate the number of underscores needed
                int total_length = major_part.length() + course_number.length();
                int underscores_needed = 8 - total_length;
                if (underscores_needed < 0)
                    underscores_needed = 0;

                // Create the formatted course name
                string courseName = major_part + string(underscores_needed, '_') + course_number;
                cout << courseName << "\t" << oss.str() << endl;
                courses.push_back(make_pair(courseName, oss.str()));
                // courses.push_back(make_pair(course_name, credit_hours));

                // Process prerequisites if present
                if (!prerequisite_text.empty())
                {
                    vector<string> prereq_courses;
                    regex prereq_regex("([A-Z]+ \\d+)");
                    smatch prereq_match;
                    string::const_iterator search_start(prerequisite_text.cbegin());
                    string::const_iterator prev_match_end = prerequisite_text.cbegin();

                    while (regex_search(search_start, prerequisite_text.cend(), prereq_match, prereq_regex))
                    {
                        // Extract the matched course
                        string matched_course = prereq_match[0];

                        // Check the segment between prev_match_end and search_start
                        string segment(prev_match_end, prereq_match[0].first);
                        bool concurrent_found = segment.find("concurrent") != string::npos;
                        bool credit_or_concurrent_found = segment.find("Credit or concurrent") != string::npos;

                        // Extract major and course number
                        string prereq_major, prereq_number;
                        istringstream prereq_stream(matched_course);
                        prereq_stream >> prereq_major >> prereq_number;

                        // Format the prerequisite course name to be 8 characters long
                        int prereq_length = prereq_major.length() + prereq_number.length();
                        int prereq_underscores_needed = 8 - prereq_length;
                        string prereq_formatted = prereq_major + string(prereq_underscores_needed > 0 ? prereq_underscores_needed : 0, '_') + prereq_number;

                        // Format the target course name to be 8 characters long
                        string major_part, course_number;
                        istringstream target_stream(course_name);
                        target_stream >> major_part >> course_number;
                        int target_length = major_part.length() + course_number.length();
                        int target_underscores_needed = 8 - target_length;
                        string formatted_target_course = major_part + string(target_underscores_needed > 0 ? target_underscores_needed : 0, '_') + course_number;

                        // Determine the relation and store the prerequisite
                        if (credit_or_concurrent_found)
                        {
                            // Add the course twice: once with relation 0 and once with -1
                            prerequisites.emplace_back(prereq_formatted, formatted_target_course, 0);
                        }
                        else
                        {
                            int relation = concurrent_found ? 0 : -1;
                            prerequisites.emplace_back(prereq_formatted, formatted_target_course, relation);
                        }

                        // Update the search_start and prev_match_end for the next iteration
                        prev_match_end = prereq_match.suffix().first;
                        search_start = prev_match_end;
                    }
                    if (prerequisite_text.find("Must be taken in the student's last semester of study") != string::npos)
                    {
                        // Define the formatted target course as "MajorName_XXX"
                        string major_part, course_number;
                        istringstream target_stream(course_name);
                        target_stream >> major_part >> course_number;
                        int target_length = major_part.length() + course_number.length();
                        int target_underscores_needed = 8 - target_length;
                        string formatted_target_course = major_part + string(target_underscores_needed > 0 ? target_underscores_needed : 0, '_') + course_number;

                        int lastCourseLen = major_part.length() + 3;
                        int targetneeded = 8 - lastCourseLen;
                        string courseName = major_part + string(targetneeded > 0 ? targetneeded : 0, '_') + "XXX";
                        prerequisitesfinal.emplace_back(formatted_target_course, courseName, -1);
                        prerequisites.emplace_back(courseName, formatted_target_course, 2);
                    }
                }
            }
        }
    }

    for (unsigned int i = 0; i < node->v.element.children.length; ++i)
    {
        search_for_h2_tags(static_cast<GumboNode *>(node->v.element.children.data[i]), courses, prerequisites, prerequisitesfinal);
    }
}

void getClasses(string sem, map<string, string> majors, vector<tuple<string, string, int>> &prerequisites, vector<tuple<string, string>> &courseList, string major)
{

    string courseFolderName = "UIC/" + sem;
    string filename = "/" + sem + "majors" + ".txt";
    string filepath = courseFolderName + filename;

    string major_name = major;

    string majorU = major_name;
    string majorL = major_name;
    std::transform(majorU.begin(), majorU.end(), majorU.begin(), ::toupper);
    std::transform(majorL.begin(), majorL.end(), majorL.begin(), ::tolower);

    if (majors.find(majorU) != majors.end())
    {
        string major_url = majors[majorU];
        string major_html_content = fetch_html(major_url);
        if (major_html_content.empty())
        {
            cerr << "Error: Fetched HTML content is empty." << endl;
        }
        else
        {

            GumboOutput *major_output = gumbo_parse(major_html_content.c_str());
            vector<pair<string, int>> courses;
            vector<tuple<string, string, int>> prerequisitesfinal;
            search_for_h2_tags(major_output->root, courseList, prerequisites, prerequisitesfinal);

            if (!prerequisitesfinal.empty())
            {
                for (const auto &prereq : prerequisitesfinal)
                {
                    prerequisites.push_back(prereq);
                }
            }

            gumbo_destroy_output(&kGumboDefaultOptions, major_output);

            string major_part = majorU, course_number = "XXX";

            // Calculate the number of underscores needed
            int total_length = major_part.length() + course_number.length();
            int underscores_needed = 8 - total_length;
            if (underscores_needed < 0)
                underscores_needed = 0;

            // Create the formatted course name
            string formatted_course_name = major_part + string(underscores_needed, '_') + course_number;

            // Write the formatted course name and credit hours to the file
            courseList.emplace_back(formatted_course_name, "0");
        }
    }
    else
    {
        cerr << "Major not found." << endl;
    }
}