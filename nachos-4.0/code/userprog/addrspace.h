// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include <string.h>

#define UserStackSize		1024 	// increase this as necessary!

/* HW3 */
class NewTranslationEntry {
  public:
    unsigned int virtualPage;  	// The page number in virtual memory.
    unsigned int physicalPage;  // The page number in real memory (relative to the
			//  start of "mainMemory"
    bool valid;         // If this bit is set, the translation is ignored.
			// (In other words, the entry hasn't been initialized.)
    bool readOnly;	// If this bit is set, the user program is not allowed
			// to modify the contents of the page.
    bool use;           // This bit is set by the hardware every time the
			// page is referenced or modified.
    bool dirty;         // This bit is set by the hardware every time the
			// page is modified.
    /* HW3 自己加 */
    int LRU_counter;    // counter for LRU
    int ID;             // page table ID
    /* HW3 自己加 */

};
/* HW3 */

class AddrSpace {
  public:
    AddrSpace();			// Create an address space.
    ~AddrSpace();			// De-allocate an address space

    void Execute(char *fileName);	// Run the the program
					// stored in the file "executable"

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 

    static bool usedPhyPage[NumPhysPages];
    /* HW3 */
    NewTranslationEntry *pageTable;
    bool UsedPhyPage[NumPhysPages]; 
    bool UsedVirtualPage[NumPhysPages]; //record the pages in the virtual memory
    int ID_number; // machine ID
    int PhyPageInfo[NumPhysPages]; //record physical page info (ID)
    NewTranslationEntry *main_tab[NumPhysPages]; // pagetable
    int ID;
    /* HW3 */
  private:
    /* HW3 我試著把他移到 public */
    // TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    /* HW3 我試著把他移到 public */
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
    /* HW3 */
    bool pageTable_is_load;
    /* HW3 */

    bool Load(char *fileName);		// Load the program into memory
					// return false if not found

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

};

#endif // ADDRSPACE_H
