// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"

//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
}

//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//	Also, to keep from looping forever, we check if there's
//	nothing on the ready list, and there are no other pending
//	interrupts.  In this case, we can safely halt.
//----------------------------------------------------------------------

void 
Alarm::CallBack() 
{
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    bool woken = sleepList.PutToReady();

    // 多加上兩個條件，一個是檢查是否 woken，一個是檢查還有沒有正在 sleep 的 thread
    // 若是都沒有就可以關掉 timer 了
    if (status == IdleMode && !woken && sleepList.IsEmpty()) { // is it time to quit?
        if (!interrupt->AnyFutureInterrupts()) {
     timer->Disable(); // turn off the timer
	}
    } else {			// there's someone to preempt
	// interrupt->YieldOnReturn();
    /* Morris 寫法 */
    if(kernel->scheduler->getSchedulerType() == RR || kernel->scheduler->getSchedulerType() == Priority) {
        // 決定是否要呼叫 interrupt->YieldOnReturn() 查看是否有更需要優先的 process 要執行
        // cout << "=== interrupt->YieldOnReturn ===" << endl;
        interrupt->YieldOnReturn();
        }
    }
    /* Morris 寫法 */
}

// implement WaitUntil function.
void 
Alarm::WaitUntil(int x)
{
    // save previous setting as oldLevel and disable interrupt.
    // SetLevel 是決定這個 thread 能不能被 interrupt
    // 這裡是把原本的 level 存起來然後把當前設定成不能被 interrupt
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
    Thread* t = kernel->currentThread;
    /* Morris 寫法 */
    int worktime = kernel->stats->userTicks - t->getStartTime();
    t->setBurstTime(t->getBurstTime() + worktime);
    t->setStartTime(kernel->stats->userTicks);
    /* Morris 寫法 */
    sleepList.PutToSleep(t, x);               // put current thread to sleep list.
    kernel->interrupt->SetLevel(oldLevel);    // recover old interrupt state.
}

// check if there is still thread sleeping
bool SleepList::IsEmpty()
{
    return threadlist.size() == 0;
}

// put the thread into sleep list
void SleepList::PutToSleep(Thread*t, int x)
{
    // 2-3 打開
    // IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff); // 不讓其他 thread 可以中斷
    
    // check if it cannot be interrupt.
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    threadlist.push_back(SleepThread(t, counter + x));  // put into the list
    t->Sleep(false);

    // 2-3 打開
    // kernel->interrupt->SetLevel(oldLevel);
}

// will be call in callback
bool SleepList::PutToReady()
{
    // 2-3 打開
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
    
    bool woken = false;
    counter ++;

    // check all thread in the list if there are thread already finish sleeping
    for(std::list<SleepThread>::iterator it = threadlist.begin();
        it != threadlist.end(); )
    {
        // 'when' 就是被創造時的 counter 加上他要 sleep 的時間，也就是他應該醒來的時間
        // 所以我們檢查 when 跟 counter 來判斷他該不該醒來
        // 'when' is time the thread should wake up
        // if counter >= when, this thread will be ready to run.
        if(counter >= it->when)
        {
            // 若是他該醒來就把從睡覺 list 中去掉，然後把他叫醒 (用 ReadyToRun)
            woken = true;
            kernel->scheduler->ReadyToRun(it->sleeper);
            it = threadlist.erase(it);
        }
        // if the thread is not ready to run, keep checking next thread in the list.
        else
        {
            it++;
        }
    }

    // 2-3 打開
    kernel->interrupt->SetLevel(oldLevel);
    
    return woken;
}