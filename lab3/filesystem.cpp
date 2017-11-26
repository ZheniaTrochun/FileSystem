#include "filesystem.h"
#include <string.h>

const char *fileRoot;

bool isFile(char *name);
vector<descriptor*> fillData(vector<string> files);
vector<descriptor*> openDirAndReadFiles(string name);
unsigned int getFileSize(string name);
descriptor *getFileDescr(string name);
dataBlock *getDataBlockById(int id);
bitmapItem *getBitmapItemById(int id);


// todo fix serialization
bool mount() {
    ifstream sysFile ("image.txt", ios_base::in | ios_base::binary);

    sysFile.read((char *)&descriptors, sizeof(descriptors));

    sysFile.read((char *)&bitmap, sizeof(bitmap));

    sysFile.read((char *)&data, sizeof(data));

    return true;
}


// todo fix serialization
void unmount() {
    ofstream sysFile ("image.txt", ios_base::in | ios_base::binary);

    sysFile.write((char *)&descriptors, sizeof(descriptors));

    sysFile.write((char *)&bitmap, sizeof(bitmap));

    sysFile.write((char *)&data, sizeof(data));

    sysFile.close();

    descriptors.clear();
    links.clear();
    bitmap.clear();
    data.clear();

    lastDescriptorId = 0;
    lastBlockId = 0;
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
        fd->isOpened = true;
        fd->name = name;

        fd->size = 0;
        fd->isFile = true;

        fd->size = 0;

        descriptors.push_back(fd);

        return fd;
    }

    fd->isOpened = true;

    return fd;
}



void close(descriptor *fd) {
    fd->isOpened = false;
}



descriptor* create(string name) {

    if (lastDescriptorId == MAX_DESCRIPTOR_ID) {
        perror("Max number of files!");

        return NULL;
    }

    if (name.size() > MAX_FILELEN) {
        perror("Too long name!");
        
        return NULL;
    }

    descriptor *fd = new descriptor();
    fd->name = name;
    fd->id = ++lastDescriptorId;

    fd->size = 0;
    fd->isFile = true;

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

    if (fd->size > newSize) {
        int delta = fd->size - newSize;

        fd->dataId.erase(fd->dataId.begin() + newSize, fd->dataId.begin() + fd->size + 1);
    } else {
        
        char **zeroData;
        zeroData = new char*[1];
        zeroData[0] = new char[BLOCKSIZE];

        for (int i(0); i < BLOCKSIZE; i++) {
            zeroData[0][i] = '0';
        }

        int delta = newSize - fd->size;

        for (int i(0); i < delta; i++) {
            int newId = ++lastBlockId;

            bitmapItem *newItem = new bitmapItem();

            newItem->id = newId;
            newItem->isFree = true;

            dataBlock *newBlock = new dataBlock();

            newBlock->id = newId;
            newBlock->data = zeroData[0];

            fd->dataId.push_back(newId);
            bitmap.push_back(newItem);
            data.push_back(newBlock);
        }

        delete zeroData[0];
        delete zeroData;
    }
    
    return false;
}



char **read(descriptor *fd, unsigned int offset, unsigned int size) {
    char **buf;
    buf = new char*[size];

    if (offset + size > fd->size) {
        perror("Invalid size");

        return NULL;
    }

    for (int i(0); i < size; i++) {
        buf[i] = new char[BLOCKSIZE + 1];
        buf[i][BLOCKSIZE] = '\0';
    }

    if (fd->isOpened) {
        for (int i(0); i < size; i++) {
            char *data = getDataBlockById(fd->dataId[i + offset])->data;
            for (int j(0); j < BLOCKSIZE; j++) {
                buf[i][j] = data[i];
            }
        }

        return buf;
    }

    return NULL;
}



bool write(descriptor *fd, unsigned int offset, unsigned int size, char **dataToWrite) {

    if (fd->isOpened) {
        if (offset + size <= fd->size) {
            for (int i(offset); i < offset + size; i++) {
                getBitmapItemById(fd->dataId[i])->isFree = false;
                getDataBlockById(fd->dataId[i])->data = dataToWrite[i - offset];
            }
        } else {
            for (int i(offset); i < fd->size; i++) {
                getBitmapItemById(fd->dataId[i])->isFree = false;
                getDataBlockById(fd->dataId[i])->data = dataToWrite[i - offset];
            }

            for (int i(0); i < offset + size - fd->size; i++) {
                dataBlock *newBlock = new dataBlock();
                bitmapItem *newBitmapItem = new bitmapItem();
    
                newBlock->id = ++lastBlockId;
                newBlock->data = dataToWrite[i + fd->size];
    
                newBitmapItem->id = newBlock->id;
                newBitmapItem->isFree = false;
                
                fd->dataId.push_back(lastBlockId);
                bitmap.push_back(newBitmapItem);
                data.push_back(newBlock);
            }
        }
        fd->size = (offset + size) > fd->size ? (offset + size) : fd->size;

        return true;
    }

    return false;
}


bool isFile(const char *name) {
    struct stat buf;
    stat((root + "/" + name).c_str(), &buf);
    return S_ISREG(buf.st_mode);
}

unsigned int getFileSize(string name) {
    struct stat buf;
    int rc = stat((root + "/" + name).c_str(), &buf);
    
    return rc == 0 ? buf.st_size : -1;
}

dataBlock* getDataBlockById(int id) {
    for (int i(0); i < data.size(); i++) {
        if (data[i]->id == id) {
            return data[i];
        }
    }
}

bitmapItem* getBitmapItemById(int id) {
    for (int i(0); i < bitmap.size(); i++) {
        if (bitmap[i]->id == id) {
            return bitmap[i];
        }
    }
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

    // bool flag = mount();

    cout << "descriptors.size()\t" << descriptors.size() << endl;
    cout << "lastId\t" << lastDescriptorId << endl;

    cout << endl << "ls" << endl;

    ls();

    create("321.txt");
    create("123.txt");

    cout << endl << "created file ls" << endl;

    ls();

    descriptor *fd1 = open("123.txt");
    descriptor *fd2 = open("321.txt");

    char **testData = new char*[1];
    testData[0] = "123456789012345";
    
    write(fd1, 0, 1, testData);

    write(fd2, 0, 1, read(fd1, 0, 1));
    
    trunkate("123.txt", 4);

    unmount();
    cout << "unmount()" << endl;

    cout << "descriptors.size()\t" << descriptors.size() << endl;
    cout << "lastId\t" << lastDescriptorId << endl;
    
    return 0;
}