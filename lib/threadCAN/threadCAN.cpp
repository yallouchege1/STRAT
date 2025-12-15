#include "threadCAN.h"


ThreadCAN::ThreadCAN(bool serialEmul, PinName rd, PinName td) : m_readThread(osPriorityHigh), m_dispatchThread(osPriorityHigh), m_writeThread(osPriorityAboveNormal)
{
    m_mailReadMsg = new Mail<CANMessage, CAN_MAIL_BUFFER_SIZE>;
    m_mailWriteMsg = new Mail<CANMessage, CAN_MAIL_BUFFER_SIZE>;
    if (serialEmul) {
        m_can = nullptr;
        m_pc = new BufferedSerial(USBTX, USBRX, 921600);
        m_pc->set_blocking(true);
        m_readThread.start(callback(ThreadCAN::serialThread, this));
    } else {
        m_pc = nullptr;
        m_can = new PrivateCAN(rd, td, 1000000);
        m_can->attach(callback(ThreadCAN::attachThread, this));
    }
    m_dispatchThread.start(callback(ThreadCAN::dispatchThread, this));
    m_writeThread.start(callback(ThreadCAN::writeThread, this));
}



ThreadCAN::~ThreadCAN()
{
    m_writeThread.terminate();
    m_dispatchThread.terminate();
    m_writeThread.join();
    m_dispatchThread.join();
    if (m_pc) {
        m_readThread.terminate();
        m_readThread.join();
        delete m_pc;
    }
    m_pc = nullptr;
    if (m_can) {
        m_can->attach(nullptr);
        delete m_can;
        m_can = nullptr;
    }
    for (unsigned int i=0; i<m_ids.size(); i++) delete m_ids[i];
    m_ids.clear();
    delete m_mailReadMsg;
    m_mailReadMsg = nullptr;
    delete m_mailWriteMsg;
    m_mailWriteMsg = nullptr;
}



void ThreadCAN::attach()
{
    CANMessage *canMsg;
    if (m_mailReadMsg->full()) { // si boite pleine, on vire le plus ancien message
        canMsg = m_mailReadMsg->try_get();
        m_mailReadMsg->free(canMsg);
        m_canFlags.set(CAN_MSG_RD_LOST);
    }
    canMsg = m_mailReadMsg->try_alloc();
    m_can->read(*canMsg);
    m_mailReadMsg->put(canMsg);
}


void ThreadCAN::serialToCanBusMachine()
{
    int etat = 0;
    CANMessage msg;
    uint8_t checksum = 0;
    uint8_t len = 0;
    uint8_t c;

    while (1) {
        m_pc->read(&c, 1);

        switch (etat) {
            case 0:
                checksum = c;
                if (c==0x55) {
                    etat = 1;
                    msg.format = CANStandard;
                    msg.type = CANData;
                } else if (c==0x5A) {
                    etat = 1;
                    msg.format = CANStandard;
                    msg.type = CANRemote;
                } else if (c==0xA5) {
                    etat = 1;
                    msg.format = CANExtended;
                    msg.type = CANData;
                } else if (c==0xAA) {
                    etat = 1;
                    msg.format = CANExtended;
                    msg.type = CANRemote;
                }
                break;
            case 1:
                checksum += c;
                msg.id = c;
                etat = 2;
                break;
            case 2:
                checksum += c;
                msg.id = (msg.id << 8) + c;
                etat = (msg.format == CANExtended) ? 3 : 5;
                break;
            case 3:
                checksum += c;
                msg.id = (msg.id << 8) + c;
                etat = 4;
                break;
            case 4:
                checksum += c;
                msg.id = (msg.id << 8) + c;
                etat = 5;
                break;
            case 5:
                checksum += c;
                msg.len = c;
                if (c>8) {
                    etat = 0; // erreur
                } else {
                    etat = ((msg.type == CANData)&&(c != 0)) ? 6 : 7;
                    len = 0;
                }
                break;
            case 6:
                checksum += c;
                msg.data[len++] = c;
                if (len == msg.len) {
                    etat = 7;
                }
                break;
            case 7:
                checksum = (~checksum);
                etat = 0;
                if (c == checksum) {
                    CANMessage *canMsg;
                    if (m_mailReadMsg->full()) { // si boite pleine, on vire le plus ancien message
                        canMsg = m_mailReadMsg->try_get();
                        m_mailReadMsg->free(canMsg);
                        m_canFlags.set(CAN_MSG_RD_LOST);
                    }
                    canMsg = m_mailReadMsg->try_alloc();
                    *canMsg = msg;
                    m_mailReadMsg->put(canMsg);
                }
                break;
        }
    }
}



void ThreadCAN::sendCanBusToSerial(const CANMessage &msg)
{
    uint8_t msgSerial[15];
    int lenMsgSerial = 0;
    uint8_t checksum;
    if (msg.format == CANStandard) {
        msgSerial[0] = (msg.type == CANData) ? 0x55 : 0x5A;
        msgSerial[1] = (msg.id & 0x00000700) >> 8;
        msgSerial[2] =  msg.id & 0x000000FF;
        msgSerial[3] = msg.len;
        checksum = msgSerial[0] + msgSerial[1] + msgSerial[2] + msgSerial[3];
        lenMsgSerial = 4;
        if (msg.type == CANData) {
            for (int i=0; i<msg.len; i++) {
                msgSerial[4+i] = msg.data[i];
                checksum += msg.data[i];
            }
            lenMsgSerial += msg.len;
        }
    } else {
        msgSerial[0] = (msg.type == CANData) ? 0xA5 : 0xAA;
        msgSerial[1] = (msg.id & 0x1F000000) >> 24;
        msgSerial[2] = (msg.id & 0x00FF0000) >> 16;
        msgSerial[3] = (msg.id & 0x0000FF00) >> 8;
        msgSerial[4] =  msg.id & 0x000000FF;
        msgSerial[5] = msg.len;
        checksum = msgSerial[0] + msgSerial[1] + msgSerial[2] + msgSerial[3]
                   + msgSerial[4] + msgSerial[5];
        lenMsgSerial = 6;
        if (msg.type == CANData) {
            for (int i=0; i<msg.len; i++) {
                msgSerial[6+i] = msg.data[i];
                checksum += msg.data[i];
            }
            lenMsgSerial += msg.len;
        }
    }
    msgSerial[lenMsgSerial++] = ~checksum;
    m_pc->write(msgSerial, lenMsgSerial);
}


void ThreadCAN::registerIds(int idMin, int idMax, void (*func)(CANMessage *))
{
    Registered *r = new Registered(idMin, idMax, reinterpret_cast<void *>(func));
    registeredVector.lock();
    m_ids.push_back(r);
    registeredVector.unlock();
}

void ThreadCAN::registerIds(int idMin, int idMax, void *obj, void (*func)(void *, CANMessage *))
{
    Registered *r = new Registered(idMin, idMax, reinterpret_cast<void *>(func), obj);
    registeredVector.lock();
    m_ids.push_back(r);
    registeredVector.unlock();
}

void ThreadCAN::unRegisterIds(void (*func)(CANMessage *))
{
    registeredVector.lock();
    for (unsigned int i=0; i<m_ids.size(); i++) {
        if ((m_ids[i]->object==nullptr) && (m_ids[i]->method == func)) {
            delete m_ids[i];
            m_ids.erase(m_ids.begin()+i);
            i--;
        }
    }
    registeredVector.unlock();
}

void ThreadCAN::unRegisterIds(void *obj, void (*func)(void *, CANMessage *))
{
    registeredVector.lock();
    for (unsigned int i=0; i<m_ids.size(); i++) {
        if ((m_ids[i]->object==obj) && (m_ids[i]->method == func)) {
            delete m_ids[i];
            m_ids.erase(m_ids.begin()+i);
            i--;
        }
    }
    registeredVector.unlock();
}

void ThreadCAN::dispatch()
{
    while (1) {
        CANMessage *canMsg = m_mailReadMsg->try_get_for(Kernel::wait_for_u32_forever);
        int id = canMsg->id;
        registeredVector.lock();
        for (unsigned int i=0; i<m_ids.size(); i++) {
            if ((id>=m_ids[i]->idMin)&&(id<=m_ids[i]->idMax)) {
                if (m_ids[i]->object != nullptr) {
                    reinterpret_cast<void (*)(void *, CANMessage *)>(m_ids[i]->method)(m_ids[i]->object, canMsg);
                } else {
                    reinterpret_cast<void (*)(CANMessage *)>(m_ids[i]->method)(canMsg);
                }
                break;
            }
        }
        registeredVector.unlock();
        m_mailReadMsg->free(canMsg);
    }
}

void ThreadCAN::write()
{
    while (1) {
        CANMessage *canMsg = m_mailWriteMsg->try_get_for(Kernel::wait_for_u32_forever);
        if (m_can) {
            if (!m_can->write(*canMsg)) {
                bool ok;
                for (int i=0; i<10; i++) {
                    wait_us(10);
                    ok = m_can->write(*canMsg);
                    if (ok) break;
                }
                if (!ok) {
                    while (!m_can->write(*canMsg)) {
                        ThisThread::sleep_for(10ms);
                    }
                }
            }
        } else if (m_pc) {
            sendCanBusToSerial(*canMsg);
        }
        m_mailWriteMsg->free(canMsg);
    }
}

void ThreadCAN::send(const CANMessage &msg)
{
    CANMessage *canMsg;
    if (m_mailWriteMsg->full()) { // si boite pleine, on vire le plus ancien message
        canMsg = m_mailWriteMsg->try_get();
        m_mailWriteMsg->free(canMsg);
        m_canFlags.set(CAN_MSG_WR_LOST);
    }
    canMsg = m_mailWriteMsg->try_alloc();
    *canMsg = msg;
    m_mailWriteMsg->put(canMsg);
}

void ThreadCAN::send(uint32_t id, const char *data, int len)
{
    send(CANMessage(id, data, len));
}

void ThreadCAN::send(uint32_t id)
{
    CANMessage msg;
    msg.id = id;
    msg.len = 0;
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint16_t d1)
{
    CANMessage msg;
    msg.id=id;
    msg.len=2;
    msg.data[0]=(uint8_t)d1;
    msg.data[1]=(uint8_t)(d1>>8);
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint16_t d1, uint16_t d2)
{
    CANMessage msg;
    msg.id=id;
    msg.len=4;
    msg.data[0]=(uint8_t)d1;
    msg.data[1]=(uint8_t)(d1>>8);
    msg.data[2]=(uint8_t)d2;
    msg.data[3]=(uint8_t)(d2>>8);
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint16_t d1, uint16_t d2, uint16_t d3)
{
    CANMessage msg;
    msg.id=id;
    msg.len=6;
    msg.data[0]=(uint8_t)d1;
    msg.data[1]=(uint8_t)(d1>>8);
    msg.data[2]=(uint8_t)d2;
    msg.data[3]=(uint8_t)(d2>>8);
    msg.data[4]=(uint8_t)d3;
    msg.data[5]=(uint8_t)(d3>>8);
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint16_t d1, uint16_t d2, uint16_t d3, uint8_t d4)
{
    CANMessage msg;
    msg.id=id;
    msg.len=7;
    msg.data[0]=(uint8_t)d1;
    msg.data[1]=(uint8_t)(d1>>8);
    msg.data[2]=(uint8_t)d2;
    msg.data[3]=(uint8_t)(d2>>8);
    msg.data[4]=(uint8_t)d3;
    msg.data[5]=(uint8_t)(d3>>8);
    msg.data[6]=d4;
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint16_t d1, uint8_t d2, uint16_t d3, uint8_t d4)
{
    CANMessage msg;
    msg.id=id;
    msg.len=6;
    msg.data[0]=(uint8_t)d1;
    msg.data[1]=(uint8_t)(d1>>8);
    msg.data[2]=(uint8_t)d2;
    msg.data[3]=(uint8_t)d3;
    msg.data[4]=(uint8_t)(d3>>8);
    msg.data[5]=d4;
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint16_t d1, uint16_t d2, uint8_t d3, uint8_t d4)
{
    CANMessage msg;
    msg.id=id;
    msg.len=6;
    msg.data[0]=(uint8_t)d1;
    msg.data[1]=(uint8_t)(d1>>8);
    msg.data[2]=(uint8_t)d2;
    msg.data[3]=(uint8_t)(d2>>8);
    msg.data[4]=d3;
    msg.data[5]=d4;
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint8_t d1)
{
    CANMessage msg;
    msg.id=id;
    msg.len=1;
    msg.data[0]=d1;
    send(msg);    
}

void ThreadCAN::send(uint32_t id, uint8_t d1, uint8_t d2)
{
    CANMessage msg;
    msg.id=id;
    msg.len=2;
    msg.data[0]=d1;
    msg.data[1]=d2;
    send(msg);    
}

void ThreadCAN::send(uint32_t id, uint8_t d1, uint8_t d2, uint8_t d3)
{
    CANMessage msg;
    msg.id=id;
    msg.len=3;
    msg.data[0]=d1;
    msg.data[1]=d2;
    msg.data[2]=d3;
    send(msg);    
}

void ThreadCAN::send(uint32_t id, uint8_t d1, uint8_t d2, uint16_t d3, uint8_t d4, uint8_t d5)
{
    CANMessage msg;
    msg.id=id;
    msg.len=6;
    msg.data[0]=d1;
    msg.data[1]=d2;
    msg.data[2]=(uint8_t)d3;
    msg.data[3]=(uint8_t)(d3>>8);
    msg.data[4]=d4;
    msg.data[5]=d5;
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint8_t d1, uint8_t d2, uint16_t d3, uint8_t d4, uint16_t d5, uint8_t d6)
{
    CANMessage msg;
    msg.id=id;
    msg.len=8;
    msg.data[0]=d1;
    msg.data[1]=d2;
    msg.data[2]=(uint8_t)d3;
    msg.data[3]=(uint8_t)(d3>>8);
    msg.data[4]=d4;
    msg.data[5]=(uint8_t)d5;
    msg.data[6]=(uint8_t)(d5>>8);
    msg.data[7]=d6;
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint8_t d1, uint16_t d2, uint8_t d3)
{
    CANMessage msg;
    msg.id=id;
    msg.len=4;
    msg.data[0]=d1;
    msg.data[1]=(uint8_t)d2;
    msg.data[2]=(uint8_t)(d2>>8);
    msg.data[3]=d3;
    send(msg);
}

void ThreadCAN::send(uint32_t id, uint32_t d1)
{
    CANMessage msg;
    msg.id=id;
    msg.len=4;
    msg.data[0]=(uint8_t)d1;
    msg.data[1]=(uint8_t)(d1>>8);
    msg.data[2]=(uint8_t)(d1>>16);
    msg.data[3]=(uint8_t)(d1>>24);
    send(msg);
}

void ThreadCAN::sendAck(uint32_t id, uint16_t from)
{
    CANMessage msg;
    msg.id = id;
    msg.len = 2;
    msg.data[0]=(uint8_t)from;
    msg.data[1]=(uint8_t)(from>>8);
    send(msg);
}

void ThreadCAN::sendRemote(uint32_t id, int len)
{
    CANMessage msg;
    msg.id = id;
    msg.len = len;
    msg.format = CANStandard;
    msg.type = CANRemote;
    send(msg);
}

