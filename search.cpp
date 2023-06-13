#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

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

void grepFile(const std::string &filename, const std::string &pattern) {
    std::ifstream ifs(filename);
    std::string line;
    while (std::getline(ifs, line)) {
        std::vector<std::string> result = split(line);
        for (auto s : result) {
            if (isMatch(s, 0, pattern, 0)) {
                std::cout << filename << " " << line << std::endl;
                break;
            }
        }
    }
}

void usage(char *program) {
    std::cout << program << " fileglobPattern regexPattern" << std::endl;
}

int main(int argc, char *argv[])
{

    if (argc < 3 || strcmp(argv[1], "--help") == 0 ) {
        usage(argv[0]);
        exit(0);
    }

    DIR *dirp = opendir(".");
    struct dirent *entry;

    std::string fileGlob = argv[1];
    std::string regexMatch = argv[2];

    while ((entry = readdir(dirp)) != NULL) {
        std::string filename = entry->d_name;
        if (filename[0] == '.')
            continue;
        if (globMatch(filename, fileGlob)) {
            grepFile(filename, regexMatch);
        }
    }

}
