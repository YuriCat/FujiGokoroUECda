#include <dirent.h>
#include <sys/stat.h>
#include "util.hpp"

using namespace std;

bool BetaDistribution::exam() const {
    if (a < 0 || b < 0) return false;
    if (a == 0 && b == 0) return false;
    return true;
}
string BetaDistribution::toString() const {
    ostringstream oss;
    oss << "Be(" << a << ", " << b << ")";
    return oss.str();
}
ostream& operator<<(ostream& out, const BetaDistribution& b) {
    out << b.toString();
    return out;
}

string toupper(const string& str) {
    string s;
    for (char c : str) s.push_back(toupper(c));
    return s;
}
string tolower(const string& str) {
    string s;
    for (char c : str) s.push_back(tolower(c));
    return s;
}
bool isSuffix(const string& s, const string& suffix) {
    if (s.size() < suffix.size()) return false;
    return s.substr(s.size() - suffix.size()) == suffix;
}
vector<string> split(const string& s,
                               char separator) {
    vector<string> result;
    string::size_type p = 0;
    string::size_type q;
    while ((q = s.find(separator, p)) != string::npos) {
        result.emplace_back(s, p, q - p);
        p = q + 1;
    }
    result.emplace_back(s, p);
    return result;
}
vector<string> split(const string& s,
                               const string& separators) {
    vector<string> result;
    string::size_type p = 0;
    string::size_type q;

    bool found;
    do {
        found = false;
        string::size_type minq = s.size();
        for (char sep : separators) {
            //cerr << "separator : " << sep << endl;
            if ((q = s.find(sep, p)) != string::npos) {
                minq = min(minq, q);
                found = true;
            }
        }
        if (found) {
            result.emplace_back(s, p, minq - p);
            p = minq + 1;
        }
    } while(found);
    result.emplace_back(s, p);
    return result;
}

vector<string> getFilePathVector(const string& ipath, const string& suffix, bool recursive) {
    // make file path vector by suffix
    vector<string> file_names;
    DIR *pdir;
    dirent *pentry;

    pdir = opendir(ipath.c_str());
    if (pdir == nullptr) return file_names;
    do {
        pentry = readdir(pdir);
        if (pentry != nullptr) {
            const string name = string(pentry->d_name);
            const string full_path = ipath + name;
            struct stat st;
            stat(full_path.c_str(), &st);
            if ((st.st_mode & S_IFMT) == S_IFDIR) { // is directory
                if (recursive && name != "." && name != "..") {
                    vector<string> tfile_names = getFilePathVector(full_path + "/", suffix, true);
                    file_names.insert(file_names.end(), tfile_names.begin(), tfile_names.end()); // add vector
                }
            } else if (suffix.size() == 0 || isSuffix(name, suffix)) {
                file_names.emplace_back(full_path);
            }
        }
    } while (pentry != nullptr);
    return file_names;
}

vector<string> getFilePathVectorRecursively(const string& ipath, const string& suffix) {
    // make file path vector by suffix recursively
    return getFilePathVector(ipath, suffix, true);
}
