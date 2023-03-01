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
#define BLOCKED -1

// ----- Includes
#include <usloss.h>
#include <phase1.h>
#include <phase2.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ----- typedefs
typedef struct Mailbox Mailbox; 
typedef struct Message Message;
typedef struct Process Process;

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
    int nextMsg;
    int sent;
    int receiveSize;

    // for zero slot mailboxes
    int zMessageSize; 
    char* zSlot[MAX_MESSAGE]; // message in a zero slot mailbox

    Message* messagesHead; 
    Process* producersHead;
    Process* consumersHead;

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

// ----- Function Prototypes
// Phase 2 Bootload
void phase2_init(void);
void phase2_start_service_processes(void);
int phase2_check_io(void);

// Messaging System 
int MboxCreate(int slots,int slot_size);
int MboxRelease(int mbox_id);
int MboxSend(int mbox_id, void *msg_ptr,int msg_size);
int MboxRecv(int mbox_id, void *msg_ptr,int msg_max_size);
int MboxCondSend(int mbox_id, void *msg_ptr,int msg_size);
int MboxCondRecv(int mbox_id, void *msg_ptr,int msg_max_size);
void waitDevice(int type,int unit,int *status);
void wakeupByDevice(int type,int unit,int status);

// Interrupt Handlers
void phase2_clockHandler(void);
void terminalHandler(void);
void syscallHandler(void);
void nullsys(USLOSS_Sysargs*);

// Helpers
void cleanMailbox(int);
void cleanSlot(int);
void cleanSlotPointer(Message*);
void cleanShadowEntry(int);
int getNextMbox(void);
int getNextProcess(void);
int getNextSlot(void);
void unblockReceivers(Mailbox*);
void unblockSenders(Mailbox*);
void addToReceiverQueue(Mailbox*, Process*);
void addToSenderQueue(Mailbox*, Process*);
void addSlot(Mailbox*, Message*);
void removeSlot(Mailbox*, Message*);

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

// ----- Phase 2 Bootload

/**
 * testing my branch
 */
void phase2_init(void) {

    USLOSS_IntVec[USLOSS_TERM_INT] = terminalHandler;
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscallHandler;
    USLOSS_IntVec[USLOSS_CLOCK_INT] = phase2_clockHandler;

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
        systemCallVec[i] = nullsys;
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
    for (int i =0; i < 7; i++) {

    }
    return 0;
}

// ----- Messaging System

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
    unblockReceivers(&mailboxes[mbox_id]); 

    // unblock the producers
    unblockSenders(&mailboxes[mbox_id]); 

    return 0;
}

/**
 * 
 */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size) {
    // checking for invalid params
    if (mbox_id < 0 || mbox_id >= MAXMBOX || msg_size < 0 || msg_size > mailboxes[mbox_id].maxMessageSize) {
        return -1;
    }

    // if we don't have any more available slots to write a message to
    if (numSlots >= MAXSLOTS) {
        return -2;
    }

    Mailbox* mbox = &mailboxes[mbox_id];

    // mailbox is invalid
    if (mbox->status == FREE) { 
        return -1;
    }

    int msgIdx = mbox->nextMsg;
    mbox->nextMsg++;

    // if we have a zero slot mailbox
    if (mbox->numSlots == 0) {
        // if we have no one waiting for a message
        if (mbox->consumersHead == NULL) {
            int procIdx = getNextProcess();

            shadowTable[procIdx].PID = getpid();
            shadowTable[procIdx].status = BLOCKED;

            addToSenderQueue(mbox, &shadowTable[procIdx]);

            memcpy(mbox->zSlot, msg_ptr, msg_size);
            mbox->zMessageSize = msg_size;

            blockMe(10+shadowTable[procIdx].PID); // why the 10+ ???

            // in case we release the mailbox before ensuring it is invalid
            if (mbox->status == FREE) {
                return -3;
            }
        // if we do have someone on the consumer buffer to get this message
        } else {
            int oldBlock = mbox->consumersHead->PID;
            mbox->consumersHead->status = FREE;

            // just free this up
            Process* nextConsumer = mbox->consumersHead->receiverNext;

            memcpy(mbox->zSlot, msg_ptr, msg_size);
            mbox->zMessageSize = msg_size;

            // now we can unblock the process because it received the message
            // it was waiting on
            unblockProc(oldBlock);
        }
        return 0;
    }

    // if the mailbox is full, block until a free slot opens up
    if (mbox->usedSlots == mbox->numSlots) {
        int procIdx = getNextProcess();

        shadowTable[procIdx].PID = getpid();
        shadowTable[procIdx].status = BLOCKED;

        addToSenderQueue(mbox, &shadowTable[procIdx]);

        blockMe(10+shadowTable[procIdx].PID); // why the 10+ ???

        // in case we release the mailbox before ensuring it is invalid
        if (mbox->status == FREE) {
            return -3;
        }

        // This whole thing is sketch
        while (msgIdx > mbox->sent + 1) {
            procIdx = getNextProcess();
            shadowTable[procIdx].PID = getpid();
			shadowTable[procIdx].status	= BLOCKED;

			addToSenderQueue(mbox, &shadowTable[procIdx]); // why the order list???
            
            blockMe(10+shadowTable[procIdx].PID); // why the 10+ ???

             // in case we release the mailbox before ensuring it is invalid
            if (mbox->status == FREE) {
                return -3;
            }

        }
    }

    // here we have space left and can just simply send the message
    int slotIdx = getNextSlot();
    Message* newMsg = &mailslots[slotIdx];

    // initialize the message
    newMsg->mboxID = mbox_id;
    newMsg->status = IN_USE;
    newMsg->next = NULL;
    newMsg->size = msg_size;
    memcpy(&newMsg->msg, msg_ptr, msg_size);

    // add the message to the mailbox
    mbox->sent++;
    addSlot(mbox, newMsg);

    // now we check if we simply send the message (if there are waiting
    // processes) or not
    if (mbox->consumersHead != NULL) {
        // the process receives the message, ie. it was sent
        mbox->consumersHead->msgSlot = newMsg;

        // remove the process from the buffer and unblock it
        int removed = mbox->consumersHead->PID;

        Process* newHead = mbox->consumersHead->senderNext;
        mbox->consumersHead = newHead; 
        // why did Vatsav make it null? like wouldn;t that just ignore the
        // rest of the remaining ones? I think it's because his queue only 
        // has one proc, so we need to actually take care of this here
        unblockProc(removed);
    }

    // should this be receivers? or consumers? idk Im confused if we even need
    // this at all, this all comes from confusion on Vatsav's order list
    while (mbox->producersHead != NULL) {
        int removed = mbox->producersHead->PID;

        Process* newHead = mbox->producersHead->senderNext; 

        mbox->consumersHead = newHead; 

        unblockProc(removed);
    }

    mbox->usedSlots++;
    numSlots++;

    return 0;
}

/**
 * 
 */
int MboxRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
    // invalid parameters
    if (mbox_id < 0 || mbox_id > MAXMBOX || msg_max_size < 0 || msg_max_size > mailboxes[mbox_id].maxMessageSize) {
        return -1;
    }

    int size = -1;

    Mailbox* mbox = &mailboxes[mbox_id];

    if (mbox->status == FREE) {
        return -1;
    }

    // if we are dealing with a zero slot mailbox
    if (mbox->numSlots == 0) {
        if (mbox->producersHead == NULL) {
            int procIdx = getNextProcess();

            shadowTable[procIdx].PID = getpid();
            shadowTable[procIdx].status = BLOCKED;

            addToReceiverQueue(mbox, &shadowTable[procIdx]);

            blockMe(10+shadowTable[procIdx].PID); // why the 10+ ???

            // in case we release the mailbox before ensuring it is invalid
            if (mbox->status == FREE) {
                return -3;
            }

            memcpy(msg_ptr, mbox->zSlot, msg_max_size);

            return mbox->zMessageSize;
        } 

        int oldBlock = mbox->producersHead->PID;
        mbox->producersHead->status = FREE;

        // just free this up
        Process* nextSender = mbox->producersHead->senderNext;
        mbox->producersHead = nextSender;

        memcpy(msg_ptr, mbox->zSlot, msg_max_size);

        // now we can unblock the process because it received the message
        // it was waiting on
        unblockProc(oldBlock);
        return mbox->zMessageSize;
    }

    int isBlocked = 0;
    int procIdx = -1;

    // if there are no messages to receive, block until we do
    if (mbox->messagesHead == NULL) {
        isBlocked = 1;
        procIdx = getNextProcess();

        shadowTable[procIdx].PID = getpid();
        shadowTable[procIdx].status = BLOCKED;
        mbox->receiveSize = msg_max_size;

        addToReceiverQueue(mbox, &shadowTable[procIdx]);

        blockMe(10+shadowTable[procIdx].PID); // why the 10+ ???

        // in case we release the mailbox before ensuring it is invalid
        if (mbox->status == FREE) {
            return -3;
        }
    }

    // can't receive a message bigger than what we can accomodate
    if (mbox->messagesHead->size > msg_max_size) {
        return -1;
    }

    // if we were blocked, now that we have a message we can just receive it
    if (isBlocked) {
        memcpy(msg_ptr, mbox->messagesHead->msg, msg_max_size);
        size = mbox->messagesHead->size;

        Message* cleanUp = mbox->messagesHead;
        mbox->messagesHead = mbox->messagesHead->next;

        cleanSlotPointer(cleanUp);  
    }

    // if we have free slots, let's just clean up the blocked sender
    if (mbox->messagesHead != NULL) {
        int blockIdx = mbox->producersHead->PID;

        mbox->producersHead->status = FREE;

        Process* next = mbox->producersHead->senderNext;
        mbox->producersHead = next;

        unblockProc(blockIdx);
    }

    mbox->usedSlots--;
    numSlots--;

    return size;
}

/**
 * 
 */
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size) {
    // invalid parameters
    if (mbox_id < 0 || mbox_id > MAXMBOX || msg_size < 0 || msg_size > mailboxes[mbox_id].maxMessageSize) {
        return -1;
    }

    // error
    if (numSlots >= MAXSLOTS) {
        return -2;
    }

    // now instead of blocking we just return the code
    Mailbox curr = mailboxes[mbox_id];

    if (curr.status == FREE) {
        return -1;
    }

    if (curr.numSlots != 0 && curr.usedSlots == curr.numSlots) {
        return -2;
    }

    // if we already checked that we are not blocking we can go ahead and send the message
    return MboxSend(mbox_id, msg_ptr, msg_size);
}

/**
 * 
 */
int MboxCondRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
    // invalid parameters
    if (mbox_id < 0 || mbox_id > MAXMBOX || msg_max_size < 0 || msg_max_size > MAX_MESSAGE) {
        return -1;
    }

    // now instead of blocking we just return the code
    Mailbox curr = mailboxes[mbox_id];

    if (curr.status == FREE) {
        return -1;
    }

    // if there's something to receive
    if (curr.messagesHead == NULL) {
        return -2;
    }

    // if we already checked that we are not blocking we can go ahead and receive the message
    return MboxRecv(mbox_id, msg_ptr, msg_max_size);
}

/**
 * 
 */
void waitDevice(int type, int unit, int *status) {

}

/**
 * Just for compiling ig
 */
void wakeupByDevice(int type, int unit, int status) {}

// ----- Interrupr Handlers

/**
 * 
 */
void phase2_clockHandler(void) {

}

/**
 * 
 */
void terminalHandler(void) {

}

/**
 * 
 */
void syscallHandler(void) {

}

/**
 * 
 */
void nullsys(USLOSS_Sysargs*) {

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
    mailboxes[slot].nextMsg = 1; // why not zero???
    mailboxes[slot].sent = 0;
    mailboxes[slot].receiveSize = 0;

    mailboxes[slot].zMessageSize = 0;
    mailboxes[slot].zSlot[0] = '\0';

    mailboxes[slot].messagesHead = NULL;
    mailboxes[slot].producersHead = NULL;
    mailboxes[slot].consumersHead = NULL;
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
void cleanSlotPointer(Message* msg) {
    msg->mboxID = -1;
    msg->next = NULL;
    msg->size = 0;
    msg->status = FREE;
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
	while(mailboxes[mailboxIncrementer % MAXMBOX].status != FREE) {
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
		if (mailslots[slotIncrementer % MAXSLOTS].status == FREE) {
            return slotIncrementer;
        }
		slotIncrementer++;
	}
	return -1;
}

/**
 * 
 */
void unblockReceivers(Mailbox* mbox) {
    Process* curr = mbox->consumersHead;

    while (curr != NULL) {
        curr->status = FREE;
        unblockProc(curr->PID);
        curr = curr->receiverNext;
    }
}

/**
 * 
 */
void unblockSenders(Mailbox* mbox) {
    Process* curr = mbox->producersHead;

    while (curr != NULL) {
        curr->status = FREE;
        unblockProc(curr->PID);
        curr = curr->senderNext;
    }
}

/**
 * 
 */
void addToReceiverQueue(Mailbox* mbox, Process* proc) {
    Process* h = mbox->consumersHead;

    if (h == NULL) {
        mbox->consumersHead = proc;
    } else {
        proc->receiverNext = mbox->consumersHead;
        mbox->consumersHead = proc;
    }
}

/**
 * 
 */
void addToSenderQueue(Mailbox* mbox, Process* proc) {
    Process* h = mbox->producersHead;

    if (h == NULL) {
        mbox->producersHead = proc;
    } else {
        proc->senderNext = mbox->producersHead;
        mbox->producersHead = proc;
    }
}

/**
 * 
 */
void addSlot(Mailbox* mbox, Message* toAdd) {
    Message* h = mbox->messagesHead;

    if (h == NULL) {
        mbox->messagesHead = toAdd;
    } else {
        toAdd->next = mbox->messagesHead;
        mbox->messagesHead = toAdd;
    }
}

/**
 * 
 */
void removeSlot(Mailbox* mbox, Message* toRemove) {
    if (mbox->messagesHead == toRemove) {
        mbox->messagesHead = mbox->messagesHead->next;
    }

    Message* curr = mbox->messagesHead; 

    while(curr->next != toRemove) {
        curr = curr->next;
    }
    curr->next = curr->next->next;
}