// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
	int	type = kernel->machine->ReadRegister(2);
	int	val;

    switch (which) {
	case SyscallException:
	    switch(type) {
		case SC_Halt:
		    DEBUG(dbgAddr, "Shutdown, initiated by user program.\n");
   		    kernel->interrupt->Halt();
		    break;
		case SC_PrintInt:
			val=kernel->machine->ReadRegister(4);
			cout << "Print integer:" <<val << endl;
			return;
/*		case SC_Exec:
			DEBUG(dbgAddr, "Exec\n");
			val = kernel->machine->ReadRegister(4);
			kernel->StringCopy(tmpStr, retVal, 1024);
			cout << "Exec: " << val << endl;
			val = kernel->Exec(val);
			kernel->machine->WriteRegister(2, val);
			return;
*/		case SC_Exit:
			DEBUG(dbgAddr, "Program exit\n");
			val=kernel->machine->ReadRegister(4);
			cout << "return value:" << val << endl;
			kernel->currentThread->Finish();
			break;
		case SC_Sleep:
			val=kernel->machine->ReadRegister(4);
			cout << "Sleep time:" << val << endl;
			kernel->alarm->WaitUntil(val);
			return;
		default:
		    cerr << "Unexpected system call " << type << "\n";
 		    break;
	    }
	    break;

	case PageFaultException:
		unsigned int j = 0;
		int victim;
		// Fetch virtual address
		int pageFaultAddr = kernel->machine->ReadRegister(BadVAddrReg);
		// Fetch virtual page number from thread pageTable
		int vpn = pageFaultAddr / PageSize;
		//check the free physical page number from main memory
		int PPN = kernel->freeMap->FindAndSet();  // 找尋 free block space
		//Fetch page entry of current thread [by VPN(pageFaultNum)]
		kernel->stats->numPageFaults++; // page fault

		while(UsedPhyPage[j] != FALSE && j < NumPhysPages){
			j++;
		}
		// load the page into the main memory if the main memory is not full  
		if(j < NumPhysPages) {
			char *buffer; //temporary save page 
			buffer = new char[PageSize];
			UsedPhyPage[j]=TRUE;
			PhyPageInfo[j]=pageTable[vpn].ID;
			main_tab[j]=&pageTable[vpn];
			pageTable[vpn].physicalPage = j;
			pageTable[vpn].valid = TRUE;
			pageTable[vpn].LRU_counter++; // counter for LRU

			kernel->SwapDisk->ReadSector(pageTable[vpn].virtualPage, buffer);
			bcopy(buffer,&mainMemory[j*PageSize],PageSize);
		}
		else {
			char *buffer1;
			char *buffer2;
			buffer1 = new char[PageSize];
			buffer2 = new char[PageSize];

			// LRU
			int min = pageTable[0].LRU_counter;
			victim=0;
			for(int index=0; index < 32; index++) {
				if(min > pageTable[index].LRU_counter) {
					min = pageTable[index].LRU_counter;
					victim = index;
				}
			}
			pageTable[victim].LRU_counter++;

			// perform page replacement, write victim frame to disk, read desired frame to memory
			bcopy(&mainMemory[victim*PageSize],buffer1,PageSize);
			kernel->SwapDisk->ReadSector(pageTable[vpn].virtualPage, buffer2);
			bcopy(buffer2,&mainMemory[victim*PageSize],PageSize);
			kernel->SwapDisk->WriteSector(pageTable[vpn].virtualPage,buffer1);

			main_tab[victim]->virtualPage=pageTable[vpn].virtualPage;
			main_tab[victim]->valid=FALSE;

			pageTable[vpn].valid = TRUE;
			pageTable[vpn].physicalPage=victim;
			PhyPageInfo[victim]=pageTable[vpn].ID;
			main_tab[victim]=&pageTable[vpn];
		}

		return;

	default:
	    cerr << "Unexpected user mode exception" << which << "\n";
	    break;
    }
    ASSERTNOTREACHED();
}
