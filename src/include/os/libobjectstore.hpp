#if __GNUC__ >= 4
  #define CEPH_RADOS_API  __attribute__ ((visibility ("default")))
#else
  #define CEPH_RADOS_API
#endif

#include <iostream>

namespace libobjectstore
{
  class CEPH_RADOS_API DB
  {
    public:
      DB() 
      {
        std::cout << "constructor..." << std::endl;
      }
      ~DB()
      {
        std::cout << "deconstructor..." << std::endl;
      }
      void create();
      void close()
      {
        std::cout << "closing..." << std::endl; 
      }
      void test();
  };
}
