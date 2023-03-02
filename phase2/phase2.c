/**
 * AUTHORS:    Kevin Nisterenko and Rey Sanayei
 * COURSE:     CSC 452, Spring 2023
 * INSTRUCTOR: Russell Lewis
 * ASSIGNMENT: Phase2
 * DUE_DATE:   03/02/2023
 * 
 * This project implements a mailbox system for IPC. It handles both the sending
 * and receiving of messages with or without payload as a way to mimic process 
 * communication in an operating system. Direct delivery strategy. 
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
 * Mailbox struct for the mailbox, contains all the necessary information
 * to maintain a mailbox. 
 */
struct Mailbox {
    int numSlots;       // number of slots this mailbox contains
    int usedSlots;      // number of slots that have been used/have a message
    int id;             // id of the mailbox
    int status;         // status of the mailbox (released, in use)
    int maxMessageSize; // max message size this mailbox can hold
    int nextMsg;        // to check if there is or not a next message
    int sent;           // how many messages have been sent
    int receiveSize;    // the size of the message received (for sanity check)

    // for zero slot mailboxes
    int zMessageSize;         // message size in a zero slot mailbox
    char* zSlot[MAX_MESSAGE]; // message in a zero slot mailbox

    // Queues
    Message* messagesHead;  // message queue 
    Process* producersHead; // senders
    Process* consumersHead; // receivers
    Process* orderedHead;   // order queue

};

/**
 * Messag struct for a specific message/slot in a mailbox that processes 
 * send and receive. 
 */
struct Message {
    int mboxID;             // so we know which mailbox it is attached to 
    int size;               // size so we caan easily check for validity
    int status;             // status of the message
    char msg[MAX_MESSAGE];  // actual message
    Message* next;          // for the queue
};

/**
 * Struct for the shadow table of processes, where we hold necessary info 
 * to block/unblock and send/receive for a process. 
 */
struct Process {
    int PID;               // pid for blocks/unblocks
    int status;            // status of the process (blocked, free)
    Process* senderNext;   // for the sender queue (if this process is a sender)
    Process* receiverNext; // for the receiver queue (if this process is a receiver)
    Process* orderedNext;  // for the ordered queue (to keep order for receiving and sending)
    Message* msgSlot       // which message it holds
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
void clockHandler(int, void*);
void terminalHandler(int, void*);
void syscallHandler(int, void*);
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
void addToOrderedQueue(Mailbox*, Process*);
void addSlot(Mailbox*, Message*);
void removeSlot(Mailbox*, Message*);

// ----- Global data structures/vars
Mailbox mailboxes[MAXMBOX];   // all mailboxes in the system
Message mailslots[MAXSLOTS];  // global pool of messages
Process shadowTable[MAXPROC]; // shadow process table

int pidIncrementer;           // so we always have the right index into the shadow table
int slotIncrementer;          // so we always have the right index into the messages array
int mailboxIncrementer;       // so we always have the right index into the mailbox array
int numSlots;                 // so we know how many slots are globally being used
int numMailboxes;             // so we know how many mailboxes are being used
int clockCount;               // so we can fire the clock handler 

void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs *args); // syscall vector

// ----- Phase 2 Bootload

/**
 * Initialized all the needed variables and data structures for the bootload
 * process so this phase can start running and all the functions safely called. 
 */
void phase2_init(void) {
    // set the handlers for USLOSS
    USLOSS_IntVec[USLOSS_TERM_INT] = terminalHandler;
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscallHandler;
    USLOSS_IntVec[USLOSS_CLOCK_INT] = clockHandler;

    // initialize the global arrays by wiping them clean
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

    // need to intialize first 7 mailboxes for input output
    for (int i = 0; i < 7; i++) {
        mailboxes[i].id = i;
        mailboxes[i].status = IN_USE;
        mailboxes[i].maxMessageSize = sizeof(int);
    }

    // set the global variables
    pidIncrementer = 0;
    slotIncrementer = 0;
    numSlots = 0;
    numMailboxes = 7;
}

/**
 * Since we do not use any service processes, this function is blank. 
 */
void phase2_start_service_processes(void) {}

/**
 * This function checks the reserved i/o mailboxes to see if there are any
 * processes waiting on it.
 * 
 * @return int representing yes or no for processes on i/o
 */
int phase2_check_io(void) {
    // first 7 mailboxes are for i/o, check if they have anything waiting on 
    // them, if so, return 1 (for yes)
    for (int i = 0; i < 7; i++) {
        if (mailboxes[i].consumersHead != NULL) {
            return 1;
        }
    }
    return 0;
}

// ----- Messaging System

/**
 * This function creates a new mailbox, esentially it allocates one of the free 
 * mailboxes with the given parameters for number of slots and slot/message size. 
 * 
 * @param slots, int representing the number of messages this mailbox can hold
 * @param slot_size, int representing the max message size of this mailbox
 * 
 * @return mbslot, int representing the id of the mailbox, -1 if the creation
 * was unsucessful
 */
int MboxCreate(int slots, int slot_size) {
    // if the params are invalid
    if (slots < 0 || slots > MAXSLOTS || slot_size < 0 || slot_size > MAX_MESSAGE) {
        return -1;
    }

    int mbslot = getNextMbox();

    // no empty mailboxes, cannot create any
    if (mbslot == -1) {
        return -1;
    }

    mailboxes[mbslot].id = mbslot;
    mailboxes[mbslot].status = IN_USE;
    mailboxes[mbslot].numSlots = slots;
    mailboxes[mbslot].maxMessageSize = slot_size;

    return mbslot;
}

/**
 * This function is used to free a currently in use mailbox and wipe it clean. 
 * It also unblocks all processes that depend on it since it will not exist 
 * anymore. 
 * 
 * @param mbox_id, id of the mailbox we want to release
 * 
 * @return int, 0 if successful, -1 otherwise
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
 * This function is used by a process to send a message in a mailbox. It specifies
 * the mailbox, the message itself and also as help, it gives the message size 
 * for simple checking. If the message can be sent (valid params and free slots),
 * it does so. Otherwise, if the message cannot be sent due to lack of free slots
 * or receivers (since we do direct delivery), then the process is blocked until 
 * the message can be sent. 
 * 
 * @param mbox_id, int representing the id of the mailbox to send the message to
 * @param msg_ptr, void pointer representing the bytes of actual message
 * @param msg_size, int representing the size of the message we are trying to send
 * 
 * @return int with status code for sucessful completion of a send. -1 if parameters
 * are invalid, -2 if there are no available slots globally, -3 if mailbox was
 * released and 0 if the sent was successful
 */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size) {
    // checking for invalid params
    if (mbox_id < 0 || mbox_id >= MAXMBOX || msg_size < 0 || msg_size > mailboxes[mbox_id].maxMessageSize) {
        return -1;
    }

    // if we don't have any more available slots to write a message to
    if (numSlots >= MAXSLOTS) {
        USLOSS_Console("MboxSend_helper: Could not send, the system is out of mailbox slots.\n");
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

            blockMe(10+shadowTable[procIdx].PID);

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
            mbox->consumersHead->receiverNext = NULL;
            mbox->consumersHead = nextConsumer; 

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

        blockMe(10+shadowTable[procIdx].PID); 

        // in case we release the mailbox before ensuring it is invalid
        if (mbox->status == FREE) {
            return -3;
        }

        // while we still have more messages than sent
        while (msgIdx > mbox->sent + 1) {
            // get the next process we have and block it
            procIdx = getNextProcess();
            shadowTable[procIdx].PID = getpid();
			shadowTable[procIdx].status	= BLOCKED;

            // add it to the order queue so we can maintain the order
			addToOrderedQueue(mbox, &shadowTable[procIdx]); 
            
            blockMe(10+shadowTable[procIdx].PID); 

             // in case we release the mailbox before ensuring it is invalid
            if (mbox->status == FREE) {
                return -3;
            }

        }
    }
    // here we have space left on the mailbox and can just simply send the message
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

        Process* newHead = mbox->consumersHead->receiverNext;
        mbox->consumersHead->receiverNext = NULL;
        mbox->consumersHead = newHead; 

        unblockProc(removed);
    }

    // free up anyone waiting on the message in the order queue
    while (mbox->orderedHead != NULL) {
        int rm = mbox->orderedHead->PID;

        Process* newHead = mbox->orderedHead->orderedNext; 
        mbox->orderedHead->orderedNext = NULL;
        mbox->orderedHead = newHead; 

        unblockProc(rm);
    }

    mbox->usedSlots++;
    numSlots++;
    
    return 0;
}

/**
 * This function is used by a process to receive a message in a mailbox. It specifies
 * the mailbox, the message field of the process and also as help, it gives the message size 
 * for simple checking. If the message can be received (valid params),
 * it does so. Otherwise, if the message cannot be recived due to lack of receivers or
 * no message to be received, it blocks the process until it can complete this operation.
 * 
 * @param mbox_id, int representing the id of the mailbox where it received the
 * @param msg_ptr, void pointer representing the bytes where the message will be copied to
 * @param msg_size, int representing the size of the message we are trying to receive/max size
 * 
 * @return size, int representing the size of the message we received. -1 if no message was 
 * received and -3 if the mailbox was released during the operation
 */
int MboxRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
    // invalid parameters
    if (mbox_id < 0 || mbox_id > MAXMBOX || msg_max_size < 0 || msg_max_size > MAX_MESSAGE) {
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

            blockMe(10+shadowTable[procIdx].PID);

            // in case we release the mailbox before ensuring it is invalid
            if (mbox->status == FREE) {
                return -3;
            }

            memcpy(msg_ptr, mbox->zSlot, msg_max_size);

            return mbox->zMessageSize;
        } 

        // if we had anyone blocked because they could not send the message
        int oldBlock = mbox->producersHead->PID;
        mbox->producersHead->status = FREE;

        // just free this up
        Process* nextSender = mbox->producersHead->senderNext;
        mbox->producersHead->senderNext = NULL;
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

        blockMe(10+shadowTable[procIdx].PID);

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
        memcpy(msg_ptr, shadowTable[procIdx].msgSlot->msg, msg_max_size);
        size = shadowTable[procIdx].msgSlot->size;
        
        Message* removed = shadowTable[procIdx].msgSlot;

        removeSlot(mbox, removed);

        shadowTable[procIdx].status = FREE;

        cleanSlotPointer(removed);  
    // otherwise we don't need to remove any messages here, and can simply copy it 
    // over
    } else {
        memcpy(msg_ptr, mbox->messagesHead->msg, msg_max_size);
        size = mbox->messagesHead->size;

        Message* cleanUp = mbox->messagesHead;
        mbox->messagesHead = mbox->messagesHead->next;

        cleanSlotPointer(cleanUp);  
    }

    // if we have free slots, let's just clean up the blocked sender
    if (mbox->producersHead != NULL) {
        int blockIdx = mbox->producersHead->PID;

        mbox->producersHead->status = FREE;
        Process* next = mbox->producersHead->senderNext;
        mbox->producersHead->senderNext = NULL;
        mbox->producersHead = next;
        unblockProc(blockIdx);
    }

    mbox->usedSlots--;
    numSlots--;

    return size;
}

/**
 * This function is a non-blocking version of the regular MboxSend. It takes the same 
 * parameters and returns, and instead of blocking it will simply return/wait to do the
 * operation later.
 * 
 * @param mbox_id, int representing the id of the mailbox to send the message to
 * @param msg_ptr, void pointer representing the bytes of actual message
 * @param msg_size, int representing the size of the message we are trying to send
 * 
 * @return int with status code for sucessful completion of a send. -1 if parameters
 * are invalid, -2 if there are no available slots globally, -3 if mailbox was
 * released and 0 if the sent was successful
 */
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size) {
    // invalid parameters
    if (mbox_id < 0 || mbox_id > MAXMBOX || msg_size < 0 || msg_size > mailboxes[mbox_id].maxMessageSize) {
        return -1;
    }

    // error
    if (numSlots >= MAXSLOTS) {
        USLOSS_Console("MboxSend_helper: Could not send, the system is out of mailbox slots.\n");
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
 * This function is a non-blocking version of the regular MboxRecv. It takes the same 
 * parameters and returns, and instead of blocking it will simply return/wait to do the
 * operation later.
 * 
 * @param mbox_id, int representing the id of the mailbox where it received the
 * @param msg_ptr, void pointer representing the bytes where the message will be copied to
 * @param msg_size, int representing the size of the message we are trying to receive/max size
 * 
 * @return size, int representing the size of the message we received. -1 if no message was 
 * received and -3 if the mailbox was released during the operation
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
 * Given a device, this function will wait (by launching a receive) on a specific device interrupt
 * to be sent. 
 * 
 * @param type, int representing type of device
 * @param unit, int representing which USLOSS unit/mailbox to send this to
 * @param status, int pointer representing the status of the interrupt (akin to message)
 */
void waitDevice(int type, int unit, int *status) {
    if (type == USLOSS_CLOCK_DEV) {
		MboxRecv(0, status, 4);
    }
	else if (type == USLOSS_TERM_DEV) {
		MboxRecv(3 + unit, status, 4);
    }
	else if (type == USLOSS_DISK_DEV) {
		MboxRecv(1 + unit, status, 4);
    }
	else {
		USLOSS_Console("Invalid device. Halt\n");
		USLOSS_Halt(1);
	}
}

/**
 * Just for compiling due to .h file
 */
void wakeupByDevice(int type, int unit, int status) {}

// ----- Interrupr Handlers

/**
 * Left blank as we used our own clockHandler for the intVec below. 
 */
void phase2_clockHandler(void) {}

/**
 * Interrupt handler for the clock device. 
 * 
 * @param type, int representing USLOSS device type
 * @param args, void pointer representing the arguments for the
 * interrupt
 */
void clockHandler(int type, void* args) {
    int status = -1;
    int i;

    // call time slice when the clock interrupt is fired
    timeSlice();
    i = USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &status);
    i++;
    // increase the clock count
    clockCount++; 
    // if we have a multiple of 5, send a message
    if (clockCount % 5 == 0) {
        MboxCondSend(0, &status, 4);
    }
}

/**
 * Interrupt handler for the terminal device. 
 * 
 * @param type, int representing USLOSS device type
 * @param args, void pointer representing the arguments for the
 * interrupt
 */
void terminalHandler(int type, void* args) {
    int tmp = -1;
    // simply call the device input for the terminal
    int x = USLOSS_DeviceInput(USLOSS_TERM_DEV, (u_int64_t) args, &tmp);
	x++;
	MboxCondSend((u_int64_t) args + 3, &tmp, 4);
}

/**
 * Interrupt handler for the syscall device. 
 * 
 * @param type, int representing USLOSS device type
 * @param args, void pointer representing the arguments for the
 * interrupt
 */
void syscallHandler(int type, void* args) {
    USLOSS_Sysargs* tmp = (USLOSS_Sysargs*) args;
    // call the appropriate handler for the specific syscall
    if (tmp->number >= 0 && tmp->number < MAXSYSCALLS) {
        systemCallVec[tmp->number](args);
    }
    // if it is an unknown/unsupported syscall, terminate
    else {
        USLOSS_Console("syscallHandler(): Invalid syscall number %d\n", tmp->number);
        USLOSS_Halt(1);
    }
}

/**
 * Null syscall handler for unimplemented system calls.
 * 
 * @param args, USLOSS_Sysargs pointer for the syscall arguments
 */
void nullsys(USLOSS_Sysargs* args) {
    USLOSS_Console("nullsys(): Program called an unimplemented syscall.  syscall no: %d   PSR: 0x09\n", args->number);
	USLOSS_Halt(1);
}

// ----- Helpers

/**
 * Helper for cleaning/initializing a mailbox to the default/zero values. 
 * 
 * @param slot, int representing index into the mailboxes array
 */
void cleanMailbox(int slot) {
    mailboxes[slot].numSlots = 0;
    mailboxes[slot].usedSlots = 0;
    mailboxes[slot].id = -1;
    mailboxes[slot].status = FREE;
    mailboxes[slot].maxMessageSize = 0;
    mailboxes[slot].nextMsg = 1; 
    mailboxes[slot].sent = 0;
    mailboxes[slot].receiveSize = 0;

    mailboxes[slot].zMessageSize = 0;
    mailboxes[slot].zSlot[0] = '\0';

    mailboxes[slot].messagesHead = NULL;
    mailboxes[slot].producersHead = NULL;
    mailboxes[slot].consumersHead = NULL;
    mailboxes[slot].orderedHead = NULL;
}

/**
 * Helper for cleaning/initializing a message to the default/zero values. 
 * 
 * @param slot, int representing index into the mailslots array
 */
void cleanSlot(int slot) {
    mailslots[slot].msg[0] = '\0';
    mailslots[slot].next = NULL;
    mailslots[slot].size = 0;
    mailslots[slot].status = FREE;
    mailslots[slot].mboxID = -1;
}

/**
 * Helper for cleaning/initializing a message to the default/zero values. 
 * 
 * @param msg, Message pointer representing the message we want to clean up
 */
void cleanSlotPointer(Message* msg) {
    msg->mboxID = -1;
    msg->next = NULL;
    msg->size = 0;
    msg->status = FREE;
}

/**
 * Helper for cleaning/initializing a shadow process entry to the default/zero
 * values. 
 * 
 * @param slot, int representing index into the shadowTable
 */
void cleanShadowEntry(int slot) {
    shadowTable[slot].PID = -1;
	shadowTable[slot].status = FREE;
	shadowTable[slot].receiverNext = NULL;
	shadowTable[slot].senderNext = NULL;
	shadowTable[slot].msgSlot = NULL;
    shadowTable[slot].orderedNext = NULL;
}

/**
 * Tries to find the next free mailbox slot in the global array and
 * returns the index so we can access that mailbox. 
 * 
 * @return int representing the index into the array
 */
int getNextMbox() {
    // if we have no free mailboxes, return -1
	if (numMailboxes >= MAXMBOX) {
		return -1;
    }

    // in a circular fashion, try to find the next free mailbox
    // in the mailboxes array
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
 * Tries to find the next free process slot in the global array and
 * returns the index so we can access that process in the shadow table. 
 * 
 * @return int representing the index into the array
 */
int getNextProcess() {
	int count = 0;
    // in a circular fashion, try to find the next free process
    // in the shadowTable
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
 * Tries to find the next free mail/message slot in the global array and
 * returns the index so we can access that message.
 * 
 * @return int representing the index into the array
 */
int getNextSlot() {
    // in a circular fashion, try to find the next free mailslot
    // in the mailslots array
	for (int i = 0; i < MAXSLOTS; i++) {
		if (mailslots[slotIncrementer % MAXSLOTS].status == FREE) {
            return slotIncrementer;
        }
		slotIncrementer++;
	}
	return -1;
}

/**
 * Unblocks all receiver processes from a given mailbox, used
 * when we release/free a mailbox. 
 * 
 * @param mbox, Mailbox pointer representing the mailbox where 
 * we want to unblock the receivers
 */
void unblockReceivers(Mailbox* mbox) {
    Process* curr = mbox->consumersHead;

    // while we have something in the queue, unblock it
    while (curr != NULL) {
        curr->status = FREE;
        unblockProc(curr->PID);
        curr = curr->receiverNext;
    }
}

/**
 * Unblocks all sender processes from a given mailbox, used
 * when we release/free a mailbox. 
 * 
 * @param mbox, Mailbox pointer representing the mailbox where 
 * we want to unblock the senders
 */
void unblockSenders(Mailbox* mbox) {
    Process* curr = mbox->producersHead;

    // while we have something in the queue, unblock it
    while (curr != NULL) {
        curr->status = FREE;
        unblockProc(curr->PID);
        curr = curr->senderNext;
    }
}

/**
 * Adds the given process to the receiver queue (linked list)
 * of processes in the given mailbox.
 * 
 * @param mbox, Mailbox pointer representing the mailbox where 
 * we want to add the receiver
 * @param proc, Process pointer representing the process
 * we want to add to the list 
 */
void addToReceiverQueue(Mailbox* mbox, Process* proc) {
    Process* h = mbox->consumersHead;
    
    // if the list is empty, the added process is the head
    if (h == NULL) {
        mbox->consumersHead = proc;
        return;
    }
    
    // otherwise, find the next free spot (end of list)
    // and add it there
    Process* curr = mbox->consumersHead;

    while (curr->receiverNext != NULL) {
        curr = curr->receiverNext;
    }
    curr->receiverNext = proc;
}

/**
 * Adds the given process to the sender queue (linked list)
 * of processes in the given mailbox.
 * 
 * @param mbox, Mailbox pointer representing the mailbox where 
 * we want to add the sender
 * @param proc, Process pointer representing the process
 * we want to add to the list 
 */
void addToSenderQueue(Mailbox* mbox, Process* proc) {
    Process* h = mbox->producersHead;

    // if the list is empty, the added process is the head
    if (h == NULL) {
        mbox->producersHead = proc;
        return;
    }

    // otherwise, find the next free spot (end of list)
    // and add it there
    Process* curr = mbox->producersHead;

    while (curr->senderNext != NULL) {
        curr = curr->senderNext;
    }
    curr->senderNext = proc;
}

/**
 * Adds the given process to the ordered queue (linked list)
 * of processes in the given mailbox.
 * 
 * @param mbox, Mailbox pointer representing the mailbox where 
 * we want to add the process
 * @param proc, Process pointer representing the process
 * we want to add to the list 
 */
void addToOrderedQueue(Mailbox* mbox, Process* proc) {
    Process* h = mbox->orderedHead;

    // if the list is empty, the added process is the head
    if (h == NULL) {
        mbox->orderedHead = proc;
        return;
    }

    // otherwise, find the next free spot (end of list)
    // and add it there
    Process* curr = mbox->orderedHead;

    while (curr->orderedNext != NULL) {
        curr = curr->orderedNext;
    }
    curr->orderedNext = proc;
}

/**
 * Adds the given message to the messages queue in the given
 * mailbox.
 * 
 * @param mbox, Mailbox pointer representing the mailbox where 
 * we want to add the message
 * @param toAdd, Message pointer representing the message
 * we want to add to the list 
 */
void addSlot(Mailbox* mbox, Message* toAdd) {
    Message* h = mbox->messagesHead;

    // if the list is empty, the added message is the head
    if (h == NULL) {
        mbox->messagesHead = toAdd;
        return;
    } 

    // otherwise, find the next free spot (end of list)
    // and add the message there
    Message* curr = mbox->messagesHead;

    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = toAdd;
    
}

/**
 * Removes the given message from the messages queue in the given
 * mailbox.
 * 
 * @param mbox, Mailbox pointer representing the mailbox where 
 * we want to remove the message
 * @param toAdd, Message pointer representing the message
 * we want to remove from the list
 */
void removeSlot(Mailbox* mbox, Message* toRemove) {
    // if there are no messages, do nothing
    if (mbox->messagesHead == NULL) {
        return;
    }

    // if the message we want to remove is the head, do so
    if (mbox->messagesHead == toRemove) {
        mbox->messagesHead = mbox->messagesHead->next;
        return;
    }

    // otherwise find the message and remove it from the
    // list
    Message* curr = mbox->messagesHead; 

    while(curr->next != toRemove) {
        curr = curr->next;
    }
    curr->next = curr->next->next;
}
