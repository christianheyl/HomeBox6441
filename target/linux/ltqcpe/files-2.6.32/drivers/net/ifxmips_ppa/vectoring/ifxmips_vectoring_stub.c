#include <linux/kernel.h>
#include <linux/module.h>

int32_t (*mei_dsm_cb_func_hook)(uint32_t *p_error_vector) = NULL;
EXPORT_SYMBOL(mei_dsm_cb_func_hook);

MODULE_LICENSE("GPL");

