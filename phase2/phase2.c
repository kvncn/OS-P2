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

// ----- Constants 
#define FREE 0
#define IN_USE 1

// ----- Includes
#include <usloss.h>
#include <phase2.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ----- typedefs
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
    int status;
    char msg[MAX_MESSAGE];  // actual message
    Message* next;          // for the queue
};

/**
 * 
 */
struct Process {
    int PID;       // pid for blocks/unblocks
    int status;
    Process* senderNext; // for the queue
    Process* receiverNext;
    Message* msgSlot
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
int getNextMbox(void);
int getNextProcess(void);
int getNextSlot(void);
void unblockQueue(ProcQueue, int);

// ----- Global data structures/vars
Mailbox mailboxes[MAXMBOX];
Message mailslots[MAX_MESSAGE];
Process shadowTable[MAXPROC];

int pidIncrementer;
int slotIncrementer;
int mailboxIncrementer;
int numSlots;
int numMailboxes;

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
    slotIncrementer = 0;
    mailboxIncrementer = 0;
    numSlots = 0;
    numMailboxes = 0;
}

/**
 * Since we do not use any service processes, this function is blank. 
 */
void phase2_start_service_processes(void) {}

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
    // if the params are invalid
    if (numSlots < 0 || slot_size < 0 || slot_size > MAX_MESSAGE) {
        return -1;
    }

    int mbslot = getNextMbox();

    // no empty mailboxes
    if (mbslot == -1) {
        return -1;
    }

    mailboxes[mbslot].id = mbslot;
    mailboxes[mbslot].status = IN_USE;
    mailboxes[mbslot].numSlots = numSlots;
    mailboxes[mbslot].maxMessageSize = slot_size;

    return mbslot;
}

/**
 * 
 */
int MboxRelease(int mbox_id) {
    // check for invalid param
    if (mbox_id < 0 || mbox_id >= MAXMBOX || mailboxes[mbox_id].status == FREE) {
        return -1;
    }

    // change mailbox to be free
    mailboxes[mbox_id].status = FREE;

    // unblock the consumers
    unblockQueue(mailboxes[mbox_id].consumers, 0); 

    // unblock the producers
    unblockQueue(mailboxes[mbox_id].producers, 1); 

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
    mailboxes[slot].status = FREE;
    mailboxes[slot].maxMessageSize = 0;

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
    mailslots[slot].status = FREE;
    mailslots[slot].mboxID = -1;
}

/**
 * 
 */
void cleanShadowEntry(int slot) {
    shadowTable[slot].PID = -1;
	shadowTable[slot].status = FREE;
	shadowTable[slot].receiverNext = NULL;
	shadowTable[slot].senderNext = NULL;
	shadowTable[slot].msgSlot	= NULL;
}

/**
 * 
 */
int getNextMbox() {
	if (numMailboxes >= MAXMBOX) {
		return -1;
    }

	int count = 0;
	while(mailbox[mailboxIncrementer % MAXMBOX].status != FREE) {
		if (count < MAXMBOX) {
            count++;
		    mailboxIncrementer++;
        } else {
            return -1;
        }
	}

	return mailboxIncrementer % MAXMBOX;
}

/**
 * 
 */
int getNextProcess() {
	int count = 0;
	while (shadowTable[pidIncrementer % MAXPROC].status != FREE) {
		if (count < MAXPROC) {
            count++;
		    pidIncrementer++;
        } else {
            return -1;
        }
	}

	return pidIncrementer % MAXPROC;
}

/**
 * 
 */
int getNextSlot() {
	for (int i = 0; i < MAXSLOTS; i++) {
		if (slots[slotIncrementer % MAXSLOTS].status == FREE) {
            return slotIncrementer;
        }
		slotIncrementer++;
	}
	return -1;
}

void unblockQueue(ProcQueue queue, int type) {
    Process* curr = queue.head;

    while (curr != NULL) {
        curr->status = FREE;
        unblockProc(curr->PID);
        if (type == 1) {
            curr = curr->senderNext;
        } else {
            curr = curr->receiverNext;
        }
    }
}