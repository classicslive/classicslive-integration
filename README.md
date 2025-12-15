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

- Add [libretro-common](https://github.com/libretro/libretro-common) to your project.
- Set any configuration variables (see [cl_config.h](https://github.com/classicslive/classicslive-integration/blob/master/cl_config.h) for a list of configuration variables and their default values).

### Project integration

#### For C / Makefile projects

**1)** Add any compile-time configurations to your Makefile:

```make
CFLAGS += -DCL_HAVE_FILESYSTEM=1
```

**2)** Include the classicslive-integration Makefile:

```make
include classicslive-integration/classicslive-integration.mk
```

**3)** Add the exported `CLASSICS_LIVE_SOURCES` to your own, for example:

```make
SOURCES += $(CLASSICS_LIVE_SOURCES)
```

---

#### For Qt (qmake) projects

**1)** Add any compile-time configurations to the defines in your qmake project (.pro):

```qmake
DEFINES += \
  CL_EXTERNAL_MEMORY=1 \
  CL_HAVE_EDITOR=1 \
  CL_HAVE_FILESYSTEM=1
```

**2)** Add the classicslive-integration project include file (.pri) to your qmake project (.pro):

```qmake
include(classicslive-integration/classicslive-integration.pri)
```

### ABI implementation

Projects using classicslive-integration must provide it with implementations for the functions detailed in **[cl_abi.h](https://github.com/classicslive/classicslive-integration/blob/master/cl_abi.h)**.

First, write the implementations:
```c
/* Make an implementation of the first ABI function... */
static cl_error cl_custom_display_message(unsigned level, const char *msg)
{
  if (level >= CL_MSG_INFO)
    printf(msg);

  return CL_OK;
}

/* Make an implementation of the second ABI function... */
static cl_error cl_custom_install_membanks(cl_memory_region_t **regions,
  unsigned *region_count)
{
  *region_count = 1;
  *regions = (cl_memory_region_t*)malloc(sizeof(cl_memory_region_t) * *region_count);
  *regions[0].base_guest = 0x80000000;
  *regions[0].size = my_ram_size;
  *regions[0].base_host = my_ram_data;
  *regions[0].endianness = CL_ENDIAN_BIG;
  *regions[0].pointer_length = 4;

  return CL_OK;
}

/* And so on for the rest of the functions detailed in cl_abi.h */
```

Next, define a structure of your function implementations:
```
static const cl_abi_t cl_test_abi =
{
  CL_ABI_VERSION,
  {
    {
      cl_custom_display_message,
      cl_custom_install_membanks,
      /* ...the rest of the functions... */
    },
    { NULL, NULL }
  }
};
```

Finally, register your ABI implementation with classicslive-integration.
```c
int main(void)
{
  if (cl_abi_register(&cl_test_abi) == CL_OK)
  {
    /* Continue with your program... */
  }
}
```

> **Tip:** See `cl_test.c` for a full example.

## License

**classicslive-integration** is provided under the MIT license. See `LICENSE` for more information.

**JSONSAX** by *John-Anthony Owens* with modifications by *The RetroArch Team* and the author is used under the MIT license. See `3rdparty/jsonsax/LICENSE` for more information.
