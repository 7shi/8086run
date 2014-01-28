#include "VMBase.h"
#include "UnixBase.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int trace;

VMBase::VMBase()
: text(NULL), data(NULL), tsize(0), brksize(0), hasExited(false) {
}

VMBase::VMBase(const VMBase &vm) : hasExited(false) {
    text = new uint8_t[0x10000];
    memcpy(text, vm.text, 0x10000);
    if (vm.data == vm.text) {
        data = text;
    } else {
        data = new uint8_t[0x10000];
        memcpy(data, vm.data, 0x10000);
    }
    tsize = vm.tsize;
    dsize = vm.dsize;
    brksize = vm.brksize;
}

VMBase::~VMBase() {
    release();
}

void VMBase::release() {
    if (data && data != text) delete[] data;
    if (text) delete[] text;
    text = data = NULL;
}

void VMBase::showsym(uint16_t addr) {
    std::map<int, Symbol>::iterator it;
    it = syms[0].find(addr);
    if (it != syms[0].end()) {
        printf("\n[%s]\n", it->second.name.c_str());
    }
    it = syms[1].find(addr);
    if (it != syms[1].end()) {
        printf("%s:\n", it->second.name.c_str());
    }
}

void VMBase::debugsym(uint16_t pc) {
    std::map<int, Symbol>::iterator it = syms[1].find(pc);
    if (it != syms[1].end()) {
        fprintf(stderr, "%s:\n", it->second.name.c_str());
    }
}

bool VMBase::load(const std::string& fn, FILE* f, size_t size) {
    if (size > 0xffff) {
        fprintf(stderr, "too long raw binary: %s\n", fn.c_str());
        return false;
    }
    release();
    text = data = new uint8_t[0x10000];
    memset(text, 0, 0x10000);
    fseek(f, 0, SEEK_SET);
    fread(text, 1, size, f);
    tsize = brksize = size;
    dsize = 0;
    return true;
}
