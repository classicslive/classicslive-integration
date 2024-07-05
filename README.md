# classicslive-integration

classicslive-integration is a C library created for interfacing with the Classics Live website, used in the following projects:

- [Pleasant](https://github.com/classicslive/Pleasant)
- [Classics Live Standalone](https://github.com/classicslive/classicslive-standalone)
- [Classics Live WUPS Plugin](https://github.com/classicslive/classicslive-wups-plugin)

## Features

- [Memory management](https://github.com/classicslive/classicslive-integration/blob/master/cl_memory.h): Interface for easily retrieving from or writing to the memory of a statically linked, dynamically linked, or external program
- [Memory searching](https://github.com/classicslive/classicslive-integration/blob/master/cl_search.h): Interface for rapidly finding desired memory addresses within the program's virtual memory
- [Software indentification](https://github.com/classicslive/classicslive-integration/blob/master/cl_identify.c): Computing hashes of various software formats to use as unique identifiers for website requests
- [Scripts](https://github.com/classicslive/classicslive-integration/blob/master/cl_script.h), [actions](https://github.com/classicslive/classicslive-integration/blob/master/cl_action.h), and [counters](https://github.com/classicslive/classicslive-integration/blob/master/cl_counter.h): Runtime script processing to act on such values and perform math, bitwise, and logic operations

## Building

- Add [libretro-common](https://github.com/libretro/libretro-common) and [jsonsax](https://github.com/johnanthonyowens/jsonsax) to your project.
- Set any configuration variables (see [cl_config.h](https://github.com/classicslive/classicslive-integration/blob/master/cl_config.h) for a list of configuration variables and their default values).
- Provide implementations for the functions prototyped in [cl_frontend.h](https://github.com/classicslive/classicslive-integration/blob/master/cl_frontend.h).
- Add the C files in this library to your makefile.
