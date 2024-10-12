#include <iostream>
#include <string>
#include <curl/curl.h>
#include <gumbo.h>
#include <fstream>
#include <map>
#include "fetch.h"

using namespace std;

// Function to extract text from a GumboNode
string getText(GumboNode *node)
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
            const string text = getText(static_cast<GumboNode *>(children->data[i]));
            if (!text.empty())
            {
                if (!contents.empty())
                {
                    contents += " ";
                }
                contents += text;
            }
        }
        return contents;
    }
    else
    {
        return "";
    }
}

// Function to extract subject codes and links
void extractSubjects(GumboNode *node, map<string, string> &subjects, map<string, string> &majorList)
{
    if (node->type != GUMBO_NODE_ELEMENT)
    {
        return;
    }

    if (node->v.element.tag == GUMBO_TAG_TR)
    {
        GumboVector *children = &node->v.element.children;
        int td_count = 0;
        GumboNode *codeNode = nullptr;
        GumboNode *linkNode = nullptr;

        for (unsigned int i = 0; i < children->length; ++i)
        {
            GumboNode *child = static_cast<GumboNode *>(children->data[i]);
            if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_TD)
            {
                ++td_count;
                if (td_count == 1)
                {
                    codeNode = child;
                }
                else if (td_count == 2)
                {
                    linkNode = child;
                }
            }
        }

        if (td_count == 2)
        {
            string subjectCode = getText(codeNode);
            GumboNode *aTag = nullptr;
            GumboVector *linkChildren = &linkNode->v.element.children;
            for (unsigned int i = 0; i < linkChildren->length; ++i)
            {
                GumboNode *linkChild = static_cast<GumboNode *>(linkChildren->data[i]);
                if (linkChild->type == GUMBO_NODE_ELEMENT && linkChild->v.element.tag == GUMBO_TAG_A)
                {
                    aTag = linkChild;
                    break;
                }
            }

            if (aTag)
            {
                GumboAttribute *href = gumbo_get_attribute(&aTag->v.element.attributes, "href");
                if (href)
                {
                    subjects[subjectCode] = href->value;
                    majorList[subjectCode] = getText(linkNode);
                }
            }
        }
    }

    GumboVector *children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i)
    {
        extractSubjects(static_cast<GumboNode *>(children->data[i]), subjects, majorList);
    }
}

map<string, string> content_UIUC(const string &url, map<string, string> &majorList)
{
    map<string, string> finalSubjects;
    map<string, string> finalList;
    // Fetch HTML content from the given URL
    string htmlContent = fetch_html(url);
    // cout << htmlContent << endl;
    // Parse HTML content with Gumbo
    GumboOutput *output = gumbo_parse(htmlContent.c_str());
    map<string, string> subjects;
    extractSubjects(output->root, subjects, finalList);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    for (auto &entry : finalList)
    {
        string first = entry.first;

        // Trim leading whitespace
        size_t start = first.find_first_not_of(" \n\r\t\f\v");
        if (start != string::npos)
        {
            first.erase(0, start);
        }
        else
        {
            first.clear(); // The string is all whitespace
        }

        // Trim trailing whitespace
        size_t end = first.find_last_not_of(" \n\r\t\f\v");
        if (end != string::npos)
        {
            first.erase(end + 1);
        }

        // Update link
        string second = entry.second;

        // Trim leading whitespace from the updated second
        start = second.find_first_not_of(" \n\r\t\f\v");
        if (start != string::npos)
        {
            second.erase(0, start);
        }
        else
        {
            second.clear(); // The string is all whitespace
        }

        // Trim trailing whitespace from the updated second
        end = second.find_last_not_of(" \n\r\t\f\v");
        if (end != string::npos)
        {
            second.erase(end + 1);
        }

        // Populate finalSubjects with cleaned-up first and second
        majorList[first] = second;
    }

    // Update the link to work
    for (auto &subject : subjects)
    {
        string first = subject.first;

        // Trim leading whitespace
        size_t start = first.find_first_not_of(" \n\r\t\f\v");
        if (start != string::npos)
        {
            first.erase(0, start);
        }
        else
        {
            first.clear(); // The string is all whitespace
        }

        // Trim trailing whitespace
        size_t end = first.find_last_not_of(" \n\r\t\f\v");
        if (end != string::npos)
        {
            first.erase(end + 1);
        }

        // Update link
        string second = subject.second;
        second = "https://courses.illinois.edu" + second;

        // Trim leading whitespace from the updated second
        start = second.find_first_not_of(" \n\r\t\f\v");
        if (start != string::npos)
        {
            second.erase(0, start);
        }
        else
        {
            second.clear(); // The string is all whitespace
        }

        // Trim trailing whitespace from the updated second
        end = second.find_last_not_of(" \n\r\t\f\v");
        if (end != string::npos)
        {
            second.erase(end + 1);
        }

        // Populate finalSubjects with cleaned-up first and second
        finalSubjects[first] = second;
    }
    return finalSubjects;
}
