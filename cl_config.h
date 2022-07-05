#ifndef CL_CONFIG_H
#define CL_CONFIG_H

#ifndef CL_HAVE_EDITOR
/* Whether or not the Classics Live Editor is included in this build. */
#define CL_HAVE_EDITOR true
#endif

#ifndef CL_EXTERNAL_MEMORY
/*
  Whether or not the target memory is external to this program, ie. being read
  from another process.
  If true, the frontend needs to supply cl_fe_memory_read and write.
*/
#define CL_EXTERNAL_MEMORY false
#endif

#ifndef CL_LIBRETRO
/* Whether or not this implementation is a libretro frontend. */
#define CL_LIBRETRO true
#endif

#endif
