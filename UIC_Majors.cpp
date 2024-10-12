#include <iostream>
#include <string>
#include <map>
#include <curl/curl.h>
#include <gumbo.h>
#include <fstream>
#include "fetch.h"

// Function to recursively search the parsed HTML tree for majors
void search_for_majors(GumboNode *node, map<string, string> &majors, map<string, string> &majorList)
{
    if (node->type != GUMBO_NODE_ELEMENT)
    {
        return;
    }
    if (node->v.element.tag == GUMBO_TAG_TR)
    {
        GumboVector *children = &node->v.element.children;
        std::string code, link, description;
        for (unsigned int i = 0; i < children->length; ++i)
        {
            GumboNode *child = static_cast<GumboNode *>(children->data[i]);
            if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_TD)
            {
                GumboNode *strong_node = static_cast<GumboNode *>(child->v.element.children.data[0]);
                if (strong_node->type == GUMBO_NODE_ELEMENT && strong_node->v.element.tag == GUMBO_TAG_STRONG)
                {
                    GumboNode *a_node = static_cast<GumboNode *>(strong_node->v.element.children.data[0]);
                    if (a_node->type == GUMBO_NODE_ELEMENT && a_node->v.element.tag == GUMBO_TAG_A)
                    {
                        GumboAttribute *href = gumbo_get_attribute(&a_node->v.element.attributes, "href");
                        link = "https://webcs7.osss.uic.edu" + string(href->value);
                        code = static_cast<GumboNode *>(a_node->v.element.children.data[0])->v.text.text;
                    }
                }
                else
                {
                    description = child->v.element.children.length > 0 ? static_cast<GumboNode *>(child->v.element.children.data[0])->v.text.text : "";
                }
            }
        }
        majors[code] = link;
        majorList[code] = description;
        // std::cout << code << ", " << link << ", " << description << std::endl;
    }

    for (unsigned int i = 0; i < node->v.element.children.length; ++i)
    {
        search_for_majors(static_cast<GumboNode *>(node->v.element.children.data[i]), majors, majorList);
    }
}

// Function to fetch majors from the specified URL, save them to a file, and return them as a map
map<string, string> getMajorsAndSave(string sem, string url, string year, map<string, string> &majorList)
{
    string html_content = fetch_html(url);
    // cout << html_content << endl;

    GumboOutput *output = gumbo_parse(html_content.c_str());
    map<string, string> majors;

    search_for_majors(output->root, majors, majorList);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    string courseFolderName = "UIC/" + sem;
    string filename = "/" + sem + "majors" + ".txt";
    string filepath = courseFolderName + filename;
    ofstream outfile(filepath);
    if (outfile.is_open())
    {
        for (const auto &major : majors)
        {
            // cout << "open" << endl;
            // Copy the URL to modify it if necessary
            // outfile << "Major: " << major.first << " URL: " << major.second << endl;
            string major_url = major.second;
            // Construct the expected path for the given semester and year
            string expected_path = "/schedule-of-classes/static/schedules/" + sem + "-" + year + "/";

            // Ensure the URL is correct by adding the missing path if necessary
            if (major_url.find("https://webcs7.osss.uic.edu") != string::npos &&
                major_url.find(".html") != string::npos &&
                major_url.find(expected_path) == string::npos)
            {
                size_t insert_pos = major_url.find(".edu") + 4;
                major_url.insert(insert_pos, expected_path);
            }
            majors[major.first] = major_url;
            // Write to file
            // cout << "Major: " << major.first << " URL: " << major.second << endl;
            outfile << "Major: " << major.first << " URL: " << major.second << endl;
            // outfile << "-----------------------------------" << endl;

            // Update the map with the corrected URL
            // major.second = major_url;
        }
        outfile.close();
        cout << "Majors have been written to " << filename << endl;
    }
    else
    {
        cerr << "Unable to open file for writing" << endl;
    }

    return majors;
}