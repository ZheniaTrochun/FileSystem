#include "filesystem.h"

const char *fileRoot;

bool isFile(char *name);
void fillData(vector<string> files);
unsigned int getFileSize(string name);
descriptor *getFileDescr(string name);

bool mount() {
    ifstream sysFile ("image.txt");

    if (sysFile.is_open()) {
        getline(sysFile, root);

        fileRoot = root.c_str();

        sysFile.close();

        vector<string> filenames;
        DIR *dir;
        struct dirent *ent;

        if ((dir = opendir (fileRoot)) != NULL) {
            /* print all the files and directories within directory */
            while ((ent = readdir (dir)) != NULL) {
                printf ("file: %s\n", ent->d_name);
                printf ("is file: %d\n", isFile(ent->d_name));
        
                if (isFile(ent->d_name)) {
                    filenames.push_back(*(new string(ent->d_name)));
                }
            }
            closedir (dir);
    
            fillData(filenames);
    
            for (int i(0); i < filesData.size(); i++) {
                cout << filesData[i].name << endl;
            }
        } else {
            /* could not open directory */
            perror ("");
            return false;
        }

        return true;
    }

    return false;
}


void unmount() {
    filesData.clear();
    links.clear();
    lastId = 0;
    root = "";
}


string ls() {
    string res = "";

    for (int i(0); i < filesData.size(); i++) {
        res += filesData[i].name + "\n";
    }

    cout << res << endl;

    return res;
}



descriptor* open(string name) {
    descriptor *fd = getFileDescr(name);

    if (fd == NULL) {
        return NULL;
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
    FILE *file = fopen((root + "/" + name).c_str(), "w");

    descriptor *fd = new descriptor();
    fd->name = name;
    fd->isOpened = false;
    fd->id = ++lastId;

    filedata data;
    data.name = name;
    data.fd = fd;
    data.size = 0;

    fclose(file);

    for (int i(0); i < filesData.size(); i++) {
        if (filesData[i].name == name) {
            filesData[i] = data;
    
            return fd;
        }
    }

    filesData.push_back(data);

    return fd;
}

filelink* link(string filename, string linkname) {
    filelink* link = new filelink();

    for (int i(0); i < links.size(); i++) {
        if (links[i]->name == linkname) {
            return links[i];
        }
    }


    for (int i(0); i < filesData.size(); i++) {
        if (filesData[i].name == filename) {

            link->name = linkname;
            link->filename = filename;
            link->fd = filesData[i].fd;

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
    //descriptor *fd = open(filename);
    descriptor *fd = getFileDescr(filename);

    if (fd == NULL) {
        return false;
    }

    if (!fd->isOpened) {
        fd = open(filename);
    }

    unsigned int oldSize = getFileSize(filename)/BLOCKSIZE;
    
    if (oldSize > newSize) {
        char **buf = read(fd, 0, newSize);    
        fseek(fd->file, 0, SEEK_SET);
        write(fd, 0, newSize, buf);
    } else {
        char zeros[BLOCKSIZE];
        
        for (int i(0); i < BLOCKSIZE; i++) {
            zeros[i] = '0';
        }

        char *data[1];
        data[0] = zeros;

        char **buf = read(fd, 0, oldSize);            
        
        fseek(fd->file, 0, SEEK_SET);
        write(fd, 0, oldSize, buf);                
        
        for (int i(oldSize); i < newSize; i++) {
            write(fd, i, 1, data);
        }
    }
    
    return false;
}


char **read(descriptor *fd, unsigned int offset, unsigned int size) {
    char **buf;
    buf = new char*[size];

    if (offset*BLOCKSIZE + size * BLOCKSIZE > getFileSize(fd->name)) {
        cout << "Error\n";

        return NULL;
    }

    for (int i(0); i < size; i++) {
        buf[i] = new char[BLOCKSIZE + 1];
        buf[i][BLOCKSIZE] = '\0';
    }

    if (fd->isOpened) {
        for (int i(0); i < size; i++) {
            if (fseek(fd->file, offset*BLOCKSIZE + i*BLOCKSIZE, SEEK_SET) == -1) {
                cout << "Error\n";

                return NULL;
            }
            
            if (fread(buf[i], 1, BLOCKSIZE, fd->file) == 0) {
                cout << "Error\n";
                
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
                cout << "Error\n";

                return false;
            }
            
            if (fwrite(data[i], 1, BLOCKSIZE, fd->file) == 0) {
                cout << "Error\n";
                
                return false;
            }
        }

        return true;
    }

    return false;
}

bool isFile(char *name) {
    struct stat buf;
    stat((root + "/" + name).c_str(), &buf);
    return S_ISREG(buf.st_mode);
}

void fillData(vector<string> files) {
    for (int i(0); i < files.size(); i++) {
        descriptor *fd = new descriptor();
        fd->id = ++lastId;
        fd->name = files[i];

        filedata data;
        data.fd = fd;
        data.name = files[i];
        data.size = getFileSize(files[i]);

        filesData.push_back(data);
    }
}

unsigned int getFileSize(string name) {
    struct stat buf;
    int rc = stat((root + "/" + name).c_str(), &buf);
    
    return rc == 0 ? buf.st_size : -1;
}

descriptor *getFileDescr(string name) {
    for (int i(0); i < filesData.size(); i++) {
        if (filesData[i].name == name) {
            return filesData[i].fd;
        }
    }

    return NULL;
}

int main() {
    bool flag = mount();

    cout << "filesData.size()\t" << filesData.size() << endl;
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

    cout << "filesData.size()\t" << filesData.size() << endl;
    cout << "lastId\t" << lastId << endl;
    
    return 0;
}