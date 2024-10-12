#include "fetch.h"

// Callback function for writing data fetched by curl
size_t WriteCallback(void *contents, size_t size, size_t nmemb, string *s)
{
    size_t newLength = size * nmemb;
    try
    {
        s->append((char *)contents, newLength);
    }
    catch (bad_alloc &e)
    {
        // Handle memory problem
        return 0;
    }
    return newLength;
}

// Function to fetch the HTML content of a webpage
string fetch_html(const string &url)
{
    CURL *curl;
    CURLcode res;
    string html;
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }
        curl_easy_cleanup(curl);
    }
    return html;
}