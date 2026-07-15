#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "set_device.h"
#include <cuda_runtime_api.h>

/*-------------------------------------------------------------------
 * Function:  Set_device
 */
int Set_device(void) {
   char* device_val = NULL;
   int device;

   device_val = getenv("DEVID");
   if (device_val == NULL || strlen(device_val) == 0) {
      fprintf(stderr, "DEVID not set.  Using device 0\n");
      device = 0;
   } else {
      device = strtol(device_val, NULL, 10);
      fprintf(stderr, "Using device %d\n", device);
   }
   cudaSetDevice(device);
   return device;
}  /* Set_device */
