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
    int numSlots; 
    int usedSlots; 
    int id; 
    int status; 
    int maxMessageSize;
    MailQueue mailQueue; 
    ProcQueue producers;
    ProcQueue consumers;

};

/**
 * 
 */
struct Message {
    int mboxID;
    int size;
    char msg[MAX_MESSAGE];  // actual message
    Message* next;          // for the queue
};

/**
 * 
 */
struct Process {
    int pid;       // pid for blocks/unblocks
    Process* senderNext; // for the queue
    Process* receiverNext;
    Message* msgSlot;
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

// Helpers
void cleanMailbox(int);
void cleanSlot(int);
void cleanShadowEntry(int);

// ----- Global data structures/vars
Mailbox mailboxes[MAXMBOX];
Message mailslots[MAX_MESSAGE];
Process shadowTable[MAXPROC];

void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs *args); // syscalls

/**
 * testing my branch
 */
void phase2_init(void) {

    USLOSS_IntVec[USLOSS_TERM_INT] = terminalHandler;
    USLOSS_IntVec[USLOSS_SYSCALL_INt] = syscallHandler;
    USLOSS_IntVec[USLOSS_CLOCK_int] = phase2_clockHandler;

    for (int i = 0; i < MAXMBOX; i++) {
        cleanMailbox(i);
    }

    for (int i = 0; i < MAXSLOTS; i++) {
        cleanSlot(i);
    }

    for (int i = 0; i < MAXPROC; i++) {
        cleanShadowEntry(i);
    }

    for (int i = 0; i < MAXSYSCALLS; i++) {
        systemCallVector[i] = nullsys;
    }

    pidIncrementer = 0;
}

/**
 * Since we do not use any service processes, this function is blank. 
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

// ----- Helpers

/**
 * 
 */
void cleanMailbox(int slot) {
    mailboxes[slot].numSlots = 0;
    mailboxes[slot].usedSlots = 0;
    mailboxes[slot].id = -1;
    mailboxes[slot].status = 0;

    mailboxes[slot].mailQueue = NULL;
    mailboxes[slot].producers = NULL;
    mailboxes[slot].consumers = NULL;
}

/**
 * 
 */
void cleanSlot(int slot) {
    mailslots[slot].msg[0] = '\0';
    mailslots[slot].next = NULL;
    mailslots[slot].size = 0;
    mailslots[slot].mboxID = -1;
}

/**
 * 
 */
void cleanShadowEntry(int slot) {
    shadowTable[slot].pid		= -1;
	shadowTable[slot].state		= 0;
	shadowTable[slot].receiverNext = NULL;
	shadowTable[slot].senderNext = NULL;
	shadowTable[slot].msgSlot	= NULL;
}
