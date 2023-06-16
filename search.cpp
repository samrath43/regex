#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stack>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <memory>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

static unsigned char a2e[256] = {
         0,  1,  2,  3, 55, 45, 46, 47, 22,  5, 37, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 60, 61, 50, 38, 24, 25, 63, 39, 28, 29, 30, 31,
        64, 79,127,123, 91,108, 80,125, 77, 93, 92, 78,107, 96, 75, 97,
       240,241,242,243,244,245,246,247,248,249,122, 94, 76,126,110,111,
       124,193,194,195,196,197,198,199,200,201,209,210,211,212,213,214,
       215,216,217,226,227,228,229,230,231,232,233, 74,224, 90, 95,109,
       121,129,130,131,132,133,134,135,136,137,145,146,147,148,149,150,
       151,152,153,162,163,164,165,166,167,168,169,192,106,208,161,  7,
        32, 33, 34, 35, 36, 21,  6, 23, 40, 41, 42, 43, 44,  9, 10, 27,
        48, 49, 26, 51, 52, 53, 54,  8, 56, 57, 58, 59,  4, 20, 62,225,
        65, 66, 67, 68, 69, 70, 71, 72, 73, 81, 82, 83, 84, 85, 86, 87,
        88, 89, 98, 99,100,101,102,103,104,105,112,113,114,115,116,117,
       118,119,120,128,138,139,140,141,142,143,144,154,155,156,157,158,
       159,160,170,171,172,173,174,175,176,177,178,179,180,181,182,183,
       184,185,186,187,188,189,190,191,202,203,204,205,206,207,218,219,
       220,221,222,223,234,235,236,237,238,239,250,251,252,253,254,255
};

void convertEBCIDCtoASCII(std::string& s) {
    for (int i = 0; i < s.size(); i++) {
        s[i] = a2e[s[i]];
    }
}


class OpenDirException : public std::exception {
private:
    int m_error;
public:
    OpenDirException(int error) : m_error(error) {}
    const char *what() {
        return "OpenDirException occurred";
    }
};

enum FileType {
    M_DIR,
    M_FILE
};

enum Trait {
    M_HIDDEN,
    M_NONE
};

struct FilePredicate {
    bool fileGlob;
    std::string fileGlobPattern;
    bool fileRegex;
    std::string fileRegexPattern;
};

bool isMatch(const std::string& s, int i,
             const std::string& p, int j) {

    if (i >= s.size() && j >= p.size()) {
        return true;
    }
    if (j >= p.size()) {
        return false;
    }

    bool match = false;
    if (s[i] == p[j] || p[j] == '.') {
        match = true;
    }
    if (p[j + 1] == '*') {
        return isMatch(s, i, p, j + 2) ||
               (match && isMatch(s, i + 1, p, j));
    }
    if (match) {
        return isMatch(s, i + 1, p, j + 1);
    }
    return false;
}

bool globMatch(const std::string &s, const std::string &pattern) {
    int i = s.size() - 1;
    int j = pattern.size() - 1;
    while (i >= 0) {
        if (s[i] == pattern[j]) {
            i--;
            j--;
        } else if (pattern[j] == '*') {
            i--;
        } else {
            break;
        }
    }


    return i <= 0;
}


class Dirent {
private:
    std::string m_entry_name;
    DIR *mdirp;
    FileType m_curr_file_type;
    std::string m_curr_file_name;
    Trait m_trait;
public:
    Dirent(const std::string &entry_name) {
        mdirp = opendir(entry_name.c_str());
        if (mdirp == NULL) {
            throw OpenDirException(errno);
        }
        m_entry_name = entry_name;
    }

    ~Dirent() {
        if (!m_entry_name.empty()) {
            closedir(mdirp);
        }
    }

    bool read() {
        struct dirent *entry;
        entry = readdir(mdirp);
        if (entry == NULL)
            return false;
        m_curr_file_name = m_entry_name + "/" + entry->d_name;
        if (entry->d_name[0] == '.') {
            m_trait = Trait::M_HIDDEN;
        } else {
            m_trait = Trait::M_NONE;
        }
        m_curr_file_type = entry->d_type == DT_DIR ? FileType::M_DIR : FileType::M_FILE;
        return true;
    }

    std::string get_filename() {
        return m_curr_file_name;
    }

    FileType get_filetype() {
        return m_curr_file_type;
    }

    Trait get_trait() {
        return m_trait;
    }
    
};

class DirentOps {
private:
    std::string m_initial_dir;
    std::shared_ptr<Dirent> m_dirent;
public:
    DirentOps(const std::string &initial_dir) {
    	m_initial_dir = initial_dir;
        m_dirent = std::make_shared<Dirent>(m_initial_dir);
    }

    void grepFile(const std::string &filename, const std::string &pattern) {
        std::ifstream ifs(filename);
        std::string line;
        while (std::getline(ifs, line)) {
            std::vector<std::string> result = split(line);
            for (auto s : result) {
                if (isMatch(s, 0, pattern, 0)) {
                    //convertEBCIDCtoASCII(filename);
                    //convertEBCIDCtoASCII(line);
                    std::cout << filename << " " << line << std::endl;
                    break;
                }
            }
        }
    }

    void browse(FilePredicate fp = {}) {
        std::stack<std::shared_ptr<Dirent> > dirents;
        dirents.push(m_dirent);
        
        while (!dirents.empty()) {
            std::shared_ptr<Dirent> d = dirents.top();
            dirents.pop();

            while (d->read()) {
                if (d->get_trait() == Trait::M_HIDDEN)
                    continue;
                if (d->get_filetype() == FileType::M_DIR) {
                    std::shared_ptr<Dirent> newdir = std::make_shared<Dirent>(d->get_filename());
                    dirents.push(newdir);
                    continue;
                }
                if (fp.fileGlob && globMatch(d->get_filename(), fp.fileGlobPattern)) {
                    if (fp.fileRegex)
                        grepFile(d->get_filename(), fp.fileRegexPattern);
                    else {
                        std::string filename = d->get_filename();
                        //convertEBCIDCtoASCII(filename);
                        std::cout << filename << std::endl;
                    }
                }
            }
        }
    }
private:
    std::vector<std::string> split(std::string line) {
        std::vector<std::string> result;
        char *p;

        p = strtok((char*)line.c_str(), " ");
        while (p != NULL) {
            std::string res(p);
            result.push_back(res);
            p = strtok(NULL, " ");
        }

        return result;
    }
};

void usage(char *program) {
    std::cout << program << " fileglobPattern [regexPattern]" << std::endl;
}

int main(int argc, char *argv[])
{
    char cwd[256];
    FilePredicate fp;

    if (argc == 1 || (argc > 1 && strcmp(argv[1], "--help") == 0)) {
        usage(argv[0]);
        exit(0);
    }

    if (argc > 1) {
        fp.fileGlob = true;
        fp.fileRegex = false;
        fp.fileGlobPattern = argv[1];
    }
    if (argc > 2) {
        fp.fileRegex = true;
        fp.fileRegexPattern = argv[2];
    }

    getcwd(cwd, sizeof(cwd));
    try {
        DirentOps diops(cwd);
        diops.browse(fp);
    } catch(...) {
        std::cerr << "Error opening directory" << std::endl;
    }
}
