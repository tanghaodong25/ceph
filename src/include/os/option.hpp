#include "global/global_init.h"
#include <string>
#include <vector>

namespace libobjectstore
{
  class Option
  {
    public:
      Option();
      ~Option();
      bool set_from_json(std::string adr);
      bool set(std::string option_key, std::string option_value);
      bool get(std::string option_key, std::string* option_value);
      void apply_changes();
    private:
      std::vector<const char*> args;
      std::string store_data_path;
      std::string store_journal_path;
  };
}
