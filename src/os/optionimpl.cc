#include "include/os/option.hpp"
#include "global/global_init.h"
#include "global/global_context.h"
#include "common/config.h"
#include "include/msgr.h"

libobjectstore::Option::Option()
{
  global_init(NULL, args, CEPH_ENTITY_TYPE_CLIENT, CODE_ENVIRONMENT_UTILITY, 0);  
  common_init_finish(g_ceph_context);
}


bool libobjectstore::Option::set(std::string option_key, std::string option_value)
{
  int res = g_ceph_context->_conf->set_val(option_key.c_str(), option_value.c_str());
  if (res < 0) return false;
  return true;
}

bool libobjectstore::Option::get(std::string option_key, std::string* option_value)
{
  return true;
}

void libobjectstore::Option::apply_changes()
{
  g_ceph_context->_conf->apply_changes(NULL);
}
