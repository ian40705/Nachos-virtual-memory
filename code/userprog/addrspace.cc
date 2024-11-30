// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

/*
bool AddrSpace::usedPhyPage[NumPhysPages] = {0};
*/
static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------


AddrSpace::AddrSpace()
{
    //add
    ID=(kernel->machine->ID_num)++;
    kernel->machine->ID_num=(kernel->machine->ID_num)++;
    /*
    pageTable = new TranslationEntry[NumPhysPages];
    for (unsigned int i = 0; i < NumPhysPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virt page # = phys page #
	pageTable[i].physicalPage = i;
//	pageTable[i].physicalPage = 0;
	pageTable[i].valid = TRUE;
//	pageTable[i].valid = FALSE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  
    }
    */
    // zero out the entire address space
//    bzero(kernel->machine->mainMemory, MemorySize);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
 //  for(int i = 0; i < numPages; i++)
   //     AddrSpace::usedPhyPage[pageTable[i].physicalPage] = false;
   delete pageTable;
}


//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool AddrSpace::Load(char *fileName) 
{
    DEBUG(dbgAddr, "AddrSpace Load()");

    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size,tmp;

    if (executable == NULL) {
        cerr << "Unable to open file " << fileName << "\n";
        return FALSE;
    }

    // 讀取檔案頭
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC)) {
        SwapHeader(&noffH);
    }
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // 計算總共需要的虛擬內存大小，包括代碼段、數據段和堆疊空間
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    // 檢查是否有足夠的內存
    if (numPages > NumPhysPages && kernel->virtualMemoryDisk == NULL) {
        cerr << "Not enough memory for the process, and no virtual memory disk available.\n";
        delete executable;
        return FALSE;
    }

    // 初始化頁表
    pageTable = new TranslationEntry[numPages];
    /*
    for (unsigned int i = 0; i < numPages; i++) {
        pageTable[i].virtualPage = i;   // 虛擬頁面號
        pageTable[i].physicalPage = -1; // 尚未分配物理頁
        pageTable[i].valid = FALSE;    // 頁面默認無效
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE; // 頁面可讀寫
        pageTable[i].count = 0;
    }
    */

   // then, copy in the code and data segments into memory
	if (noffH.code.size > 0) {
     
        for(unsigned int j=0,i=0;i < numPages ;i++){
            j=0;
            while(kernel->machine->usedPhyPage[j]!=FALSE&&j<NumPhysPages){j++;}

            // main memory is enough, put the page to main memory
            if(j<NumPhysPages){   
                kernel->machine->usedPhyPage[j]=TRUE;
                kernel->machine->PhyPageName[j]=ID;
                kernel->machine->main_tab[j]=&pageTable[i];
                pageTable[i].physicalPage = j;
                pageTable[i].valid = TRUE;
                pageTable[i].use = FALSE;
                pageTable[i].dirty = FALSE;
                pageTable[i].readOnly = FALSE;
                pageTable[i].ID =ID;
                pageTable[i].count++; // LRU counter when save in memory
                executable->ReadAt(&(kernel->machine->mainMemory[j*PageSize]),PageSize, noffH.code.inFileAddr+(i*PageSize));  
            }
            // main memory is not enough, use virtual memory
            else{ 
                char *buffer;
                buffer = new char[PageSize];
                tmp=0;
                while(kernel->machine->usedvirPage[tmp]!=FALSE){tmp++;}
                kernel->machine->usedvirPage[tmp]=true;
                pageTable[i].virtualPage=tmp; //record the virtual page we save 
                pageTable[i].valid = FALSE; //not load in main memory
                pageTable[i].use = FALSE;
                pageTable[i].dirty = FALSE;
                pageTable[i].readOnly = FALSE;
                pageTable[i].ID =ID;
                executable->ReadAt(buffer,PageSize, noffH.code.inFileAddr+(i*PageSize));
                kernel->virtualMemoryDisk->WriteSector(tmp,buffer); // write in virtual memory 

            }
        }
    }

    if (noffH.initData.size > 0) {
    
        // add
        executable->ReadAt(
        &(kernel->machine->mainMemory[noffH.initData.virtualAddr]),
            noffH.initData.size, noffH.initData.inFileAddr);
    }
 

    delete executable; // 關閉文件
    return TRUE;
}

void AddrSpace::LoadSegment(OpenFile *executable, int inFileAddr, int segmentSize, int virtualAddr) 
{
    int startPage = virtualAddr / PageSize;
    int pageOffset = virtualAddr % PageSize;

    for (int i = 0; i < segmentSize; i += PageSize) {
        int vpn = startPage + i / PageSize; // 計算虛擬頁面號
        int copySize = min(PageSize - pageOffset, (unsigned)(segmentSize - i));

        // 查找空閒物理頁面
        int freePage = -1;
        for ( int j = 0; j < NumPhysPages; j++) {
            if (!kernel->machine->usedPhyPage[j]) {
                freePage = j;
                kernel->machine->usedPhyPage[j] = TRUE;
                break;
            }
        }

        if (freePage != -1) {
            DEBUG(dbgAddr, "Find free physical page");
            // 分配物理頁面
            pageTable[vpn].physicalPage = freePage;
            pageTable[vpn].valid = TRUE;

            // 加載數據到內存
            executable->ReadAt(&(kernel->machine->mainMemory[freePage * PageSize + pageOffset]),
                               copySize, inFileAddr + i);
        } else {
            // 處理虛擬內存
            DEBUG(dbgAddr, "No free physical page, swapping to disk...");

            char *buffer = new char[PageSize];
            executable->ReadAt(buffer, copySize, inFileAddr + i);

            int swapPage = FindSwapPage();
            kernel->machine->usedvirPage[swapPage] = TRUE;
            kernel->virtualMemoryDisk->WriteSector(swapPage, buffer);

            pageTable[vpn].virtualPage = swapPage;
            pageTable[vpn].valid = FALSE;
            delete[] buffer;
        }

        pageOffset = 0; // 清除偏移，後續頁面對齊
    }
}

int AddrSpace::FindSwapPage() 
{
    for (int i = 0; i < NumPhysPages; i++) {
        if (!kernel->machine->usedvirPage[i]) {
            return i;
        }
    }
    ASSERT(FALSE); // 如果找不到可用的交換頁，則應觸發錯誤
    return -1;
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	(for now) use our own thread to run it.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void 
AddrSpace::Execute(char *fileName) 
{
     pageTableLoaded=FALSE;
    if (!Load(fileName)) {
	cout << "inside !Load(FileName)" << endl;
	return;				// executable not found
    }

    //kernel->currentThread->space = this;
    this->InitRegisters();		// set the initial register values
    this->RestoreState();		// load page table register
    pageTableLoaded=TRUE;
    kernel->machine->Run();		// jump to the user progam

    ASSERTNOTREACHED();			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    if(pageTableLoaded){
        pageTable=kernel->machine->pageTable;
        numPages=kernel->machine->pageTableSize;
}
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}
