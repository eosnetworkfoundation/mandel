#include <eosio/testing/tester.hpp>
#include <fc/log/logger_config.hpp>
#include <eosio/chain/deep_mind.hpp>

#include <boost/test/unit_test.hpp>

#include <deep-mind.hpp>

using namespace eosio::testing;

extern void setup_test_logging();

struct deep_mind_log_fixture
{
   deep_mind_handler deep_mind_logger;
   fc::temp_file log_output;

   deep_mind_log_fixture(const std::optional<std::string>& fifo_path={})
   {
      auto cfg = fc::logging_config::default_config();

      if (fifo_path) {
         cfg.appenders.push_back(
            appender_config( "deep-mind", "dmlog",
               mutable_variant_object()
                  ( "fifo", fifo_path->c_str())
         ) );
      } else {
         cfg.appenders.push_back(
            appender_config( "deep-mind", "dmlog",
               mutable_variant_object()
                  ( "file", log_output.path().preferred_string().c_str())
         ) );
      }

      fc::logger_config lc;
      lc.name = "deep-mind";
      lc.level = fc::log_level::all;
      lc.appenders.push_back("deep-mind");
      cfg.loggers.push_back( lc );

      fc::configure_logging(cfg);
      setup_test_logging();

      deep_mind_logger.update_config(deep_mind_handler::deep_mind_config{.zero_elapsed = true});
      deep_mind_logger.update_logger("deep-mind");
   }
   ~deep_mind_log_fixture()
   {
     std::cout << "~deep_mind_log_fixture\n";
      fc::configure_logging(fc::logging_config::default_config());
      setup_test_logging();
   }
};

struct deep_mind_tester : deep_mind_log_fixture, validating_tester
{
   deep_mind_tester(const std::optional<std::string>& fifo_path={}) : deep_mind_log_fixture(fifo_path), validating_tester({}, &deep_mind_logger) {}
};

namespace {

void compare_files(const std::string& filename1, const std::string& filename2)
{
   std::ifstream file1(filename1.c_str());
   std::ifstream file2(filename2.c_str());
   std::string line1, line2;
   int i = 1;
   while(std::getline(file2, line2))
   {
      if(!std::getline(file1, line1))
      {
         BOOST_TEST(false, "Unexpected end of input of file1 at line " << i);
         return;
      }
      if(line1 != line2)
      {
         BOOST_TEST(false, "Mismatch at line " << i << "\n+ " << line1 << "\n- " << line2);
         return;
      }
      ++i;
   }
   if(std::getline(file1, line1))
   {
      BOOST_TEST(false, "Expected end of file of file1 at line " << i);
   }
}

} // namespace

BOOST_AUTO_TEST_SUITE(deep_mind_tests)

BOOST_FIXTURE_TEST_CASE(deep_mind, deep_mind_tester)
{
   produce_block();

   create_account( "alice"_n );

   push_action(config::system_account_name, "updateauth"_n, "alice"_n, fc::mutable_variant_object()
               ("account", "alice")
               ("permission", "test1")
               ("parent", "active")
               ("auth", authority{{"eosio"_n, "active"_n}}));

   produce_block();

   bool save_log = [](){
      auto argc = boost::unit_test::framework::master_test_suite().argc;
      auto argv = boost::unit_test::framework::master_test_suite().argv;
      return std::find(argv, argv + argc, std::string("--save-dmlog")) != (argv + argc);
   }();

   if(save_log)
   {
      // Cannot use fc::copy as it cannot overwrite an existing destination file 
      fc::rename(log_output.path().preferred_string(), DEEP_MIND_LOGFILE);
   }
   else
   {
      compare_files(log_output.path().preferred_string(), DEEP_MIND_LOGFILE);
   }
}

BOOST_AUTO_TEST_CASE(deep_mind_fifo_test)
{
   fc::temp_file fifo, regular;

   if (mkfifo(fifo.path().preferred_string().c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0) {
      FC_THROW("mkfifo failed for path ${name}, errno ${errno} --${errstr}", ("name", fifo.path().preferred_string()) ("errno", errno) ("errstr", strerror(errno)));
   }

   std::string cmd = "cat < " + fifo.path().preferred_string() + " > " + regular.path().preferred_string();
   auto fpin =  popen(cmd.c_str(), "r");
   if (fpin == nullptr) {
      std::cout << "popen failed" << strerror(errno) << std::endl;
   }
   sleep(2);

   {
      deep_mind_tester t { fifo.path().preferred_string() };
      t.produce_block();
      t.create_account( "alice"_n );
      t.push_action(config::system_account_name, "updateauth"_n, "alice"_n, fc::mutable_variant_object()
                  ("account", "alice")
                  ("permission", "test1")
                  ("parent", "active")
                  ("auth", authority{{"eosio"_n, "active"_n}}));
      t.produce_block();
   }

   compare_files(regular.path().preferred_string(), DEEP_MIND_LOGFILE);
}

BOOST_AUTO_TEST_SUITE_END()
