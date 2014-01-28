#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef WIN32
#include <windows.h>
#endif

std::string rootpath;

std::string readstr(uint8_t *mem, int max) {
    std::string ret;
    char ch;
    for (int i = 0; i < max && (ch = mem[i]); i++) {
        ret += ch;
    }
    return ret;
}

std::string hex(int v, int len) {
    char buf[32], format[16] = "%x";
    if (v < 0) {
        strcpy(format, "-%x");
        v = -v;
    } else if (0 < len && len < 32) {
        snprintf(format, sizeof (format), "%%0%dx", len);
    }
    snprintf(buf, sizeof (buf), format, v);
    return buf;
}

std::string hexdump(uint8_t *mem, int len) {
    std::string ret;
    char buf[3];
    for (int i = 0; i < len; i++) {
        snprintf(buf, sizeof (buf), "%02x", mem[i]);
        ret += buf;
    }
    return ret;
}

std::string hexdump2(uint8_t *mem, int len) {
    std::string ret;
    char buf[5];
    for (int i = 0; i < len; i += 2) {
        if (i > 0) ret += ' ';
        snprintf(buf, sizeof (buf), "%04x", read16(mem + i));
        ret += buf;
    }
    return ret;
}

void setroot(std::string root) {
    while (endsWith(root, "/"))
        root = root.substr(0, root.size() - 1);
    rootpath = root;
}

std::string convpath(const std::string &path) {
#ifdef WIN32
    if (startsWith(path, "/tmp/"))
        return getenv("TEMP") + path.substr(4);
    if (startsWith(path, "/usr/tmp/"))
        return getenv("TEMP") + path.substr(8);
#else
    if (startsWith(path, "/usr/tmp/"))
        return path.substr(4);
#endif
    if (!rootpath.empty()) {
        if (startsWith(path, "/")) {
            std::string path2 = rootpath + path;
            struct stat st;
            if (!stat(path2.c_str(), &st)) return path2;
        }
    }
    return path;
}

bool startsWith(const std::string &s, const std::string &prefix) {
    if (s.size() < prefix.size()) return false;
    return s.substr(0, prefix.size()) == prefix;
}

bool endsWith(const std::string &s, const std::string &suffix) {
    if (s.size() < suffix.size()) return false;
    return s.substr(s.size() - suffix.size(), suffix.size()) == suffix;
}

std::string replace(const std::string &src, const std::string &s1, const std::string &s2) {
    if (s1.empty()) return src;
    std::string ret;
    int p = 0;
    while (p < (int) src.size()) {
        int pp = src.find(s1, p);
        if (pp < 0) {
            ret += src.substr(p);
            break;
        }
        ret += src.substr(p, pp - p) + s2;
        p = pp + s1.size();
    }
    return ret;
}

#ifdef WIN32

std::string getErrorMessage(int err) {
    LPVOID lpMsgBuf;
    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) & lpMsgBuf,
            0,
            NULL
            );
    std::string ret = replace((const char *) lpMsgBuf, "\r\n", "\n");
    LocalFree(lpMsgBuf);
    if (!endsWith(ret, "\n")) ret += "\n";
    return ret;
}
#endif
