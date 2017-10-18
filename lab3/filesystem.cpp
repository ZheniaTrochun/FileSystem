#include "filesystem.h"
#include <string.h>

const char *fileRoot;

bool isFile(char *name);
vector<descriptor*> fillData(vector<string> files);
vector<descriptor*> openDirAndReadFiles(string name);
unsigned int getFileSize(string name);
descriptor *getFileDescr(string name);


bool mount() {
    ifstream sysFile ("image.txt");

    if (sysFile.is_open()) {
        getline(sysFile, root);

        sysFile.close();

        openDirAndReadFiles("");

        return true;
    }

    return false;
}


vector<descriptor*> openDirAndReadFiles(string path) {
    const char *name = path.c_str();

    vector<string> filenames;
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir ((root + "/" + name).c_str())) != NULL) {
        while ((ent = readdir (dir)) != NULL) {

            if (strlen(ent->d_name) > 2) {
                filenames.push_back(*(new string((path == "") ? ent->d_name : (path + "/" + ent->d_name))));
            }
        }
        closedir (dir);

        return fillData(filenames);
    } else {
        perror ("Not able to open dir");

        return *(new vector<descriptor*>());
    }
}


void unmount() {
    descriptors.clear();
    links.clear();
    lastId = 0;
    root = "";
}


string ls() {
    string res = "";

    for (int i(0); i < descriptors.size(); i++) {
        res += descriptors[i]->name + "\n";
    }

    cout << res << endl;

    return res;
}



descriptor* open(string name) {
    descriptor *fd = getFileDescr(name);

    if (fd == NULL) {
        fd = new descriptor();
        fd->file = fopen((root + "/" + name).c_str(), "r+");
        fd->isOpened = true;
        fd->name = name;

        fd->size = 0;
        fd->isFile = true;

        descriptors.push_back(fd);

        return fd;
    }

    fd->file = fopen((root + "/" + fd->name).c_str(), "r+");
    fd->isOpened = true;

    return fd;
}

void close(descriptor *fd) {
    fclose(fd->file);
    fd->isOpened = false;
}

descriptor* create(string name) {

    if (lastId == MAX_DESCRIPTOR_ID) {
        perror("Max number of files!");

        return NULL;
    }

    if (name.size() > MAX_FILELEN) {
        perror("Too long name!");
        
        return NULL;
    }


    FILE *file = fopen((root + "/" + name).c_str(), "w");

    descriptor *fd = new descriptor();
    fd->name = name;
    fd->id = ++lastId;

    fd->size = 0;
    fd->isFile = true;

    fclose(file);

    for (int i(0); i < descriptors.size(); i++) {
        if (descriptors[i]->name == name) {
            descriptors[i] = fd;
    
            return fd;
        }
    }

    descriptors.push_back(fd);

    return fd;
}

filelink* link(string filename, string linkname) {
    filelink* link = new filelink();

    for (int i(0); i < links.size(); i++) {
        if (links[i]->name == linkname) {
            return links[i];
        }
    }


    for (int i(0); i < descriptors.size(); i++) {
        if (descriptors[i]->name == filename) {

            link->name = linkname;
            link->fd = descriptors[i];

            links.push_back(link);

            return link;
        }
    }

    return NULL;
}

void unlink(string linkname) {
    for (int i(0); i < links.size(); i++) {
        if (links[i]->name == linkname) {
            links.erase(links.begin() + i);

            return;
        }
    }
}

bool trunkate(string filename, unsigned int newSize) {
    descriptor *fd = getFileDescr(filename);

    if ((fd == NULL) || (!fd->isOpened)) {
        fd = open(filename);
    }

    unsigned int oldSize = getFileSize(filename)/BLOCKSIZE;
    
    if (oldSize > newSize) {
        char **buf = read(fd, 0, newSize);    
        fseek(fd->file, 0, SEEK_SET);
        write(fd, 0, newSize, buf);
    } else {
        
        char **zeroData;
        zeroData = new char*[1];
        zeroData[0] = new char[BLOCKSIZE];

        for (int i(0); i < BLOCKSIZE; i++) {
            zeroData[0][i] = '0';
        }

        char **buf = read(fd, 0, oldSize);            
        
        fseek(fd->file, 0, SEEK_SET);
        write(fd, 0, oldSize, buf);                
        
        for (int i(oldSize); i < newSize; i++) {
            write(fd, i, 1, zeroData);
        }

        delete zeroData[0];
        delete zeroData;
    }
    
    return false;
}


char **read(descriptor *fd, unsigned int offset, unsigned int size) {
    char **buf;
    buf = new char*[size];

    if (offset*BLOCKSIZE + size * BLOCKSIZE > getFileSize(fd->name)) {
        perror("Invalid size");

        return NULL;
    }

    for (int i(0); i < size; i++) {
        buf[i] = new char[BLOCKSIZE + 1];
        buf[i][BLOCKSIZE] = '\0';
    }

    if (fd->isOpened) {
        for (int i(0); i < size; i++) {
            if (fseek(fd->file, offset*BLOCKSIZE + i*BLOCKSIZE, SEEK_SET) == -1) {
                perror("fseek error");

                return NULL;
            }
            
            if (fread(buf[i], 1, BLOCKSIZE, fd->file) == 0) {
                perror("fread error");
                
                return NULL;
            }
        }

        return buf;
    }

    return NULL;
}


bool write(descriptor *fd, unsigned int offset, unsigned int size, char **data) {

    if (fd->isOpened) {
        for (int i(0); i < size; i++) {
            if (fseek(fd->file, offset*BLOCKSIZE + i*BLOCKSIZE, SEEK_SET) == -1) {
                perror("fseek error");

                return false;
            }
            
            if (fwrite(data[i], 1, BLOCKSIZE, fd->file) == 0) {
                perror("fwrite error");
                
                return false;
            }
        }

        return true;
    }

    return false;
}

bool isFile(const char *name) {
    struct stat buf;
    stat((root + "/" + name).c_str(), &buf);
    return S_ISREG(buf.st_mode);
}

vector<descriptor*> fillData(vector<string> files) {
    vector<descriptor*> fds;
    
    for (int i(0); i < files.size(); i++) {
        descriptor *fd = new descriptor();
        fd->id = ++lastId;
        fd->name = files[i];
        fd->isFile = isFile(fd->name.c_str());
        
        if (fd->isFile) {
            fd->size = getFileSize(files[i]);
        } else {
            fd->inner = openDirAndReadFiles(fd->name);
        }
            
        fds.push_back(fd);
        descriptors.push_back(fd);
    }

    return fds;
}

unsigned int getFileSize(string name) {
    struct stat buf;
    int rc = stat((root + "/" + name).c_str(), &buf);
    
    return rc == 0 ? buf.st_size : -1;
}

descriptor *getFileDescr(string name) {
    for (int i(0); i < descriptors.size(); i++) {
        if (descriptors[i]->name == name) {
            return descriptors[i];
        }
    }

    return NULL;
}

int main() {
    bool flag = mount();

    cout << "descriptors.size()\t" << descriptors.size() << endl;
    cout << "lastId\t" << lastId << endl;

    cout << endl << "ls" << endl;

    ls();

    create("321.txt");

    cout << endl << "created file ls" << endl;

    ls();

    descriptor *fd1 = open("123.txt");
    descriptor *fd2 = open("321.txt");

    write(fd2, 0, 1, read(fd1, 0, 1));

    trunkate("123.txt", 4);

    unmount();
    cout << "unmount()" << endl;

    cout << "descriptors.size()\t" << descriptors.size() << endl;
    cout << "lastId\t" << lastId << endl;
    
    return 0;
}