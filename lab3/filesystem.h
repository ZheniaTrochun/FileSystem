#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>

#define BLOCKSIZE 16
#define MAX_DESCRIPTOR_ID 32
#define MAX_FILELEN 32

using namespace std;

const string filesystemImage = "image.txt";
string root;

int lastId = 0;

struct descriptor {
    int id;
    bool isOpened;
    bool isFile;
    unsigned int size;
    string name;
    FILE *file;
    vector<descriptor*> inner;

    descriptor() {
        isOpened = false;
    }
};

struct filelink {
    descriptor *fd;
    string name;
    string filename;
};


vector<descriptor*> descriptors;
vector<filelink*> links;

// You need to keep in mind that all sizes ONLY in blocks

bool mount(); 
void unmount(); 

string ls(); 

descriptor* create(string name); 
descriptor* open(string name); 
void close(descriptor *fd); 

char **read(descriptor *fd, unsigned int offset, unsigned int size);
bool write(descriptor *fd, unsigned int offset, unsigned int size, char **data);

filelink* link(string filename, string linkname); 
void unlink(string linkname); 

bool trunkate(string filename, unsigned int newSize);
