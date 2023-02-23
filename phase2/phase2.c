/**
 * AUTHORS:    Kevin Nisterenko and Rey Sanayei
 * COURSE:     CSC 452, Spring 2023
 * INSTRUCTOR: Russell Lewis
 * ASSIGNMENT: Phase2
 * DUE_DATE:   03/02/2023
 * 
 * This project implements a mailbox system for IPC. It handles both the sending
 * and receiving of messages with or without payload as a way to mimic process 
 * communication in an operating system. 
 */

// ----- Includes
#include <usloss.h>
#include <phase2.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// typedefs
typedef struct Mailbox Mailbox; 
typedef struct Message Message;
typedef struct Process Process;
typedef struct MailQueue MailQueue;
typedef struct ProcQueue ProcQueue;

// ----- Structs

/**
 * 
 */
struct Mailbox {
    MailQueue mailQueue; 
    ProcQueue producers;
    ProcQueue consumers;
};

/**
 * 
 */
struct Message {
    char msg[MAX_MESSAGE];  // actual message
    Message* next;          // for the queue
};

/**
 * 
 */
struct Process {
    int pid;       // pid for blocks/unblocks
    Process* next; // for the queue
};

/**
 * 
 */
struct MailQueue {
    int size; 
    Message* head;
    Message* tail;
};

/**
 * 
 */
struct ProcQueue {
    int size; 
    Process* head;
    Process* tail;
};

// ----- Function Prototypes
// Phase 2 Bootload
void phase2_init(void);
void phase2_start_service_processes(void);
int phase2_check_io(void);
void phase2_clockHandler(void);

// Messaging System 
int MboxCreate(int slots,int slot_size);
int MboxRelease(int mbox_id);
int MboxSend(int mbox_id, void *msg_ptr,int msg_size);
int MboxRecv(int mbox_id, void *msg_ptr,int msg_max_size);
int MboxCondSend(int mbox_id, void *msg_ptr,int msg_size);
int MboxCondRecv(int mbox_id, void *msg_ptr,int msg_max_size);
void waitDevice(int type,int unit,int *status);
void wakeupByDevice(int type,int unit,int status);

// is this supposed to be here ?? void (*systemCallVec[])(USLOSS_Sysargs *args);

// ----- Global data structures/vars
Mailbox mailboxes[MAXMBOX];
Message mailslots[MAX_MESSAGE];

/**
 * testing my branch
 */
void phase2_init(void) {

}

/**
 * 
 */
void phase2_start_service_processes(void) {

}

/**
 * 
 */
int phase2_check_io(void) {
    return 0;
}

/**
 * 
 */
void phase2_clockHandler(void) {

}

/**
 * 
 */
int MboxCreate(int slots, int slot_size) {
    return 0;
}

/**
 * 
 */
int MboxRelease(int mbox_id) {
    return 0;
}

/**
 * 
 */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size) {
    return 0;
}

/**
 * 
 */
int MboxRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
    return 0;
}

/**
 * 
 */
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size) {
    return 0;
}

/**
 * 
 */
int MboxCondRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
    return 0;
}

/**
 * 
 */
void waitDevice(int type, int unit, int *status) {

}

/**
 * 
 */
void wakeupByDevice(int type, int unit, int status) {

}