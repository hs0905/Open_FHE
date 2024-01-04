#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/param.h>
#include <errno.h>

#include <memory.h>
#include <math.h>
#include <time.h>
#include <mutex>
#include <vector>
#include <deque>
#include <utility>
#include <tuple>
#include <functional>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <list>
#include <iostream>


#define FPGA_N 65536
#define OCB_MB 100 // OCB = On-Chip Buffer
#define HBM_GB 8
#define POLY_SIZE (8 * FPGA_N)
#define MB_TO_BYTES 1048576 // 1 MB = 1024 * 1024 Bytes
#define GB_TO_MB 1024 // 1 GB = 1024 MB
#define MB_TO_ENTRIES_NUM (MB_TO_BYTES / POLY_SIZE)
#define OCB_ENTRIES_NUM (OCB_MB * MB_TO_ENTRIES_NUM)
#define HBM_ENTRIES_NUM (HBM_GB * GB_TO_MB * MB_TO_ENTRIES_NUM)