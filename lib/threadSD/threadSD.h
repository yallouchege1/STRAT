#ifndef __THREADSD_H
#define __THREADSD_H

#include "mbed.h"
#include <FATFileSystem.h>
#include "threadCAN.h"
#include "fichiers.h"

/*  Utilise 4 ID CAN consécutifs                */ 
/*  Valeur par defaut : 0x3F0 à 0x3F3 inclus    */
const int ID_FILESYSTEM = 0x3F0;

const bool DEFAULT_SUPERUSER_STATUS = true;

class ThreadSD
{
public:
    enum FlagSD {
        FLAG_INIT=1,
        FLAG_NO_CARD=2,
        FLAG_READY=4,
        FLAG_BUSY=8,
        FLAG_CAN_REQUEST=16,
        FLAG_CAN_CONTROL=32,

        FLAG_FORCE_TERMINATE=128
    };
    enum SDMsgConstants {
        CMD_LS=1,
        CMD_CD_ROOT=2,
        CMD_CD_PARENT=3,
        CMD_CD_NUM=4,
        CMD_CD_NAME=5,
        CMD_MKDIR=6,
        CMD_DEL_NAME=7,
        CMD_RENAME=8,
        CMD_MKFILE=9,
        CMD_UPLOAD_EXIST=10,
        CMD_UPLOAD_CREATE=11,
        CMD_DOWNLOAD=12,
        CMD_COPY_EXIST=13,
        CMD_COPY_CREATE=14
    };
protected:
    typedef struct {
        int message;
        int num;
        int srcSize;
        char *dataSrc;
        int destSize;
        char *dataDest;
        char *reply;
    } MsgSD;
    EventFlags m_flagSDCard;
    MsgSD m_mail;
    FATFileSystem m_fs;
    void runSDCard();
    static void run(ThreadSD *p)
    {
        p->runSDCard();
    }
    Thread m_mainThread;
    Dossier *m_root, *m_current;
    char *m_reply;
    bool m_superUser;
    int16_t m_error;
    CANMessage m_canMsg;
    ThreadCAN *m_tcan;
    File m_currentFile;
    bool m_lastDatas;
    void setDataSrc(const char *init, unsigned int size = 0);
    void setDataDest(const char *init, unsigned int size = 0);
    void remoteCANControl(CANMessage *msg);
    static void remoteCANControl(void *p, CANMessage *msg)
    {
        static_cast<ThreadSD *>(p)->remoteCANControl(msg);
    }
    int m_idBase;
    void canFileSystem();
    Dossier *getPath(char *path);
    void sendReplyStartFrame(int size);
    char *cmd_ls(char *path = nullptr);
    char *cmd_cd(char *path = nullptr);
    char *cmd_cd(int num);
    char *cmd_mkdir(char *name, char *path);
    char *cmd_mkdir(char *newDir);
    char *cmd_remove(char *name, char *path);
    char *cmd_remove(char *src);
    char *cmd_rename(char *srcName, char *destName, char *srcPath = nullptr, char *destPath = nullptr);
    char *cmd_rename(char *src);
    char *cmd_mkfile(char *newFile);
    char *cmd_mkfile(char *name, char *path);
    char *cmd_uploadExist(char *file);
    char *cmd_uploadCreate(char *newFile);
    int32_t cmd_download(char *file);
    char *cmd_copy(char *src, char *dest, bool forceExist = false);
    char *cmd_copy(char *src, bool forceExist = false);
    int sendDownloadStartFrame(int totalSize, bool first = false);
public:
    enum ErrorCode {
        ERROR_NO_ERROR=0,
        ERROR_NO_PARENT,
        ERROR_NO_DIR,
        ERROR_CANT_CREATE_DIR,
        ERROR_INVALID_NAME,
        ERROR_INVALID_PATH,
        ERROR_FILESYSTEM,
        ERROR_ALREADY_EXIST,
        ERROR_SOURCE_NOT_FOUND,
        ERROR_RENAME,
        ERROR_NO_DEST,
        ERROR_REMOVE,
        ERROR_OPEN_FILE_WRITE,
        ERROR_OPEN_FILE_READ,
        ERROR_OPEN_FILE_CREATE,
        ERROR_READ_FILE,
        ERROR_WRITE_FILE,
        ERROR_ONLY_SUPERUSER_CMD,
        ERROR_COUNT
    };
    ThreadSD(const char *name = "sd");
    ~ThreadSD();
    char *ls(uint32_t millisec = osWaitForever);
    char *cdRoot(uint32_t millisec = osWaitForever);
    char *cdParent(uint32_t millisec = osWaitForever);
    char *cdNum(int n, uint32_t millisec = osWaitForever);
    char *cdName(const char *n, uint32_t millisec = osWaitForever);
    char *mkdir(const char *n, uint32_t millisec = osWaitForever);
    char *rmName(const char *n, uint32_t millisec = osWaitForever);
    bool waitReady(uint32_t millisec = osWaitForever);
    char *getReply()
    {
        return m_mail.reply;
    }
    uint32_t status()
    {
        return m_flagSDCard.get();
    }
    int lastError()
    {
        return m_error;
    }
    void setSuperUserStatus(bool s=true)
    {
        m_superUser = s;
    }
    void registerCANControl(ThreadCAN &tcan, int idBase = ID_FILESYSTEM);
    void unRegisterCANControl(ThreadCAN &tcan);
};

#endif
