// vim: sw=4 ts=4 ai expandtab
#include "SearchPath.h"
#include "StaticBuffer.h"
#include "misc.h"
#include <iterator>
#include <unistd.h>
#include <string.h>

using namespace std;

SearchPath::SearchPath(const bool _i, const char* _d, const char *_f, const char *_v) :
    internal(_i),
    path(buildPath(_f, _v))
{
    as_string = Sprintf("- %s: %s%s", _d, path.c_str(), _i ? " (internal)" : "");
};

const string SearchPath::buildPath(const char *f, const char *v) {
    string path = Sprintf(f, v);
    if (*(path.cend()-1) == '/') {
        return path;
    }
    return path + "/";
};

const string SearchPath::asPath() const {
    return path;
}
;
const string SearchPath::asPath(const string filename) const {
    return path + filename;
};

const string SearchPath::asString() const {
    return as_string;
};

SearchPathFinder::SearchPathFinder(vector<SearchPath> _paths) :
    paths(_paths)
{
    array = new const char*[paths.size()];
    const char **tmp = array;
    if (!array) {
        error("Unable to calloc(%ld, %ld)\n", paths.size(), sizeof(char*));
    }
    string buffer = "Search paths include:\n";
    for (auto const& sp: paths) {
        buffer += "  " + sp.asString() + "\n";
        *(tmp++) = sp.asPath("").c_str();
    }
    as_string = buffer;
};

SearchPathFinder::~SearchPathFinder() {
    if (array) {
        for (uint i = 0 ; i < paths.size() ; i++) {
            array[i] = 0;
        }
        delete [] array;
    }
};

const string SearchPathFinder::find(const string filename) const {
    for (auto const& sp: paths) {
        const string fullpath = sp.asPath(filename);
        if (access(fullpath.c_str(), R_OK) == 0) {
            return fullpath;
        }
    }
    return "";
};

const string SearchPathFinder::asString() const {
    return as_string;
};

const char * const * SearchPathFinder::asArray() const {
    return array;
};

const vector<SearchPath> SearchPathFinder::getPaths() const {
    return paths;
};

const uint SearchPathFinder::size() const {
    return paths.size();
};

