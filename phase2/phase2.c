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
#include <phase1.h>
#include <phase2.h> 
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//#include <stdio.h>

// typedefs
typedef struct MboxEntry	MbEntry;	// Struct storing data of a mailbox's entry
typedef struct MboxEntry*	MbPtr;		// Pointer to MboxEntry (can remove)
typedef struct Slot		    Slot;		// Slot storing a slot's data
typedef struct Slot*		SPtr;		// Pointer to Slot (can remove)
typedef struct ShadowPTE	ShadowPTE;	// Struct storing the Shadow Proc Table's data.
typedef struct ShadowPTE*	ShadowPtr;	// Pointer to ShadowPTE (can remove)

// ----- Structs
// struct definition for MailBoxEntry
struct MboxEntry {
	int		mbox_id;		// The ID of the mailbox
	int		state;			// The current state of the mailbox (UNUSED or OCCUPIED)
	int		num_slots;		// The max number of slots it is allowed to hold
	int		slot_size;		// The size of each slot it is holding
	int		used_slots;		// The number of used slots in the mailbox
	int		msg_sent;	    // The number of messages that have been sent
	int		zero_size;		// The size of a slot in the case of a zero slot mailbox
	char*		zero_slot[MAX_MESSAGE];	// The message in the slot of a zero slot mailbox
	int		received_size;	// The size of the message received
	int		next_msg;		// To maintain order in case of conflicting priorities

	SPtr		slot_head;		// The head pointer to the linked lists of slots
	ShadowPtr	receive_head;	// The head pointer to the linked list of processes waiting on a receive
	ShadowPtr	send_head;		// The head pointer to the linked list of processes waiting on a send
	ShadowPtr	order;			// The head pointer to the linked list we maintain to enforce order in the case of conflicting priorities
};

struct Slot {
	int		mbox_id;		// The ID of the mailbox the slot belongs to
	int		state;			// The state of the slot
	char		message[MAX_MESSAGE];	// The message that held by the slot
	int		size;			// The size of the slot
	SPtr	next;			// The next pointer for the linked list of slots in the mailbox
};

struct ShadowPTE {
	int		pid;			// The PID corresponsing to the ProcTable in Phase 1
	int		state;			// The (simplified) state of the process (UNUSED OR OCCUPIED)
	ShadowPtr	receive_next;		// Pointer to the next process in the receive linked list
	ShadowPtr	send_next;		// Pointer to the next process in the send linked list
	ShadowPtr	order_next;		// Pointer to the next process in the order linked list
	SPtr		msg_slot;		// Pointer to the message slot for the process' message
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

// helper functions


// ----- Global data structures/vars

// ** I had to make the mailbox static since test44 had the 'mailbox' as a variable name causing it to not compile
MailBoxEntry mailbox[MAXMBOX];		// The Mailbox
Slot slots[MAXSLOTS];				// Global array of slots
ShadowPTE shadowTable[MAXPROC];		// The shaodw proc table (links to the proc table from Phase 1)

/**
 * 
 */
void phase2_init(void) {

}

/**
 * 
 */
void phase2_start_service_processes(void) {

}

/**
 * test
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