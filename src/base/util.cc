#include <dirent.h>
#include <sys/stat.h>
#include "util.hpp"

bool BetaDistribution::exam() const {
    if (a < 0 || b < 0) return false;
    if (a == 0 && b == 0) return false;
    return true;
}
std::string BetaDistribution::toString() const {
    std::ostringstream oss;
    oss << "Be(" << a << ", " << b << ")";
    return oss.str();
}
std::ostream& operator<<(std::ostream& out, const BetaDistribution& b) {
    out << b.toString();
    return out;
}

std::string toupper(const std::string& str) {
    std::string s;
    for (char c : str) s.push_back(std::toupper(c));
    return s;
}
std::string tolower(const std::string& str) {
    std::string s;
    for (char c : str) s.push_back(std::tolower(c));
    return s;
}
bool isSuffix(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return s.substr(s.size() - suffix.size()) == suffix;
}
std::vector<std::string> split(const std::string& s,
                               char separator) {
    std::vector<std::string> result;
    std::string::size_type p = 0;
    std::string::size_type q;
    while ((q = s.find(separator, p)) != std::string::npos) {
        result.emplace_back(s, p, q - p);
        p = q + 1;
    }
    result.emplace_back(s, p);
    return result;
}
std::vector<std::string> split(const std::string& s,
                               const std::string& separators) {
    std::vector<std::string> result;
    std::string::size_type p = 0;
    std::string::size_type q;
    
    bool found;
    do {
        found = false;
        std::string::size_type minq = s.size();
        for (char sep : separators) {
            //cerr << "separator : " << sep << endl;
            if ((q = s.find(sep, p)) != std::string::npos) {
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

std::vector<std::string> getFilePathVector(const std::string& ipath, const std::string& suffix, bool recursive) {
    // make file path vector by suffix
    std::vector<std::string> file_names;
    DIR *pdir;
    dirent *pentry;

    pdir = opendir(ipath.c_str());
    if (pdir == nullptr) return file_names;
    do {
        pentry = readdir(pdir);
        if (pentry != nullptr) {
            const std::string name = std::string(pentry->d_name);
            const std::string full_path = ipath + name;
            struct stat st;
            stat(full_path.c_str(), &st);
            if ((st.st_mode & S_IFMT) == S_IFDIR) { // is directory
                if (recursive && name != "." && name != "..") {
                    std::vector<std::string> tfile_names = getFilePathVector(full_path + "/", suffix, true);
                    file_names.insert(file_names.end(), tfile_names.begin(), tfile_names.end()); // add vector
                }
            } else if (suffix.size() == 0 || isSuffix(name, suffix)) {
                file_names.emplace_back(full_path);
            }
        }
    } while (pentry != nullptr);
    return file_names;
}

std::vector<std::string> getFilePathVectorRecursively(const std::string& ipath, const std::string& suffix) {
    // make file path vector by suffix recursively
    return getFilePathVector(ipath, suffix, true);
}
