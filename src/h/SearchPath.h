// vim: sw=4 ts=4 ai expandtab
#ifndef DIRT_SEARCH_PATH_H
#define DIRT_SEARCH_PATH_H

#include <string>
#include <vector>

using namespace std;

class SearchPath {
    public:
        SearchPath(const bool internal, const char * description, const char * fmt, const char * var);
        const string asPath() const;
        const string asPath(const string filename) const;
        const string asString() const;
    private:
        const bool internal;
        const string path;
        string as_string;
        static const string buildPath(const char *filename, const char *value);
};

class SearchPathFinder {
    public:
        SearchPathFinder(vector<SearchPath> paths);
        ~SearchPathFinder();
        const string find(const string filename) const;
        const string asString() const;
        const char * const * asArray() const;
        const uint size() const;
        const vector<SearchPath> getPaths() const;
    private:
        const vector<SearchPath> paths;
        const char ** array;
        string as_string;
};

#endif
