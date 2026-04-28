#include "mainFirmware.h"

#if defined(DUAL_CORE)

#if defined(CORE_CM7)
 void mainFirmware(void)
    {
       while (true)
       {
          // Main firmware loop code goes here
       }
    } 
#elif defined(CORE_CM4)
    void mainFirmware(void)
    {
       while (true)
       {
          // Main firmware loop code goes here
       }
    }
#else
    #error "Unknown core type! Please define CORE_CM7 or CORE_CM4."
#endif
#else
    void mainFirmware(void)
    {
       while (true)
       {
          // Main firmware loop code goes here
       }
    } 
#endif