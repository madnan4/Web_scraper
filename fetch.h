#ifndef FETCH_H
#define FETCH_H

#include <iostream>
#include <string>
#include <map>
#include <curl/curl.h>
#include <gumbo.h>
#include <fstream>

using namespace std;

// Callback function for writing data fetched by curl
size_t WriteCallback(void *contents, size_t size, size_t nmemb, string *s);

// Function to fetch the HTML content of a webpage
string fetch_html(const string &url);

#endif
