#include "include/os/libobjectstore.hpp"
#include "include/msgr.h"
#include "global/global_init.h"
#include "global/global_context.h"
#include "common/ceph_argparse.h"
#include "common/config.h"

#include "os/ObjectStore.h"

void libobjectstore::DB::test()
{
  vector<const char*> args;

  global_init(NULL, args, CEPH_ENTITY_TYPE_CLIENT, CODE_ENVIRONMENT_UTILITY, 0);
  common_init_finish(g_ceph_context);

  std::cout << "creating..." << std::endl;
  g_ceph_context->_conf->set_val("osd_journal_size", "400");
  g_ceph_context->_conf->set_val("filestore_index_retry_probability", "0.5");
  g_ceph_context->_conf->set_val("filestore_op_thread_timeout", "1000");
  g_ceph_context->_conf->set_val("filestore_op_thread_suicide_timeout", "10000");
  g_ceph_context->_conf->set_val("filestore_debug_disable_sharded_check", "true");
  g_ceph_context->_conf->set_val("filestore_fiemap", "true");
  g_ceph_context->_conf->set_val("bluestore_fsck_on_mount", "true");
  g_ceph_context->_conf->set_val("bluestore_fsck_on_umount", "true");
  g_ceph_context->_conf->set_val("bluestore_debug_misc", "true");
  g_ceph_context->_conf->set_val("bluestore_debug_small_allocations", "4");
  g_ceph_context->_conf->set_val("bluestore_debug_freelist", "true");
  g_ceph_context->_conf->set_val("bluestore_clone_cow", "true");
  g_ceph_context->_conf->set_val("bluestore_max_alloc_size", "196608");

  // set small cache sizes so we see trimming during Synthetic tests
  g_ceph_context->_conf->set_val("bluestore_buffer_cache_size", "2000000");
  g_ceph_context->_conf->set_val("bluestore_onode_cache_size", "500");

  g_ceph_context->_conf->set_val(
    "enable_experimental_unrecoverable_data_corrupting_features", "*");
  g_ceph_context->_conf->apply_changes(NULL);

  int r = ::mkdir("store_test_temp_dir", 0777);
  int j = ::mkdir("store_test_temp_journal", 0777);
  if (r < 0 || j < 0) {
    std::cout << "error during mkdiring..." << std::endl;
    return;
  }
  boost::scoped_ptr<ObjectStore> store(0);
  ObjectStore *store_ = ObjectStore::create(g_ceph_context, "bluestore", "store_test_temp_dir", "store_test_temp_journal"); 
  if (!store_) {
    std::cout << "created" << std::endl;
  }
  store_->mkfs();
  store_->mount();
  store.reset(store_);

  std::cout << "start unmount...." << std::endl;
  if (store)
  {
    store->umount(); 
  } 

}

void libobjectstore::DB::create()
{

}
