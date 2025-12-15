#ifndef __THREADCAN_H
#define __THREADCAN_H

#include <vector>
#include "mbed.h"
#include "privateCAN.h"

#define CAN_MAIL_BUFFER_SIZE    100

class ThreadCAN {
protected:
    class Registered {
        public:
            int idMin, idMax;
            void *object;
            void *method;
            Registered(int min, int max, void *f, void *o = nullptr) :
                idMin(min), idMax(max), object(o), method(f) {}
    };
    vector<Registered *> m_ids; 
    Mutex registeredVector;
    Mail<CANMessage, CAN_MAIL_BUFFER_SIZE> *m_mailReadMsg;
    Mail<CANMessage, CAN_MAIL_BUFFER_SIZE> *m_mailWriteMsg;
    BufferedSerial *m_pc;
    PrivateCAN *m_can;
    enum CAN_FLAGS {
        CAN_MSG_RD_LOST = 1,
        CAN_MSG_WR_LOST = 2
    };
    EventFlags m_canFlags;
    void serialToCanBusMachine();
    void attach();
    static void attachThread(ThreadCAN *p) { p->attach(); }
    static void serialThread(ThreadCAN *p) { p->serialToCanBusMachine(); }
    void sendCanBusToSerial(const CANMessage &msg);
    void dispatch();
    static void dispatchThread(ThreadCAN *p) { p->dispatch(); }
    void write();
    static void writeThread(ThreadCAN *p) { p->write(); }
    Thread m_readThread, m_dispatchThread, m_writeThread;
public:
// Configuration du CAN sur CAN2 car CAN1 incompatible avec le TouchScreen
    ThreadCAN(bool serialEmul = false, PinName rd = PB_5, PinName td = PB_13);
    ~ThreadCAN();
    void registerIds(int idMin, int idMax, void (*func)(CANMessage *));
    void registerIds(int idMin, int idMax, void *obj, void (*func)(void *, CANMessage *));
    void unRegisterIds(void (*func)(CANMessage *));
    void unRegisterIds(void *obj, void (*func)(void *, CANMessage *));
    void send(const CANMessage &msg);
    void send(uint32_t id, const char *data, int len = 8);
    void send(uint32_t id);
    void send(uint32_t id, uint8_t d1);
    void send(uint32_t id, uint16_t d1);
    void send(uint32_t id, uint32_t d1);
    void send(uint32_t id, uint8_t d1, uint8_t d2);
    void send(uint32_t id, uint16_t d1, uint16_t d2);
    void send(uint32_t id, uint8_t d1, uint8_t d2, uint8_t d3);
    void send(uint32_t id, uint8_t d1, uint16_t d2, uint8_t d3);
    void send(uint32_t id, uint16_t d1, uint16_t d2, uint16_t d3);
    void send(uint32_t id, uint16_t d1, uint8_t d2, uint16_t d3, uint8_t d4);
    void send(uint32_t id, uint16_t d1, uint16_t d2, uint8_t d3, uint8_t d4);
    void send(uint32_t id, uint16_t d1, uint16_t d2, uint16_t d3, uint8_t d4);
    void send(uint32_t id, uint8_t d1, uint8_t d2, uint16_t d3, uint8_t d4, uint8_t d5);
    void send(uint32_t id, uint8_t d1, uint8_t d2, uint16_t d3, uint8_t d4, uint16_t d5, uint8_t d6);
    void sendAck(uint32_t id, uint16_t from);
    void sendRemote(uint32_t id, int len = 0);
};

#endif
