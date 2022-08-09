#include <iostream>

#include <boost/program_options.hpp>

#include <benchmark.hpp>

namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

int main(int argc, char* argv[]) {
   uint32_t num_runs = 1;
   std::string module_name;

   auto modules = benchmark::get_modules();

   options_description cli ("benchmark command line options");
   cli.add_options()
      ("module,m", bpo::value<std::string>(), "the module to be benchmarked; if this option is not present, all modules are benchmarked.")
      ("list,l", "list of supported modules")
      ("runs,r", bpo::value<uint32_t>(&num_runs)->default_value(1000), "the number of runs per function")
      ("help,h", "execute the benchmarked functions for the specified number of times, and report average, minimum, and maximum time in nanoseconds");

   variables_map vmap;
   try {
      bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
      bpo::notify(vmap);

      if (vmap.count("help") > 0) {
         cli.print(std::cerr);
         return 0;
      }

      if (vmap.count("list") > 0) {
         auto first = true;
         std::cout << "Supported modules are ";
         for (auto& [name, f]: modules) {
            if (first) {
               first = false;
            } else {
               std::cout << ", ";
            }
            std::cout << name;
         }
         std::cout << std::endl;
         return 0;
      }

      if (vmap.count("module") > 0) {
         module_name = vmap["module"].as<std::string>();
         if (modules.find(module_name) == modules.end()) {
            std::cout << module_name << " is not supported" << std::endl;
            return 1;
         }
      }
   } catch (bpo::unknown_option &ex) {
      std::cerr << ex.what() << std::endl;
      cli.print (std::cerr);
      return 1;
   } catch( ... ) {
      std::cerr << "unknown exception" << std::endl;
   }

   benchmark::set_num_runs(num_runs);
   benchmark::printt_header();

   if (module_name.empty()) {
      for (auto& [name, f]: modules) {
         std::cout << name << ":" << std::endl;
         f();
         std::cout << std::endl;
      }
   } else {
      std::cout << module_name << ":" << std::endl;
      modules[module_name]();
      std::cout << std::endl;
   }

   return 0;
}
