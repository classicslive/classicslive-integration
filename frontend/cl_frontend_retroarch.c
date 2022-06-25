#include <command.h>
#include <network/net_http_special.h>
#include <tasks/tasks_internal.h>

#include "cl_frontend.h"
#include "../cl_memory.h"

void cl_fe_display_message(unsigned level, const char *msg)
{
   runloop_msg_queue_push(msg, 0, 2 * 60, false, NULL, 0, 0);
}

bool cl_fe_install_membanks(void)
{
   rarch_system_info_t* sys_info = runloop_get_system_info();
   unsigned num_descs;

   if (!sys_info)
      return false;
   num_descs = sys_info->mmaps.num_descriptors;

   /* 
      If a RetroArch mmap is available, copy it into a temporary array of 
      libretro descriptors, then run the generic libretro initializer.
   */
   if (num_descs > 0)
   {
      struct retro_memory_descriptor **descs = 
         (struct retro_memory_descriptor**)calloc(
            sizeof(struct retro_memory_descriptor*), num_descs);
      bool success;
      unsigned i;

      for (i = 0; i < num_descs; i++)
         descs[i] = &sys_info->mmaps.descriptors[i].core;
      success = cl_init_membanks_libretro(descs, num_descs);
      free(descs);

      return success;
   }
   /* No mmaps; we try retro_get_memory_data instead */
   /* TODO: Supporting this makes Dolphin misbehave... */
   else
   {
      retro_ctx_memory_info_t mem_info;

      mem_info.id = RETRO_MEMORY_SYSTEM_RAM;
      core_get_memory(&mem_info);

      /* Nothing here either, let's give up */
      if (!mem_info.data || !mem_info.size)
         return false;

      /* Copy RETRO_MEMORY_SYSTEM_RAM data into a single CL membank */
      memory.banks = (cl_membank_t*)malloc(sizeof(cl_membank_t));
      memory.banks[0].data = (uint8_t*)mem_info.data;
      memory.banks[0].size = mem_info.size;
      memory.banks[0].start = 0;
      snprintf(memory.banks[0].title, sizeof(memory.banks[0].title), "%s", 
         "RETRO_MEMORY_SYSTEM_RAM");
      memory.bank_count = 1;

      return true;
   }

   return false;
}

void cl_fe_pause(void)
{
   command_event(CMD_EVENT_PAUSE, NULL);
}

static void cl_retroarch_callback(retro_task_t *ra_task, void *task_data,
   void *user_data, const char *error)
{
   cl_task_t *cl_task = (cl_task_t*)ra_task->state;

   if (cl_task->callback)
      cl_task->callback(cl_task);
}

static void cl_retroarch_task(retro_task_t *ra_task)
{
   cl_task_t *cl_task = (cl_task_t*)ra_task->state;

   cl_task->handler(cl_task);

   task_set_finished(ra_task, true);
}

void cl_fe_network_post(const char *url, const char *data, void *callback)
{
   task_push_http_post_transfer(url, data, true, "POST", callback, NULL);
}

void cl_fe_thread(cl_task_t *cl_task)
{
   retro_task_t *ra_task = task_init();

   ra_task->handler  = cl_retroarch_task;
   ra_task->state    = cl_task;
   ra_task->callback = cl_retroarch_callback;

   task_queue_push(ra_task);
}

void cl_fe_unpause(void)
{
   command_event(CMD_EVENT_UNPAUSE, NULL);
}
