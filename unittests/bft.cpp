#include <eosio/chain/block_header_state.hpp>
#include <utility>
#include <algorithm>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(bft)

void dump_state(const eosio::chain::detail::confirmation_group_v1& g)
{
   std::cout << "proposed_irreversible_blocknum: " << g.proposed_irreversible_blocknum << std::endl;
   std::cout << "irreversible_blocknum: " << g.irreversible_blocknum << std::endl;
   std::cout << "confirm1:\n";
   for(auto [prod, range] : g.producer_to_last_confirmed)
   {
      std::cout << "  " << prod.to_string() << ": [" << range.first << "," << range.second << ")\n";
   }
   std::cout << "confirm2:\n";
   for(auto [prod, range] : g.producer_to_second_confirmations)
   {
      std::cout << "  " << prod.to_string() << ": [" << range.first << "," << range.second << ")\n";
   }
}

BOOST_AUTO_TEST_CASE(single_producer)
{
   eosio::chain::detail::confirmation_group_v1 g0;
   g0.set_producers({N(a1)});
   uint32_t head = 0;
   for(uint32_t n : { 10, 1, 1, 1, 1, 4, 6, 1 })
   {
      g0.confirm1(N(a1), {head, head + n});
      head += n;
      BOOST_TEST(g0.proposed_irreversible_blocknum == head - 1);
      BOOST_TEST(g0.irreversible_blocknum == head - 1);
   }
}

BOOST_AUTO_TEST_CASE(single_producer_skip)
{
   eosio::chain::detail::confirmation_group_v1 g0;
   g0.set_producers({N(a1)});
   uint32_t head = 0;
   for(uint32_t n : { 10, 1, 1, 2, 2, 4, 6, 1 })
   {
      g0.confirm1(N(a1), {head + 1, head + n});
      head += n;
      if(n > 1)
      {
         BOOST_TEST(g0.proposed_irreversible_blocknum == head - 1);
         BOOST_TEST(g0.irreversible_blocknum == head - 1);
      }
   }
}

BOOST_AUTO_TEST_CASE(multi_producer_all)
{
   eosio::chain::detail::confirmation_group_v1 g0;
   g0.set_producers({N(p1), N(p2), N(p3), N(p4)});
   g0.confirm1(N(p1), {0, 5});
   g0.confirm1(N(p2), {0, 6});
   g0.confirm1(N(p3), {0, 7});
   g0.confirm1(N(p4), {0, 8});
   BOOST_TEST(g0.proposed_irreversible_blocknum == 5);
   BOOST_TEST(g0.irreversible_blocknum == 0);
   eosio::chain::name producers[] = { N(p4), N(p1), N(p2), N(p3) };
   for(int i = 9; i < 20; ++i)
   {
      g0.confirm1(producers[i%4], {i - 4, i});
      BOOST_TEST(g0.proposed_irreversible_blocknum == i - 3);
      BOOST_TEST(g0.irreversible_blocknum == i - 5);
   }
}

BOOST_AUTO_TEST_CASE(multi_producer_min_quorum)
{
   eosio::chain::detail::confirmation_group_v1 g0;
   g0.set_producers({N(p1), N(p2), N(p3), N(p4)});
   g0.confirm1(N(p1), {0, 5});
   g0.confirm1(N(p2), {0, 6});
   g0.confirm1(N(p3), {0, 7});
   g0.confirm1(N(p1), {5, 8});
   BOOST_TEST(g0.proposed_irreversible_blocknum == 5);
   BOOST_TEST(g0.irreversible_blocknum == 0);
   eosio::chain::name producers[] = { N(p2), N(p3), N(p1) };
   for(int i = 9; i < 20; ++i)
   {
      g0.confirm1(producers[i%3], {i - 3, i});
      BOOST_TEST(g0.proposed_irreversible_blocknum == i - 3);
      BOOST_TEST(g0.irreversible_blocknum == i - 5);
   }
}

BOOST_AUTO_TEST_CASE(multi_producer_below_min)
{
   eosio::chain::detail::confirmation_group_v1 g0;
   g0.set_producers({N(p1), N(p2), N(p3), N(p4)});
   g0.confirm1(N(p1), {0, 5});
   g0.confirm1(N(p2), {0, 6});
   eosio::chain::name producers[] = { N(p2), N(p1) };
   for(int i = 7; i < 20; ++i)
   {
      g0.confirm1(producers[i%2], {i - 2, i});
      BOOST_TEST(g0.proposed_irreversible_blocknum == 0);
      BOOST_TEST(g0.irreversible_blocknum == 0);
   }
}

BOOST_AUTO_TEST_CASE(multi_producer_interference)
{
   eosio::chain::detail::confirmation_group_v1 g0;
   std::vector producers = { N(p11), N(p12), N(p13), N(p14), N(p15), N(p21), N(p22) };
   g0.set_producers(producers);

   auto is_good_producer = [](auto n) { return n != N(p12) && n != N(p14); };
   auto good_confirm = [](eosio::chain::detail::confirmation_group_v1& g0, eosio::chain::name prod, uint32_t head_blocknum)
   {
      auto iter = g0.producer_to_last_confirmed.find(prod);
      g0.confirm1(prod, {iter->second.second, head_blocknum});
   };

   std::size_t current_producer_index = 4;
   uint32_t head_blocknum = 9;
   for(int i = 0; i < 20; ++i)
   {
      auto prod = producers[current_producer_index];
      if(is_good_producer(prod))
      {
         good_confirm(g0, prod, head_blocknum);
      }
      else
      {
         ++head_blocknum;
         g0.confirm1(prod, {head_blocknum - 1, head_blocknum});
      }
      ++head_blocknum;
      current_producer_index = (current_producer_index + 1) % 7;
   }
   BOOST_TEST(g0.irreversible_blocknum == 18);
}

BOOST_AUTO_TEST_SUITE_END()
