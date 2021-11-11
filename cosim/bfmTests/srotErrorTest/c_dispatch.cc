//************************************************************************
// Copyright 2021 Massachusetts Institute of Technology
// SPDX short identifier: BSD-2-Clause
//
// File Name:      
// Program:        Common Evaluation Platform (CEP)
// Description:    
// Notes:          
//
//************************************************************************
#include <unistd.h>
#include "v2c_cmds.h"
#include "access.h"
#include "c_dispatch.h"
#include "c_module.h"
#include "cep_adrMap.h"
#include "cep_apis.h"
#include "simPio.h"

/*
 * main 
 */
int main(int argc, char *argv[])
{

  /* ===================================== */
  /*   SETUP SECTION FOR SIMULATION        */
  /* ===================================== */
  unsigned long seed;
  sscanf(argv[1],"0x%x",&seed);  
  printf("Seed = 0x%x\n",seed);
  int errCnt = 0;
  int verbose = 0x1f;

  /* ===================================== */
  /*   spawn all the paralle threads       */
  /* ===================================== */
  int activeSlot = 0; // only 1 board


  //
  // ============================  
  // fork all the tests here
  // ============================  
  //
  shPthread thr;
  //
  // max number of cores not include the system thread
  //
  int maxHost = MAX_CORES; // number of cores/threads

  // Select ONE random core to use for this test
  //long unsigned int mask = 1 << (seed & 0x3);
  long unsigned int mask = 0x2;

  // Set the active CPU mask before spawning any threads
  //   Note: This mask is not visible from within the thread, but
  //   merely controls the C <--> Simulation communication
  thr.SetActiveMask(mask);

  // Launch threads based on the mask
  for (int i = 0; i < maxHost ; i++) {
    if ((long unsigned int)(1 << i) & mask) {
      thr.ForkAThread(  activeSlot,     // slotId         - Unused
                        i,              // cpuId          - Used within the thread to identify which core it is
                        verbose,        // verbose        - Verbosity parameter
                        mask,   		// Accessible via tParm->seed from within the thread
                        c_module        // *start_routine - Pointer to spawned thread entry function
                        );
    }
  }
  
  // Launch the system thread
  thr.AddSysThread(SYSTEM_SLOT_ID,SYSTEM_CPU_ID);

  //
  // ============================
  // Turn on the wave here
  // ============================  
  //
  int cycle2start=0;
  int cycle2capture=-1; // til end
  int wave_enable=1;
  #ifndef NOWAVE
  dump_wave(cycle2start, cycle2capture, wave_enable);
  #endif
  //
  // Enable main memory logging
  //
  //DUT_WRITE_DVT(DVTF_ENABLE_MAIN_MEM_LOGGING, DVTF_ENABLE_MAIN_MEM_LOGGING, 1);
  DUT_WRITE_DVT(DVTF_ENABLE_MAIN_MEMWR_LOGGING, DVTF_ENABLE_MAIN_MEMWR_LOGGING, 1);
  //
  // ============================  
  // wait until all the threads are done
  // ============================  
  //
  int Done = 0;
  while (!Done) {
    Done = thr.AllThreadDone();
    sleep(2);
  }
  /* ===================================== */
  /*   END-OF-TEST CHECKING                */
  /* ===================================== */
  errCnt += thr.GetErrorCount();
  if (errCnt != 0) {
    LOGE("======== TEST FAIL ========== %x\n",errCnt);
  } else {
    LOGI("%s ======== TEST PASS ========== \n",__FUNCTION__);
  }
  //
  // shutdown HW side
  //
  thr.Shutdown();
  return(errCnt);
}