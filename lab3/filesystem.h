#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>

#define BLOCKSIZE 16

using namespace std;

const string filesystemImage = "image.txt";
string root;

int lastId = 0;

struct descriptor {
    int id;
    bool isOpened;
    string name;
    FILE *file;

    descriptor() {
        isOpened = false;
    }
};

struct filedata {
    string name;
    descriptor *fd;
    unsigned int size;
};

struct filelink {
    descriptor *fd;
    string name;
    string filename;
};


vector<filedata> filesData;
vector<filelink*> links;


bool mount(); // +
void unmount(); // +

string ls(); // +

descriptor* create(string name); // +
descriptor* open(string name); // +
void close(descriptor *fd); // +

char **read(descriptor *fd, unsigned int offset, unsigned int size);
bool write(descriptor *fd, unsigned int offset, unsigned int size, char **data);

filelink* link(string filename, string linkname); // +
void unlink(string linkname); // +

// todo rework
bool trunkate(string filename, unsigned int newSize);
