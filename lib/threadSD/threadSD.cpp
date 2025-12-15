#include "threadSD.h"
#include <string>
#include <vector>

#include "SDIOBlockDevice.h"

ThreadSD::ThreadSD(const char *name)
    : m_fs(name), m_superUser(DEFAULT_SUPERUSER_STATUS), m_error(0)
{
    m_tcan = nullptr;
    m_root = new Dossier;
    m_current = m_root;
    m_flagSDCard.clear();
    m_mail.dataSrc = nullptr;
    m_mail.dataDest = nullptr;
    m_mainThread.start(callback(ThreadSD::run, this));
}

ThreadSD::~ThreadSD()
{
    m_flagSDCard.set(FLAG_FORCE_TERMINATE);
    m_mainThread.join();
    delete[] m_mail.dataSrc;
    m_mail.dataSrc = nullptr;
    delete[] m_mail.dataDest;
    m_mail.dataDest = nullptr;
    delete m_root;
    m_root = nullptr;
    m_current = nullptr;
}

void ThreadSD::runSDCard()
{
    int etat = 0;
    static int canTimeout = 0;

// Instantiate the Block Device for sd card on DISCO
    SDIOBlockDevice *bd = nullptr;

    while (1) {
        if (m_flagSDCard.get() & FLAG_FORCE_TERMINATE) return;
        switch (etat) {
            case 0 : {
                m_flagSDCard.set(FLAG_INIT);
                if (bd == nullptr) bd = new SDIOBlockDevice();
                m_error = m_fs.mount(bd);
                if (m_error) {
                    m_flagSDCard.set(FLAG_NO_CARD);
                    m_fs.unmount();
                    delete bd;
                    bd = nullptr;
                    ThisThread::sleep_for(1s);
                } else {
                    ThisThread::sleep_for(500ms);
                    etat = 1;
                }
            }
            break;
            case 1 : {
                m_root->clear();
                m_root->fill(&m_fs);
                m_reply = m_root->serialize(&m_fs);
                etat = 2;
                m_flagSDCard.clear(FLAG_INIT | FLAG_NO_CARD);
                m_flagSDCard.set(FLAG_READY);
            }
            break;
            case 2 : {
                m_flagSDCard.wait_any(FLAG_BUSY | FLAG_CAN_REQUEST, 100, false);
                uint32_t flags = m_flagSDCard.get();
                if (flags & FLAG_BUSY) {
                    etat = 3;
                } else if (flags & FLAG_CAN_REQUEST) {
                    if (m_flagSDCard.wait_all(FLAG_READY, 0) & FLAG_READY) {
                        m_flagSDCard.set(FLAG_CAN_CONTROL);
                        m_flagSDCard.set(FLAG_BUSY);
                        m_flagSDCard.clear(FLAG_CAN_REQUEST);
                        canFileSystem();
                        canTimeout = 0;
                        etat = 4;
                    }
                } else if (!bd->isPresent()) {
                    m_flagSDCard.clear(FLAG_READY);
                    m_flagSDCard.set(FLAG_NO_CARD);
                    etat = 0;
                }
            }
            break;
            case 3 : {
                if (m_mail.message == CMD_LS) {
                    m_mail.reply = cmd_ls();
                } else if (m_mail.message == CMD_CD_ROOT) {
                    m_mail.reply = cmd_cd();
                } else if (m_mail.message == CMD_CD_PARENT) {
                    char p[] = "..";
                    m_mail.reply = cmd_cd(p);
                } else if (m_mail.message == CMD_CD_NUM) {
                    m_mail.reply = cmd_cd(m_mail.num);
                } else if (m_mail.message == CMD_CD_NAME) {
                    m_mail.reply = cmd_cd(m_mail.dataSrc);
                } else if (m_mail.message == CMD_MKDIR) {
                    m_mail.reply = cmd_mkdir(m_mail.dataSrc);
                } else if (m_mail.message == CMD_DEL_NAME) {
                    m_mail.reply = cmd_remove(m_mail.dataSrc);
                }
                m_flagSDCard.clear(FLAG_BUSY);
                m_flagSDCard.set(FLAG_READY);
                etat = 2;
            }
            break;
            case 4 : {
                m_flagSDCard.wait_any(FLAG_CAN_REQUEST, 100, false);
                uint32_t flags = m_flagSDCard.get();
                if (flags & FLAG_CAN_REQUEST) {
                    m_flagSDCard.clear(FLAG_CAN_REQUEST);
                    canFileSystem();
                    canTimeout = 0;
                } else if (!bd->isPresent()) {
                    m_flagSDCard.clear(FLAG_CAN_CONTROL);
                    m_flagSDCard.set(FLAG_NO_CARD);
                    etat = 0;
                } else {
                    canTimeout++;
                    if (canTimeout > 10) { // 1 seconde sans requete CAN
                        m_flagSDCard.clear(FLAG_BUSY);
                        m_flagSDCard.clear(FLAG_CAN_CONTROL);
                        m_flagSDCard.set(FLAG_READY);
                        etat=2;
                    }
                }
            }
            break;
        }
    }
}

Dossier *ThreadSD::getPath(char *path)
{
    char *end;
    Dossier *current, *child;
    m_error = 0;
    if (path == nullptr) {
        return m_root;
    } else if (path[0] == '\0') {
        return m_root;
    } else if (path[0] == '/') {
        current = m_root;
        path++;
        if (*path == '\0') {
            return m_root;
        }
    } else if (strcmp(path, "..") == 0) {
        current = m_current->getParent();
        if (!current) {
            m_error = ERROR_NO_PARENT;
            return nullptr;
        }
        return current;
    } else {
        current = m_current;
    }
    bool encore;
    end = path;
    do {
        while (*end) {
            if (*end == '/') break;
            end++;
        }
        encore = (*end == '/');
        *end = '\0';
        if (!Dossier::isValidDirName(path)) {
            m_error = ERROR_INVALID_NAME;
            return nullptr;
        }
        child = current->getDir(path);
        if (!child) {
            m_error = ERROR_INVALID_PATH;
            return nullptr;
        }
        if (!child->isFilled()) {
            m_error = child->fill(&m_fs);
            if (m_error) {
                m_error = ERROR_FILESYSTEM;
                return nullptr;
            }
        }
        current = child;
        end++;
        path = end;
    } while (encore);
    return current;
}

char *ThreadSD::cmd_ls(char *path)
{
    m_error = 0;
    Dossier *current;
    if (path) {
        current = getPath(path);
        if (!current) current = m_current;
    } else {
        current = m_current;
    }
    m_reply = current->serialize(&m_fs);
    return m_reply;
}

char *ThreadSD::cmd_cd(char *path)
{
    m_error = 0;
    if (path) {
        Dossier *current = getPath(path);
        if (current) m_current = current;
    } else {
        m_current = m_root;
    }
    m_reply = m_current->serialize(&m_fs);
    return m_reply;
}

char *ThreadSD::cmd_cd(int num)
{
    m_error = 0;
    Dossier *child = m_current->getDir(num);
    if (child) {
        m_current = child;
        if (!child->isFilled()) m_error = child->fill(&m_fs);
    } else {
        m_error = ERROR_NO_DIR;
    }
    m_reply =  m_current->serialize(&m_fs);
    return m_reply;
}

char *ThreadSD::cmd_mkdir(char *name, char *path)
{
    m_error = 0;
    Dossier *current;
    if (path) {
        current = getPath(path);
        if (!current) {
            m_error = ERROR_INVALID_PATH;
            return m_reply;
        }
    } else {
        current = m_current;
    }
    if (!current->isFilled()) {
        m_error = current->fill(&m_fs);
        if (m_error) return m_reply;
    }
    if (!Dossier::isValidDirName(name)) {
        m_error = ERROR_INVALID_NAME;
        return m_reply;
    }
    if (current->fileExist(name) || current->dirExist(name)) {
        m_error = ERROR_ALREADY_EXIST;
        return m_reply;
    }
    string tmp = current->getFullName()+string("/")+string(name);
    m_error = m_fs.mkdir(tmp.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (m_error) {
        m_error = ERROR_FILESYSTEM;
        return m_reply;
    }
    m_current = current->addDir(name);
    m_reply = m_current->serialize(&m_fs);
    return m_reply;
}

char *ThreadSD::cmd_mkdir(char *newDir)
{
    char *slash = nullptr;
    char *p = newDir;
    while (*p) {
        if (*p == '/') slash = p;
        p++;
    }
    if (!slash) return cmd_mkdir(newDir, nullptr);
    *slash = '\0';
    slash++;
    return cmd_mkdir(slash, newDir);
}

char *ThreadSD::cmd_remove(char *name, char *path)
{
    m_error = 0;
    Dossier *current;
    if (path) {
        current = getPath(path);
        if (!current) {
            m_error = ERROR_INVALID_PATH;
            return m_reply;
        }
    } else {
        current = m_current;
    }
    if (!current->isFilled()) {
        m_error = current->fill(&m_fs);
        if (m_error) return m_reply;
    }
    Dossier *childDir = current->getDir(name);
    if (childDir) {
        m_error = m_fs.remove(childDir->getFullName().c_str());
        if (m_error) {
            m_error = ERROR_REMOVE;
            return m_reply;
        }
        current->rmDir(childDir);
        m_reply = current->serialize(&m_fs);
        return m_reply;
    }
    Fichier *childFile = current->getFile(name);
    if (childFile) {
        m_error = m_fs.remove(childFile->getFullName().c_str());
        if (m_error) {
            m_error = ERROR_REMOVE;
            return m_reply;
        }
        current->rmFile(childFile);
        m_reply = current->serialize(&m_fs);
        return m_reply;
    }
    m_error = ERROR_SOURCE_NOT_FOUND;
    return m_reply;
}

char *ThreadSD::cmd_remove(char *src)
{
    char *slash = nullptr;
    char *p = src;
    while (*p) {
        if (*p == '/') slash = p;
        p++;
    }
    if (!slash) return cmd_remove(src, nullptr);
    *slash = '\0';
    slash++;
    return cmd_remove(slash, src);
}

char *ThreadSD::cmd_rename(char *srcName, char *destName, char *srcPath, char *destPath)
{
    m_error = 0;
    Dossier *parentSrc;
    if (srcPath) {
        parentSrc = getPath(srcPath);
        if (!parentSrc) {
            m_error = ERROR_INVALID_PATH;
            return m_reply;
        }
    } else {
        parentSrc = m_current;
    }
    if (!parentSrc->isFilled()) {
        m_error = parentSrc->fill(&m_fs);
        if (m_error) return m_reply;
    }
    Dossier *parentDest;
    if (destPath) {
        parentDest = getPath(destPath);
        if (!parentDest) {
            m_error = ERROR_INVALID_PATH;
            return m_reply;
        }
    } else {
        parentDest = parentSrc;
    }
    if (!parentDest->isFilled()) {
        m_error = parentDest->fill(&m_fs);
        if (m_error) return m_reply;
    }
    if (parentDest->fileExist(destName) || parentDest->dirExist(destName)) {
        m_error = ERROR_ALREADY_EXIST;
        return m_reply;
    }
    Dossier *srcDir = parentSrc->getDir(srcName);
    if (srcDir) {
        if (parentSrc == parentDest) {
            string srcTmp = srcDir->getFullName();
            string undo = srcDir->getName();
            srcDir->setName(destName);
            string destTmp = srcDir->getFullName();
            m_error = m_fs.rename(srcTmp.c_str(), destTmp.c_str());
            if (m_error) {
                m_error = ERROR_RENAME;
                srcDir->setName(undo.c_str());
            }
            m_reply = parentSrc->serialize(&m_fs);
            return m_reply;
        } else {
// non implémenté
        }
    }
    Fichier *srcFile = parentSrc->getFile(srcName);
    if (srcFile) {
        if (parentSrc == parentDest) {
            string srcTmp = srcFile->getFullName();
            string undo = srcFile->getName();
            srcFile->setName(destName);
            string destTmp = srcFile->getFullName();
            m_error = m_fs.rename(srcTmp.c_str(), destTmp.c_str());
            if (m_error) {
                m_error = ERROR_RENAME;
                srcFile->setName(undo.c_str());
            }
            m_reply = parentSrc->serialize(&m_fs);
            return m_reply;
        } else {
// non implémenté
        }
    }
    m_error = ERROR_SOURCE_NOT_FOUND;
    return m_reply;
}

char *ThreadSD::cmd_rename(char *src)
{
    char *separator = src;
    while (*separator) {
        if (*separator == ':') break;
        separator++;
    }
    if (*separator == '\0') {
        m_error = ERROR_NO_DEST;
        return m_reply;
    }
    *separator = '\0';
    char *destPath = separator + 1;
    char *destName = nullptr;
    char *p = destPath;
    while (*p) {
        if (*p == '/') destName = p;
        p++;
    }
    if (!destName) {
        destName = destPath;
        destPath = nullptr;
    } else {
        *destName = '\0';
        destName++;
    }
    char *srcName = nullptr;
    p = src;
    while (*p) {
        if (*p == '/') srcName = p;
        p++;
    }
    if (!srcName) {
        srcName = src;
        src = nullptr;
    } else {
        *srcName = '\0';
        srcName++;
    }
    return cmd_rename(srcName, destName, src, destPath);
}

char *ThreadSD::cmd_mkfile(char *name, char *path)
{
    m_error = 0;
    Dossier *current;
    if (path) {
        current = getPath(path);
        if (!current) {
            m_error = ERROR_INVALID_PATH;
            return m_reply;
        }
    } else {
        current = m_current;
    }
    if (!current->isFilled()) {
        m_error = current->fill(&m_fs);
        if (m_error) return m_reply;
    }
    if (!Fichier::isValidFileName(name)) {
        m_error = ERROR_INVALID_NAME;
        return m_reply;
    }
    if (current->fileExist(name) || current->dirExist(name)) {
        m_error = ERROR_ALREADY_EXIST;
        return m_reply;
    }
    string tmp = current->getFullName()+string("/")+string(name);
    File file;
    m_error = file.open(&m_fs, tmp.c_str(), O_RDWR | O_CREAT);
    if (m_error) {
        m_error = ERROR_FILESYSTEM;
        return m_reply;
    }
    file.close();
    current->addFile(name);
    m_reply = m_current->serialize(&m_fs);
    return m_reply;
}

char *ThreadSD::cmd_mkfile(char *newFile)
{
    char *slash = nullptr;
    char *p = newFile;
    while (*p) {
        if (*p == '/') slash = p;
        p++;
    }
    if (!slash) return cmd_mkfile(newFile, nullptr);
    *slash = '\0';
    slash++;
    return cmd_mkfile(slash, newFile);
}

char *ThreadSD::cmd_uploadExist(char *file)
{
    m_currentFile.set_blocking(true);
    m_error = m_currentFile.open(&m_fs, file, O_WRONLY | O_TRUNC);
    if (m_error) m_error = ERROR_OPEN_FILE_WRITE;
    char *slash = nullptr;
    char *p = file;
    while (*p) {
        if (*p == '/') slash = p;
        p++;
    }
    if (!slash) {
        m_current = m_root;
        return cmd_ls();
    }
    *slash = '\0';
    return cmd_ls(file);
}

char *ThreadSD::cmd_uploadCreate(char *newFile)
{
    string source(newFile);
    char *rep = cmd_mkfile(newFile);
    if (m_error) return rep;
    m_currentFile.set_blocking(true);
    m_error = m_currentFile.open(&m_fs, source.c_str(), O_WRONLY | O_TRUNC);
    if (m_error) m_error = ERROR_OPEN_FILE_WRITE;
    return rep;
}

int32_t ThreadSD::cmd_download(char *file)
{
    int32_t size = 0;
    m_currentFile.set_blocking(true);
    m_error = m_currentFile.open(&m_fs, file);
    if (m_error) {
        m_error = ERROR_OPEN_FILE_READ;
    } else {
        size = m_currentFile.size();
    }
    return size;
}

char *ThreadSD::cmd_copy(char *src, char *dest, bool forceExist)
{
    m_error = 0;
    Fichier *f = m_root->findFile(src);
    if (!f) {
        m_error = ERROR_SOURCE_NOT_FOUND;
        return m_reply;
    }
    if (!f->isFile()) {
        m_error = ERROR_SOURCE_NOT_FOUND;
        return m_reply;
    }
    string destDir = Fichier::dirOfFile(dest);
    string destName = Fichier::nameOfFile(dest);
    Dossier *d = m_root->findDir(destDir);
    if (!d) {
        m_error = ERROR_NO_DEST;
        return m_reply;
    }
    m_reply = d->serialize(&m_fs);
    if (!Fichier::isValidFileName(destName)) {
        m_error = ERROR_INVALID_NAME;
        return m_reply;
    }
    if (d->dirExist(destName)) {
        m_error = ERROR_INVALID_NAME;
        return m_reply;
    }
    File srcFile, destFile;
    srcFile.set_blocking(true);
    m_error = srcFile.open(&m_fs, src);
    if (m_error) {
        m_error = ERROR_OPEN_FILE_READ;
        return m_reply;
    }
    destFile.set_blocking(true);
    if (d->fileExist(destName)) {
        if (!forceExist) {
            m_error = ERROR_ALREADY_EXIST;
            return m_reply;
        }
        m_error = destFile.open(&m_fs, dest, O_WRONLY | O_TRUNC);
        if (m_error) {
            m_error = ERROR_OPEN_FILE_WRITE;
            return m_reply;
        }
    } else {
        m_error = destFile.open(&m_fs, dest, O_WRONLY | O_CREAT);
        if (m_error) {
            m_error = ERROR_OPEN_FILE_CREATE;
            return m_reply;
        }
        d->addFile(destName);
        m_reply = d->serialize(&m_fs);
    }
    int ok;
    char c;
    while (1) {
        ok = srcFile.read(&c, 1);
        if (ok < 0) {
            m_error = ERROR_READ_FILE;
            return m_reply;
        }
        if (ok == 0) break;
        ok = destFile.write(&c, 1);
        if (ok < 0) {
            m_error = ERROR_WRITE_FILE;
            return m_reply;
        }
    }
    return m_reply;
}

char *ThreadSD::cmd_copy(char *src, bool forceExist)
{
    char *separator = src;
    while (*separator) {
        if (*separator == ':') break;
        separator++;
    }
    if (*separator == '\0') {
        m_error = ERROR_NO_DEST;
        return m_reply;
    }
    *separator = '\0';
    char *destPath = separator + 1;
    return cmd_copy(src, destPath, forceExist);
}

void ThreadSD::remoteCANControl(CANMessage *msg)
{
    m_canMsg = *msg;
    m_flagSDCard.set(FLAG_CAN_REQUEST);
}

#define ID_FS_0     (m_idBase)
#define ID_FS_1     (m_idBase+1)
#define ID_FS_2     (m_idBase+2)
#define ID_FS_3     (m_idBase+3)

void ThreadSD::registerCANControl(ThreadCAN &tcan, int idBase)
{
    m_idBase = idBase;
    m_tcan = &tcan;
    auto f = static_cast<void (*)(void *, CANMessage *)>(&ThreadSD::remoteCANControl);
    tcan.registerIds(m_idBase, m_idBase+3, this, f);
}

void ThreadSD::unRegisterCANControl(ThreadCAN &tcan)
{
    auto f = static_cast<void (*)(void *, CANMessage *)>(&ThreadSD::remoteCANControl);
    tcan.unRegisterIds(this, f);
}

void ThreadSD::sendReplyStartFrame(int size)
{
    memcpy(m_canMsg.data, &size, 4);
    m_canMsg.id = ID_FS_1;
    m_canMsg.format = CANStandard;
    m_canMsg.type = CANData;
    m_canMsg.len = 4;
    if (m_error) {
        m_canMsg.len = 6;
        memcpy(&m_canMsg.data[4], &m_error, 2);
    }
    m_tcan->send(m_canMsg);
}

int ThreadSD::sendDownloadStartFrame(int totalSize, bool first)
{
    int packetSize = 1024;
    if (packetSize > totalSize) packetSize = totalSize;
    if (first) {
        memcpy(&m_canMsg.data[4], &totalSize, 3);
        m_canMsg.len = 7;
    } else {
        if (totalSize == 0) {
            m_currentFile.close();
            return 0;
        }
        m_canMsg.len = 4;
    }
    memcpy(m_canMsg.data, &packetSize, 4);
    m_canMsg.id = ID_FS_2;
    m_canMsg.format = CANStandard;
    m_canMsg.type = CANData;
    if (m_error) {
        m_canMsg.len = 6;
        memcpy(&m_canMsg.data[4], &m_error, 2);
    }
    m_tcan->send(m_canMsg);
    return packetSize;
}

void ThreadSD::canFileSystem()
{
    static char *reply;
    static int replySize;
    static int memoCmd;
    static int sizeSrc, recuSrc;
    static char *dataSrc = nullptr;
    static char memo[50];
    static int totalSize = 0;

    if (m_canMsg.id == (unsigned int)ID_FS_0) {
        if (m_canMsg.type == CANData) {
            if (m_canMsg.len == 8) {
                int n = (recuSrc+8 <= sizeSrc) ? 8 : sizeSrc-recuSrc;
                memcpy(&dataSrc[recuSrc], m_canMsg.data, n);
                recuSrc += n;
                m_tcan->sendRemote(ID_FS_0);
                if (recuSrc == sizeSrc) {
                    dataSrc[sizeSrc] = '\0';
                    if (memoCmd == ThreadSD::CMD_MKDIR) {
                        reply = cmd_mkdir(dataSrc);
                    } else if (memoCmd == ThreadSD::CMD_DEL_NAME) {
                        reply = cmd_remove(dataSrc);
                    } else if (memoCmd == ThreadSD::CMD_RENAME) {
                        reply = cmd_rename(dataSrc);
                    } else if (memoCmd == ThreadSD::CMD_MKFILE) {
                        reply = cmd_mkfile(dataSrc);
                    } else if (memoCmd == ThreadSD::CMD_UPLOAD_EXIST) {
                        reply = cmd_uploadExist(dataSrc);
                    } else if (memoCmd == ThreadSD::CMD_UPLOAD_CREATE) {
                        reply = cmd_uploadCreate(dataSrc);
                    } else if (memoCmd == ThreadSD::CMD_COPY_EXIST) {
                        reply = cmd_copy(dataSrc, true);
                    } else if (memoCmd == ThreadSD::CMD_COPY_CREATE) {
                        reply = cmd_copy(dataSrc);
                    }
                    if (memoCmd == ThreadSD::CMD_DOWNLOAD) {
                        totalSize = cmd_download(dataSrc);
                        replySize = sendDownloadStartFrame(totalSize, true);
                    } else {
                        replySize = strlen(reply);
                        sendReplyStartFrame(replySize);
                    }
                }
            } else if ((m_canMsg.len == 1)||(m_canMsg.len == 2)) {
                if (m_canMsg.data[0] == ThreadSD::CMD_LS) {
                    reply = cmd_ls();
                } else if (m_canMsg.data[0] == ThreadSD::CMD_CD_ROOT) {
                    reply = cmd_cd();
                } else if (m_canMsg.data[0] == ThreadSD::CMD_CD_PARENT) {
                    char p[] = "..";
                    reply = cmd_cd(p);
                } else if (m_canMsg.data[0] == ThreadSD::CMD_CD_NUM) {
                    reply = cmd_cd(m_canMsg.data[1]);
                }
                replySize = strlen(reply);
                sendReplyStartFrame(replySize);
            } else if (m_canMsg.len == 5) {
                memcpy(&sizeSrc, m_canMsg.data, 4);
                memoCmd = m_canMsg.data[4];
                delete[] dataSrc;
                dataSrc = new char[sizeSrc+1];
                recuSrc = 0;
                m_tcan->sendRemote(ID_FS_0);
            }
        }
    } else if (m_canMsg.id == (unsigned int)ID_FS_1) {
        if (m_canMsg.type == CANRemote) {
            if (replySize > 0) {
                m_tcan->send(ID_FS_1, reply);
                reply += 8;
                replySize -= 8;
            }
        }
    } else if (m_canMsg.id == (unsigned int)ID_FS_2) {
        if (m_canMsg.type == CANData) {
            if (m_canMsg.len == 8) {
                int n = (recuSrc+8 <= sizeSrc) ? 8 : sizeSrc-recuSrc;
                m_currentFile.write(m_canMsg.data, n);
                recuSrc += n;
                m_tcan->sendRemote(ID_FS_2);
                if (recuSrc == sizeSrc) {
                    if (m_lastDatas) {
                        int taille = m_currentFile.size();
                        sprintf(memo, "%d octets", taille);
                        m_currentFile.close();
                        reply = memo;
                        replySize = strlen(reply);
                        sendReplyStartFrame(replySize);
                    }
                }
            } else if ((m_canMsg.len == 4) || (m_canMsg.len == 5)) {
                memcpy(&sizeSrc, m_canMsg.data, 4);
                recuSrc = 0;
                m_lastDatas = m_canMsg.len == 5;
                m_tcan->sendRemote(ID_FS_2);
            }
        } else if (m_canMsg.type == CANRemote) {
            if (replySize > 0) {
                int n = (replySize > 8) ? 8 : replySize;
                m_currentFile.read(m_canMsg.data, n);
                m_canMsg.len = 8;
                m_canMsg.type = CANData;
                m_tcan->send(m_canMsg);
                replySize -= n;
                totalSize -= n;
            } else {
                replySize = sendDownloadStartFrame(totalSize);
            }
        }
    }
    //m_tcan->send(m_canMsg);
}

char *ThreadSD::ls(uint32_t millisec)
{
    if (m_flagSDCard.wait_all(FLAG_READY, 0) & FLAG_READY) {
        m_mail.message = CMD_LS;
        m_flagSDCard.set(FLAG_BUSY);
        if (waitReady(millisec)) return getReply();
    }
    return nullptr;
}

char *ThreadSD::cdRoot(uint32_t millisec)
{
    if (m_flagSDCard.wait_all(FLAG_READY, 0) & FLAG_READY) {
        m_mail.message = CMD_CD_ROOT;
        m_flagSDCard.set(FLAG_BUSY);
        if (waitReady(millisec)) return getReply();
    }
    return nullptr;
}

char *ThreadSD::cdParent(uint32_t millisec)
{
    if (m_flagSDCard.wait_all(FLAG_READY, 0) & FLAG_READY) {
        m_mail.message = CMD_CD_PARENT;
        m_flagSDCard.set(FLAG_BUSY);
        if (waitReady(millisec)) return getReply();
    }
    return nullptr;
}

char *ThreadSD::cdNum(int n, uint32_t millisec)
{
    if (m_flagSDCard.wait_all(FLAG_READY, 0) & FLAG_READY) {
        m_mail.message = CMD_CD_NUM;
        m_mail.num = n;
        m_flagSDCard.set(FLAG_BUSY);
        if (waitReady(millisec)) return getReply();
    }
    return nullptr;
}

char *ThreadSD::cdName(const char *n, uint32_t millisec)
{
    if (m_flagSDCard.wait_all(FLAG_READY, 0) & FLAG_READY) {
        m_mail.message = CMD_CD_NAME;
        setDataSrc(n);
        m_flagSDCard.set(FLAG_BUSY);
        if (waitReady(millisec)) return getReply();
    }
    return nullptr;
}

char *ThreadSD::mkdir(const char *n, uint32_t millisec)
{
    if (m_flagSDCard.wait_all(FLAG_READY, 0) & FLAG_READY) {
        m_mail.message = CMD_MKDIR;
        setDataSrc(n);
        m_flagSDCard.set(FLAG_BUSY);
        if (waitReady(millisec)) return getReply();
    }
    return nullptr;
}

char *ThreadSD::rmName(const char *n, uint32_t millisec)
{
    if (m_flagSDCard.wait_all(FLAG_READY, 0) & FLAG_READY) {
        m_mail.message = CMD_DEL_NAME;
        setDataSrc(n);
        m_flagSDCard.set(FLAG_BUSY);
        if (waitReady(millisec)) return getReply();
    }
    return nullptr;
}

bool ThreadSD::waitReady(uint32_t millisec)
{
    return m_flagSDCard.wait_all(FLAG_READY, millisec, false)&FLAG_READY;
}

void ThreadSD::setDataSrc(const char *init, unsigned int size)
{
    delete[] m_mail.dataSrc;
    if (size == 0) size = strlen(init)+1;
    m_mail.srcSize = size;
    m_mail.dataSrc = new char[size];
    memcpy(m_mail.dataSrc, init, size);
}

void ThreadSD::setDataDest(const char *init, unsigned int size)
{
    delete[] m_mail.dataDest;
    if (size == 0) size = strlen(init)+1;
    m_mail.destSize = size;
    m_mail.dataDest = new char[size];
    memcpy(m_mail.dataDest, init, size);
}
