#include <iostream>
#include <string>
#include <map>
#include <curl/curl.h>
#include <gumbo.h>
#include <fstream>
#include "fetch.h"

using namespace std;

void search_for_courses(GumboNode *node, map<string, string> &majors, map<string, string> &majorList)
{
    if (node->type != GUMBO_NODE_ELEMENT)
        return;

    // Check if the node is a <ul> element
    if (node->v.element.tag == GUMBO_TAG_UL)
    {
        GumboVector *children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i)
        {
            GumboNode *child = static_cast<GumboNode *>(children->data[i]);
            // Check if the child node is a <li> element
            if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_LI)
            {
                GumboNode *a_tag = nullptr;
                for (unsigned int j = 0; j < child->v.element.children.length; ++j)
                {
                    GumboNode *li_child = static_cast<GumboNode *>(child->v.element.children.data[j]);
                    if (li_child->type == GUMBO_NODE_ELEMENT && li_child->v.element.tag == GUMBO_TAG_A)
                    {
                        a_tag = li_child;
                        break;
                    }
                }

                if (a_tag)
                {
                    GumboAttribute *href = gumbo_get_attribute(&a_tag->v.element.attributes, "href");
                    if (href)
                    {
                        // Extract the text and href attributes
                        string href_value = href->value;
                        if (href_value.find("coursedescriptions/") != string::npos)
                        {
                            GumboNode *text_node = static_cast<GumboNode *>(a_tag->v.element.children.data[0]);
                            if (text_node && text_node->type == GUMBO_NODE_TEXT)
                            {
                                string text = text_node->v.text.text;
                                size_t pos = text.find('(');
                                if (pos != string::npos)
                                {
                                    string code = text.substr(0, pos);
                                    string name = text.substr(pos + 1);
                                    // Remove parentheses from the code
                                    code.erase(remove(code.begin(), code.end(), '('), code.end());
                                    name.erase(remove(name.begin(), name.end(), ')'), name.end());
                                    majors[name] = "https://catalog.uis.edu" + href_value;
                                    majorList[name] = code;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Recursively search through all children nodes
    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        search_for_courses(static_cast<GumboNode *>(children->data[i]), majors, majorList);
    }
}

map<string, string> UISMajorContent(string url, map<string, string> &majorList)
{
    string html_content = fetch_html(url);

    // ofstream outfile("UIS_Html.txt");
    // if (outfile.is_open())
    // {
    //     outfile << html_content;
    //     outfile.close();
    // }
    // else
    // {
    //     cerr << "Unable to open file for writing" << endl;
    // }
    map<string, string> majors;

    // Parse the HTML content
    GumboOutput *output = gumbo_parse(html_content.c_str());
    // Search for courses in the parsed HTML
    search_for_courses(output->root, majors, majorList);
    // Clean up Gumbo output
    gumbo_destroy_output(&kGumboDefaultOptions, output);

    return majors;
}
