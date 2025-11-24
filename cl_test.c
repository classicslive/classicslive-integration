#include "cl_counter.h"
#include "cl_frontend.h"

/* Provide stub implementations of every frontend function */
void cl_fe_display_message(unsigned level, const char *msg) {}
bool cl_fe_install_membanks(void) { return false; }
const char *cl_fe_library_name(void) { return 0; }
void cl_fe_network_post(const char *url, char *data, cl_network_cb_t callback, void *userdata) {}
void cl_fe_pause(void) {}
void cl_fe_thread(cl_task_t *task) {}
void cl_fe_unpause(void) {}
bool cl_fe_user_data(cl_user_t *user, unsigned index) { return false; }

int main(void)
{
  return cl_ctr_tests();
}
