#include <eosio/testing/tester.hpp>

#include <boost/test/unit_test.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio::testing;
using namespace eosio::chain;
using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(consensus_tests)

BOOST_FIXTURE_TEST_CASE( single_producer, TESTER ) try {
   for ( int i = 0; i < 5; ++i ) {
      produce_block();
      auto state = control->head_block_state();
      BOOST_TEST( state->last_irreversible_block_num() == state->block_num - 1);
   }
} FC_LOG_AND_RETHROW()

struct consensus_tester : TESTER
{
   // \post the head block has the given block number
   // \post the producer schedule is set to producers
   // \post the pending block is the first block to be created by the new producer schedule
   // \post LIB is the block before the head block
   void setup( std::initializer_list<account_name> producers, uint32_t block_num )
   {
      create_accounts( producers );
      while ( control->head_block_num() < block_num - 3 ) {
         produce_block();
      }
      set_producers( producers );
      produce_and_check( block_num - 2, block_num - 3, N(eosio) );
      BOOST_TEST( control->active_producers().version == 0 );
      produce_and_check( block_num - 1, block_num - 2, N(eosio) );
      BOOST_TEST( control->active_producers().version == 1 );
      produce_and_check( block_num - 0, block_num - 1, N(eosio) );
   }
   void check( uint32_t block_num, uint32_t last_irreversible_block_num, account_name prod = account_name() ) {
      auto state = control->head_block_state();
      BOOST_TEST( state->block_num == block_num );
      BOOST_TEST_CONTEXT( "block_num = " << block_num ) {
         BOOST_TEST( state->last_irreversible_block_num() == last_irreversible_block_num );
         if ( prod != account_name() ) {
            BOOST_TEST( state->header.producer == prod );
         }
      }
   };
   // Produces a block and verifies its block_num, LIB, and producer.
   // If the producer is not specified, it must be the same as the previous block
   void produce_and_check( uint32_t block_num, uint32_t last_irreversible_block_num, account_name prod = account_name() ) {
      if ( prod == account_name() )
      {
         prod = control->head_block_producer();
      }
      produce_block();
      check( block_num, last_irreversible_block_num, prod );
   };
   struct block_info {
      uint32_t block_num;
      uint32_t lib;
      account_name producer;
      int32_t confirmed = -1;
   };
   void produce_and_check( std::initializer_list<block_info> expected )
   {
      for ( auto [ block_num, irreversible, prod, _ ] : expected )
      {
         produce_and_check( block_num, irreversible, prod );
      }
   }
   block_timestamp_type get_next_slot( account_name prod )
   {
      auto state = control->head_block_state();
      block_timestamp_type result = state->header.timestamp;
      ++result.slot;
      while( state->get_scheduled_producer( result ).producer_name != prod )
      {
         ++result.slot;
      }
      return result;
   }
   // Skips to the next slot scheduled for a specified producer.
   // If confirmed is not specified, uses the normal default confirmations.
   // If prod is not specified, a block will be produced normally, and
   // the block produced must have the same producer as the previous block.
   void produce_and_check_with_producer( uint32_t block_num, uint32_t last_irreversible_block_num, account_name prod, int32_t confirmed = -1 ) {
      if ( confirmed >= 0 )
      {
         control->abort_block();
         control->start_block( get_next_slot( prod ), confirmed );
      }
      if ( prod != account_name() )
      {
         produce_block( get_next_slot( prod ).to_time_point() - control->head_block_time() );
      }
      else
      {
         prod = control->head_block_producer();
         produce_block();
      }
      check( block_num, last_irreversible_block_num, prod );
   };
   void produce_and_check_with_producer( std::initializer_list<block_info> expected )
   {
      for ( auto [ block_num, irreversible, prod, confirmed ] : expected )
      {
         produce_and_check_with_producer( block_num, irreversible, prod, confirmed );
      }
   }
};

BOOST_FIXTURE_TEST_CASE( two_producers, consensus_tester ) try {
   setup( {N(alice),N(bob)}, 15 );
   produce_and_check({
      { 16, 14, N(bob) },
      { 17, 14 },
      { 18, 14 },
      { 19, 14 },
      { 20, 14 },
      { 21, 14 },
      { 22, 14 },
      { 23, 14 },
      { 24, 14, N(bob) },
      { 25, 15, N(alice) },
      { 26, 15 },
      { 27, 15 },
      { 28, 15 },
      { 29, 15 },
      { 30, 15 },
      { 31, 15 },
      { 32, 15 },
      { 33, 15 },
      { 34, 15 },
      { 35, 15 },
      { 36, 15, N(alice) },
      { 37, 24, N(bob) },
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( two_producers_neither_confirms , consensus_tester ) try {
   setup( {N(alice),N(bob)}, 15 );
   produce_and_check_with_producer({
      { 16, 14, N(bob), 0 },
      { 17, 15, N(alice), 0 },
      { 18, 15, N(bob), 0 },
      { 19, 15, N(alice), 0 },
      { 20, 15, N(bob), 0 },
      { 21, 15, N(alice), 0 }
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( two_producers_one_confirms , consensus_tester ) try {
   setup( {N(alice),N(bob)}, 15 );
   produce_and_check_with_producer({
      { 16, 14, N(bob), 0 },
      { 17, 15, N(alice), 1 },
      { 18, 15, N(bob), 0 },
      { 19, 16, N(alice), 1 },
      { 20, 16, N(bob), 0 },
      { 21, 18, N(alice), 1 }
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( three_producers , consensus_tester ) try {
   setup( {N(alice),N(bob),N(carol)}, 15 );
   produce_and_check({
      { 16, 14, N(bob) },
      { 17, 14 },
      { 18, 14 },
      { 19, 14 },
      { 20, 14 },
      { 21, 14 },
      { 22, 14 },
      { 23, 14 },
      { 24, 14, N(bob) },
      { 25, 14, N(carol) },
      { 26, 14 },
      { 27, 14 },
      { 28, 14 },
      { 29, 14 },
      { 30, 14 },
      { 31, 14 },
      { 32, 14 },
      { 33, 14 },
      { 34, 14 },
      { 35, 14 },
      { 36, 14, N(carol) },
      { 37, 15, N(alice) },
      { 38, 15 },
      { 39, 15 },
      { 40, 15 },
      { 41, 15 },
      { 42, 15 },
      { 43, 15 },
      { 44, 15 },
      { 45, 15 },
      { 46, 15 },
      { 47, 15 },
      { 48, 15, N(alice) },
      { 49, 15, N(bob) },
      { 50, 15 },
      { 51, 15 },
      { 52, 15 },
      { 53, 15 },
      { 54, 15 },
      { 55, 15 },
      { 56, 15 },
      { 57, 15 },
      { 58, 15 },
      { 59, 15 },
      { 60, 15, N(bob) },
      { 61, 24, N(carol) },
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( three_producers_two_active , consensus_tester ) try {
   setup( {N(alice),N(bob),N(carol)}, 15 );
   produce_and_check_with_producer({
      { 16, 14, N(bob) },
      { 17, 14, N(alice) },
      { 18, 14, N(bob) },
      { 19, 14, N(alice) },
      { 20, 14, N(bob) },
      { 21, 14, N(alice) },
      { 22, 14, N(bob) },
      { 23, 14, N(alice) },
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( three_producers_short_confirm , consensus_tester ) try {
   setup( {N(alice),N(bob),N(carol)}, 15 );
   produce_and_check_with_producer({
      { 16, 14, N(bob), 1 },
      { 17, 14, N(carol), 1 },
      { 18, 15, N(alice), 1 },
      { 19, 15, N(bob), 1 },
      { 20, 15, N(carol), 1 },
      { 21, 15, N(alice), 1 },
      { 22, 15, N(bob), 1 },
      { 23, 15, N(carol), 1 },
      { 24, 15, N(alice), 1 },
      { 25, 15, N(bob), 1 },
      { 26, 15, N(carol), 1 },
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( three_producers_one_short_confirm , consensus_tester ) try {
   setup( {N(alice),N(bob),N(carol)}, 15 );
   produce_and_check_with_producer({
      { 16, 14, N(bob) },
      { 17, 14, N(carol), 0 },
      { 18, 15, N(alice) },
      { 19, 15, N(bob) },
      { 20, 15, N(carol), 0 },
      { 21, 15, N(alice) },
      { 22, 17, N(bob) },
      { 23, 17, N(carol), 0 },
      { 24, 17, N(alice) },
      { 25, 20, N(bob) },
      { 26, 20, N(carol), 0 },
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( confirmation_tracking_over_limit, consensus_tester ) try {
   setup( {N(alice),N(bob),N(carol)}, 15 );
   produce_and_check_with_producer({
      { 16, 14, N(bob) },
      { 17, 14, N(carol), 1 },
   });
   uint32_t forgotten_at = config::maximum_tracked_dpos_confirmations + 16;
   for(uint32_t i = 18; i < forgotten_at; ++i)
   {
      produce_and_check_with_producer( i, 14 , N(carol) );
   }
   produce_and_check_with_producer({
      { forgotten_at, 15, N(alice), config::maximum_tracked_dpos_confirmations + 1 },
      { forgotten_at + 1, 15, N(bob), 0 },
      { forgotten_at + 2, 15, N(carol), 0 },
      { forgotten_at + 3, 15, N(alice), 0 },
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( confirmation_tracking_at_limit, consensus_tester ) try {
   setup( {N(alice),N(bob),N(carol)}, 15 );
   produce_and_check_with_producer({
      { 16, 14, N(bob) },
      { 17, 14, N(carol), 1 },
   });
   uint32_t forgotten_at = config::maximum_tracked_dpos_confirmations + 15;
   for(uint32_t i = 18; i < forgotten_at; ++i)
   {
      produce_and_check_with_producer( i, 14 , N(carol) );
   }
   produce_and_check_with_producer({
      { forgotten_at, 15, N(alice), config::maximum_tracked_dpos_confirmations },
      { forgotten_at + 1, 15, N(bob), 0 },
      { forgotten_at + 2, 15, N(carol), 0 },
      { forgotten_at + 3, 16, N(alice), 0 },
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( four_producers, consensus_tester ) try {
   setup( {N(alice),N(bob),N(carol),N(dave)}, 15 );
   produce_and_check_with_producer({
      { 16, 14, N(bob) },
      { 17, 14, N(carol) },
      { 18, 15, N(dave) },
      { 19, 15, N(alice) },
      { 20, 15, N(bob) },
      { 21, 16, N(carol) },
      { 22, 17, N(dave) },
      { 23, 18, N(alice) },
      { 24, 19, N(bob) },
      { 25, 20, N(carol) },
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( stop_confirming, consensus_tester ) try {
   setup( {N(alice),N(bob),N(carol),N(dave)}, 15 );
   produce_and_check_with_producer({
      { 16, 14, N(bob) },
      { 17, 14, N(carol) },
      { 18, 15, N(dave) },
      { 19, 15, N(alice), 0 },
      { 20, 15, N(bob), 0 },
      { 21, 16, N(carol), 0 },
      { 22, 16, N(dave), 0 },
      { 23, 16, N(alice), 0 },
      { 24, 16, N(bob), 0 },
      { 25, 16, N(carol), 0 },
      { 26, 16, N(dave), 0 },
      { 27, 16, N(alice), 0 },
      { 28, 16, N(bob), 0 },
   });
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( confirmations_across_new_producer_schedule, consensus_tester ) try {
   create_accounts( {N(carol),N(dave),N(emily),N(frank)} );
   setup( {N(alice),N(bob)}, 15 );
   produce_and_check({
      { 16, 14, N(bob) },
      { 17, 14 },
      { 18, 14 },
      { 19, 14 },
      { 20, 14 },
      { 21, 14 },
      { 22, 14 },
      { 23, 14 },
      { 24, 14 },
      { 25, 15, N(alice) },
      { 26, 15 },
      { 27, 15 },
   });
   set_producers( {N(carol),N(dave),N(emily),N(frank)} );
   produce_and_check({
      { 28, 15 },
      { 29, 15 },
      { 30, 15 },
      { 31, 15 },
      { 32, 15 },
      { 33, 15 },
      { 34, 15 },
      { 35, 15 },
      { 36, 15 },
      { 37, 24, N(bob) },
      { 38, 24 },
      { 39, 24 },
      { 40, 24 },
      { 41, 24 },
      { 42, 24 },
      { 43, 24 },
      { 44, 24 },
      { 45, 24 },
      { 46, 24 },
      { 47, 24 },
      { 48, 24 },
      { 49, 36, N(alice) }, // schedule 2 becomes pending here
      { 50, 36 },
      { 51, 36 },
      { 52, 36 },
      { 53, 36 },
      { 54, 36 },
      { 55, 36 },
      { 56, 36 },
      { 57, 36 },
      { 58, 36 },
      { 59, 36 },
      { 60, 36 },
      { 61, 48, N(bob) },
      { 62, 48 },
      { 63, 48 },
      { 64, 48 },
      { 65, 48 },
      { 66, 48 },
      { 67, 48 },
      { 68, 48 },
      { 69, 48 },
      { 70, 48 },
      { 71, 48 },
      { 72, 48 },
      { 73, 60, N(alice) },
      { 74, 60, N(emily) },
      { 75, 60 },
      { 76, 60 },
      { 77, 60 },
      { 78, 60 },
      { 79, 60 },
      { 80, 60 },
      { 81, 60 },
      { 82, 60 },
      { 83, 60 },
      { 84, 60 },
      { 85, 60, N(frank) },
      { 86, 60 },
      { 87, 60 },
      { 88, 60 },
      { 89, 60 },
      { 90, 60 },
      { 91, 60 },
      { 92, 60 },
      { 93, 60 },
      { 94, 60 },
      { 95, 60 },
      { 96, 60 },
      { 97, 73, N(carol) },
      { 98, 73 },
   });

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
