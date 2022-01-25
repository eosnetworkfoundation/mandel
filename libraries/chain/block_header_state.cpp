#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/exceptions.hpp>
#include <limits>

namespace eosio { namespace chain {

   namespace detail {
      bool is_builtin_activated( const protocol_feature_activation_set_ptr& pfa,
                                 const protocol_feature_set& pfs,
                                 builtin_protocol_feature_t feature_codename )
      {
         auto digest = pfs.get_builtin_digest(feature_codename);
         const auto& protocol_features = pfa->protocol_features;
         return digest && protocol_features.find(*digest) != protocol_features.end();
      }
   }

   producer_authority block_header_state::get_scheduled_producer( block_timestamp_type t )const {
      auto index = t.slot % (active_schedule.producers.size() * config::producer_repetitions);
      index /= config::producer_repetitions;
      return active_schedule.producers[index];
   }

   namespace detail {

   void confirmation_group_v0::set_producers(const std::vector<producer_authority>& producers)
   {
      flat_map<account_name,uint32_t> new_producer_to_last_produced;

      for( const auto& pro : producers ) {
            auto existing = producer_to_last_produced.find( pro.producer_name );
            if( existing != producer_to_last_produced.end() ) {
               new_producer_to_last_produced[pro.producer_name] = existing->second;
            } else {
               new_producer_to_last_produced[pro.producer_name] = dpos_irreversible_blocknum;
            }
      }
      // This is really annoying, because it should be redundant,
      // except for the fact that it can carry over to the next producer schedule
      // Conjecture:
      // If the new producer schedule does not include the producer who created the block that activated it,
      // but the next producer schedule does include this producer, it will be allowed to confirm
      // blocks farther back than any other producer that gets switched in.
      // TODO: test whether this is actually observable
      auto last_producer = std::find_if(producer_to_last_produced.begin(), producer_to_last_produced.end(),
                                        [=](const auto& arg){ return arg.second == block_num; });
      new_producer_to_last_produced[last_producer->first] = block_num;

      producer_to_last_produced = std::move( new_producer_to_last_produced );

      flat_map<account_name,uint32_t> new_producer_to_last_implied_irb;

      for( const auto& pro : producers ) {
         auto existing = producer_to_last_implied_irb.find( pro.producer_name );
         if( existing != producer_to_last_implied_irb.end() ) {
            new_producer_to_last_implied_irb[pro.producer_name] = existing->second;
         } else {
            new_producer_to_last_implied_irb[pro.producer_name] = dpos_irreversible_blocknum;
         }
      }

      producer_to_last_implied_irb = std::move( new_producer_to_last_implied_irb );
   }

   uint32_t confirmation_group_v0::calc_dpos_last_irreversible( account_name producer_of_next_block )const {
      vector<uint32_t> blocknums; blocknums.reserve( producer_to_last_implied_irb.size() );
      for( auto& i : producer_to_last_implied_irb ) {
         blocknums.push_back( (i.first == producer_of_next_block) ? dpos_proposed_irreversible_blocknum : i.second);
      }
      /// 2/3 must be greater, so if I go 1/3 into the list sorted from low to high, then 2/3 are greater

      if( blocknums.size() == 0 ) return 0;

      std::size_t index = (blocknums.size()-1) / 3;
      std::nth_element( blocknums.begin(),  blocknums.begin() + index, blocknums.end() );
      return blocknums[ index ];
   }

   std::size_t confirmation_group_v0::num_active_producers() const
   {
      if(producer_to_last_implied_irb.empty())
      {
         return 1;
      }
      else
      {
         return producer_to_last_implied_irb.size();
      }
   }

   confirmation_group_v0 confirmation_group_v0::confirm(account_name producer, std::pair<uint32_t, uint32_t> confirmed_range) const
   {
      confirmation_group_v0 result;

      result.block_num = confirmed_range.second - 1;

      /// grow the confirmed count
      static_assert(std::numeric_limits<uint8_t>::max() >= (config::max_producers * 2 / 3) + 1, "8bit confirmations may not be able to hold all of the needed confirmations");

      // This uses the previous block active_schedule because thats the "schedule" that signs and therefore confirms _this_ block
      uint32_t required_confs = (uint32_t)(num_active_producers() * 2 / 3) + 1;

      auto itr = producer_to_last_produced.find( producer );
      if( itr != producer_to_last_produced.end() ) {
        EOS_ASSERT( itr->second < confirmed_range.first, producer_double_confirm,
                    "producer ${prod} double-confirming known range",
                    ("prod", producer)("num", block_num+1)
                    ("confirmed", confirmed_range.second - confirmed_range.first - 1)
                    ("last_produced", itr->second) );
      }

      if( confirm_count.size() < config::maximum_tracked_dpos_confirmations ) {
         result.confirm_count.reserve( confirm_count.size() + 1 );
         result.confirm_count  = confirm_count;
         result.confirm_count.resize( confirm_count.size() + 1 );
         result.confirm_count.back() = (uint8_t)required_confs;
      } else {
         result.confirm_count.resize( confirm_count.size() );
         memcpy( &result.confirm_count[0], &confirm_count[1], confirm_count.size() - 1 );
         result.confirm_count.back() = (uint8_t)required_confs;
      }

      auto new_dpos_proposed_irreversible_blocknum = dpos_proposed_irreversible_blocknum;

      int32_t i = (int32_t)(result.confirm_count.size() - 1);
      uint32_t blocks_to_confirm = confirmed_range.second - confirmed_range.first; /// confirm the head block too
      while( i >= 0 && blocks_to_confirm ) {
        --result.confirm_count[i];
        //idump((confirm_count[i]));
        if( result.confirm_count[i] == 0 )
        {
            uint32_t block_num_for_i = result.block_num - (uint32_t)(result.confirm_count.size() - 1 - i);
            new_dpos_proposed_irreversible_blocknum = block_num_for_i;
            //idump((dpos2_lib)(block_num)(dpos_irreversible_blocknum));

            if (i == static_cast<int32_t>(result.confirm_count.size() - 1)) {
               result.confirm_count.resize(0);
            } else {
               memmove( &result.confirm_count[0], &result.confirm_count[i + 1], result.confirm_count.size() - i  - 1);
               result.confirm_count.resize( result.confirm_count.size() - i - 1 );
            }

            break;
        }
        --i;
        --blocks_to_confirm;
      }

      result.dpos_proposed_irreversible_blocknum   = new_dpos_proposed_irreversible_blocknum;
      result.dpos_irreversible_blocknum            = calc_dpos_last_irreversible( producer );

      result.producer_to_last_produced        = producer_to_last_produced;
      result.producer_to_last_produced[producer] = result.block_num;
      result.producer_to_last_implied_irb     = producer_to_last_implied_irb;
      result.producer_to_last_implied_irb[producer] = dpos_proposed_irreversible_blocknum;

      return result;
   }

   void confirmation_group_v1::set_producers(const std::vector<producer_authority>& producers)
   {
      producer_to_last_confirmed.clear();
      producer_to_last_confirmed.reserve(producers.size());
      producer_to_second_confirmations.clear();
      for(const producer_authority& prod : producers)
      {
         producer_to_last_confirmed.insert(std::pair{prod.producer_name, std::pair<uint32_t, uint32_t>{0,0}});
      }
   }

   void confirmation_group_v1::set_producers(const std::vector<account_name>& producers)
   {
      producer_to_last_confirmed.clear();
      producer_to_last_confirmed.reserve(producers.size());
      producer_to_second_confirmations.clear();
      for(account_name prod : producers)
      {
         producer_to_last_confirmed.insert(std::pair{prod, std::pair<uint32_t, uint32_t>{0,0}});
      }
   }

   // Returns the highest range that is contained by at least threshold input ranges
   static std::pair<uint32_t, uint32_t> threshold_intersection(const flat_map<account_name, std::pair<uint32_t, uint32_t>>& inputs, std::size_t threshold)
   {
      std::vector<std::pair<uint32_t, int8_t>> counters;
      counters.reserve(inputs.size() * 2);
      for(const auto& [account, range] : inputs)
      {
         counters.push_back({range.first, 1});
         counters.push_back({range.second, -1});
      }
      std::sort(counters.begin(), counters.end(), std::greater<>());
      uint32_t start = 0;
      uint32_t prev = 0;
      uint32_t total = 0;
      uint32_t prev_total = 0;
      for(auto [x, direction] : counters)
      {
         if(x != prev)
         {
            if(prev_total >= threshold && total < threshold)
            {
               break;
            }
            else if(prev_total < threshold && total >= threshold)
            {
               start = prev;
            }
            prev_total = total;
            prev = x;
         }
         total -= direction;
      }
      return {prev, start};
   }

   confirmation_group_v1 confirmation_group_v1::confirm(account_name producer, std::pair<uint32_t, uint32_t> confirmed_range) const
   {
      confirmation_group_v1 result = *this;
      if(confirmed_range.first == confirmed_range.second)
      {
         return result;
      }
      if(producer_to_last_confirmed.empty())
      {
         result.set_producers({N(eosio)});
      }

      uint32_t threshold = result.producer_to_last_confirmed.size()*2/3 + 1;

      // Update last confirmed
      auto iter = result.producer_to_last_confirmed.find(producer);
      EOS_ASSERT( iter != result.producer_to_last_confirmed.end() &&
                  iter->second.second <= confirmed_range.first &&
                  confirmed_range.first <= confirmed_range.second, block_validate_exception, "Illegal confirmation" );

      if(iter->second.second == confirmed_range.first)
      {
         iter->second.second = confirmed_range.second;
      }
      else
      {
         iter->second = confirmed_range;
      }

      auto second_confirmations = threshold_intersection(result.producer_to_last_confirmed, threshold);
      second_confirmations.first = iter->second.first;
      if(second_confirmations.first < second_confirmations.second)
      {
         result.proposed_irreversible_blocknum = std::max(proposed_irreversible_blocknum, second_confirmations.second - 1);

         result.producer_to_second_confirmations[producer] = second_confirmations;
         auto [low, high] = threshold_intersection(result.producer_to_second_confirmations, threshold);
         if(low < high)
         {
            result.irreversible_blocknum = std::max(irreversible_blocknum, high - 1);
         }
      }
      return result;
   }

   }

   pending_block_header_state  block_header_state::next( block_timestamp_type when,
                                                         uint16_t num_prev_blocks_to_confirm )const
   {
      pending_block_header_state result;

      if( when != block_timestamp_type() ) {
        EOS_ASSERT( when > header.timestamp, block_validate_exception, "next block must be in the future" );
      } else {
        (when = header.timestamp).slot++;
      }

      auto proauth = get_scheduled_producer(when);

      result.block_num                                       = block_num + 1;
      result.previous                                        = id;
      result.timestamp                                       = when;
      result.confirmed                                       = num_prev_blocks_to_confirm;
      result.active_schedule_version                         = active_schedule.version;
      result.prev_activated_protocol_features                = activated_protocol_features;

      result.valid_block_signing_authority                   = proauth.authority;
      result.producer                                        = proauth.producer_name;

      result.blockroot_merkle = blockroot_merkle;
      result.blockroot_merkle.append( id );

      confirmations.visit([&](const auto& c){ result.confirmations = c.confirm( proauth.producer_name, { result.block_num - num_prev_blocks_to_confirm, result.block_num + 1 } ); });

      result.prev_pending_schedule                 = pending_schedule;

      if( pending_schedule.schedule.producers.size() &&
          result.last_irreversible_block_num() >= pending_schedule.schedule_lib_num )
      {
         result.active_schedule = pending_schedule.schedule;

         result.confirmations.visit([&](auto& c){ c.set_producers( result.active_schedule.producers ); });

         result.was_pending_promoted = true;
      } else {
         result.active_schedule                  = active_schedule;
      }

      return result;
   }

   signed_block_header pending_block_header_state::make_block_header(
                                                      const checksum256_type& transaction_mroot,
                                                      const checksum256_type& action_mroot,
                                                      const optional<producer_authority_schedule>& new_producers,
                                                      vector<digest_type>&& new_protocol_feature_activations,
                                                      const protocol_feature_set& pfs
   )const
   {
      signed_block_header h;

      h.timestamp         = timestamp;
      h.producer          = producer;
      h.confirmed         = confirmed;
      h.previous          = previous;
      h.transaction_mroot = transaction_mroot;
      h.action_mroot      = action_mroot;
      h.schedule_version  = active_schedule_version;

      if( new_protocol_feature_activations.size() > 0 ) {
         emplace_extension(
               h.header_extensions,
               protocol_feature_activation::extension_id(),
               fc::raw::pack( protocol_feature_activation{ std::move(new_protocol_feature_activations) } )
         );
      }

      if (new_producers) {
         if ( detail::is_builtin_activated(prev_activated_protocol_features, pfs, builtin_protocol_feature_t::wtmsig_block_signatures) ) {
            // add the header extension to update the block schedule
            emplace_extension(
                  h.header_extensions,
                  producer_schedule_change_extension::extension_id(),
                  fc::raw::pack( producer_schedule_change_extension( *new_producers ) )
            );
         } else {
            legacy::producer_schedule_type downgraded_producers;
            downgraded_producers.version = new_producers->version;
            for (const auto &p : new_producers->producers) {
               p.authority.visit([&downgraded_producers, &p](const auto& auth){
                  EOS_ASSERT(auth.keys.size() == 1 && auth.keys.front().weight == auth.threshold, producer_schedule_exception, "multisig block signing present before enabled!");
                  downgraded_producers.producers.emplace_back(legacy::producer_key{p.producer_name, auth.keys.front().key});
               });
            }
            h.new_producers = std::move(downgraded_producers);
         }
      }

      return h;
   }

   block_header_state pending_block_header_state::_finish_next(
                                 const signed_block_header& h,
                                 const protocol_feature_set& pfs,
                                 const std::function<void( block_timestamp_type,
                                                           const flat_set<digest_type>&,
                                                           const vector<digest_type>& )>& validator

   )&&
   {
      EOS_ASSERT( h.timestamp == timestamp, block_validate_exception, "timestamp mismatch" );
      EOS_ASSERT( h.previous == previous, unlinkable_block_exception, "previous mismatch" );
      EOS_ASSERT( h.confirmed == confirmed, block_validate_exception, "confirmed mismatch" );
      EOS_ASSERT( h.producer == producer, wrong_producer, "wrong producer specified" );
      EOS_ASSERT( h.schedule_version == active_schedule_version, producer_schedule_exception, "schedule_version in signed block is corrupted" );

      auto exts = h.validate_and_extract_header_extensions();

      std::optional<producer_authority_schedule> maybe_new_producer_schedule;
      std::optional<digest_type> maybe_new_producer_schedule_hash;
      bool wtmsig_enabled = false;

      if (h.new_producers || exts.count(producer_schedule_change_extension::extension_id()) > 0 ) {
         wtmsig_enabled = detail::is_builtin_activated(prev_activated_protocol_features, pfs, builtin_protocol_feature_t::wtmsig_block_signatures);
      }

      if( h.new_producers ) {
         EOS_ASSERT(!wtmsig_enabled, producer_schedule_exception, "Block header contains legacy producer schedule outdated by activation of WTMsig Block Signatures" );

         EOS_ASSERT( !was_pending_promoted, producer_schedule_exception, "cannot set pending producer schedule in the same block in which pending was promoted to active" );

         const auto& new_producers = *h.new_producers;
         EOS_ASSERT( new_producers.version == active_schedule.version + 1, producer_schedule_exception, "wrong producer schedule version specified" );
         EOS_ASSERT( prev_pending_schedule.schedule.producers.empty(), producer_schedule_exception,
                    "cannot set new pending producers until last pending is confirmed" );

         maybe_new_producer_schedule_hash.emplace(digest_type::hash(new_producers));
         maybe_new_producer_schedule.emplace(new_producers);
      }

      if ( exts.count(producer_schedule_change_extension::extension_id()) > 0 ) {
         EOS_ASSERT(wtmsig_enabled, producer_schedule_exception, "Block header producer_schedule_change_extension before activation of WTMsig Block Signatures" );
         EOS_ASSERT( !was_pending_promoted, producer_schedule_exception, "cannot set pending producer schedule in the same block in which pending was promoted to active" );

         const auto& new_producer_schedule = exts.lower_bound(producer_schedule_change_extension::extension_id())->second.get<producer_schedule_change_extension>();

         EOS_ASSERT( new_producer_schedule.version == active_schedule.version + 1, producer_schedule_exception, "wrong producer schedule version specified" );
         EOS_ASSERT( prev_pending_schedule.schedule.producers.empty(), producer_schedule_exception,
                     "cannot set new pending producers until last pending is confirmed" );

         maybe_new_producer_schedule_hash.emplace(digest_type::hash(new_producer_schedule));
         maybe_new_producer_schedule.emplace(new_producer_schedule);
      }

      protocol_feature_activation_set_ptr new_activated_protocol_features;
      { // handle protocol_feature_activation
         if( exts.count(protocol_feature_activation::extension_id() > 0) ) {
            const auto& new_protocol_features = exts.lower_bound(protocol_feature_activation::extension_id())->second.get<protocol_feature_activation>().protocol_features;
            validator( timestamp, prev_activated_protocol_features->protocol_features, new_protocol_features );

            new_activated_protocol_features =   std::make_shared<protocol_feature_activation_set>(
                                                   *prev_activated_protocol_features,
                                                   new_protocol_features
                                                );
         } else {
            new_activated_protocol_features = std::move( prev_activated_protocol_features );
         }
      }

      auto block_number = block_num;

      block_header_state result( std::move( *static_cast<detail::block_header_state_common*>(this) ) );

      result.id      = h.id();
      result.header  = h;

      result.header_exts = std::move(exts);

      if( maybe_new_producer_schedule ) {
         result.pending_schedule.schedule = std::move(*maybe_new_producer_schedule);
         result.pending_schedule.schedule_hash = std::move(*maybe_new_producer_schedule_hash);
         result.pending_schedule.schedule_lib_num    = block_number;
      } else {
         if( was_pending_promoted ) {
            result.pending_schedule.schedule.version = prev_pending_schedule.schedule.version;
         } else {
            result.pending_schedule.schedule         = std::move( prev_pending_schedule.schedule );
         }
         result.pending_schedule.schedule_hash       = std::move( prev_pending_schedule.schedule_hash );
         result.pending_schedule.schedule_lib_num    = prev_pending_schedule.schedule_lib_num;
      }

      result.activated_protocol_features = std::move( new_activated_protocol_features );

      return result;
   }

   block_header_state pending_block_header_state::finish_next(
                                 const signed_block_header& h,
                                 vector<signature_type>&& additional_signatures,
                                 const protocol_feature_set& pfs,
                                 const std::function<void( block_timestamp_type,
                                                           const flat_set<digest_type>&,
                                                           const vector<digest_type>& )>& validator,
                                 bool skip_validate_signee
   )&&
   {
      if( !additional_signatures.empty() ) {
         bool wtmsig_enabled = detail::is_builtin_activated(prev_activated_protocol_features, pfs, builtin_protocol_feature_t::wtmsig_block_signatures);

         EOS_ASSERT(wtmsig_enabled, producer_schedule_exception, "Block contains multiple signatures before WTMsig block signatures are enabled" );
      }

      auto result = std::move(*this)._finish_next( h, pfs, validator );

      if( !additional_signatures.empty() ) {
         result.additional_signatures = std::move(additional_signatures);
      }

      // ASSUMPTION FROM controller_impl::apply_block = all untrusted blocks will have their signatures pre-validated here
      if( !skip_validate_signee ) {
        result.verify_signee( );
      }

      return result;
   }

   block_header_state pending_block_header_state::finish_next(
                                 signed_block_header& h,
                                 const protocol_feature_set& pfs,
                                 const std::function<void( block_timestamp_type,
                                                           const flat_set<digest_type>&,
                                                           const vector<digest_type>& )>& validator,
                                 const signer_callback_type& signer
   )&&
   {
      auto pfa = prev_activated_protocol_features;

      auto result = std::move(*this)._finish_next( h, pfs, validator );
      result.sign( signer );
      h.producer_signature = result.header.producer_signature;

      if( !result.additional_signatures.empty() ) {
         bool wtmsig_enabled = detail::is_builtin_activated(pfa, pfs, builtin_protocol_feature_t::wtmsig_block_signatures);
         EOS_ASSERT(wtmsig_enabled, producer_schedule_exception, "Block was signed with multiple signatures before WTMsig block signatures are enabled" );
      }

      return result;
   }

   /**
    *  Transitions the current header state into the next header state given the supplied signed block header.
    *
    *  Given a signed block header, generate the expected template based upon the header time,
    *  then validate that the provided header matches the template.
    *
    *  If the header specifies new_producers then apply them accordingly.
    */
   block_header_state block_header_state::next(
                        const signed_block_header& h,
                        vector<signature_type>&& _additional_signatures,
                        const protocol_feature_set& pfs,
                        const std::function<void( block_timestamp_type,
                                                  const flat_set<digest_type>&,
                                                  const vector<digest_type>& )>& validator,
                        bool skip_validate_signee )const
   {
      return next( h.timestamp, h.confirmed ).finish_next( h, std::move(_additional_signatures), pfs, validator, skip_validate_signee );
   }

   digest_type   block_header_state::sig_digest()const {
      auto header_bmroot = digest_type::hash( std::make_pair( header.digest(), blockroot_merkle.get_root() ) );
      return digest_type::hash( std::make_pair(header_bmroot, pending_schedule.schedule_hash) );
   }

   void block_header_state::sign( const signer_callback_type& signer ) {
      auto d = sig_digest();
      auto sigs = signer( d );

      EOS_ASSERT(!sigs.empty(), no_block_signatures, "Signer returned no signatures");
      header.producer_signature = sigs.back();
      sigs.pop_back();

      additional_signatures = std::move(sigs);

      verify_signee();
   }

   void block_header_state::verify_signee( )const {

      size_t num_keys_in_authority = valid_block_signing_authority.visit([](const auto &a){ return a.keys.size(); });
      EOS_ASSERT(1 + additional_signatures.size() <= num_keys_in_authority, wrong_signing_key,
                 "number of block signatures (${num_block_signatures}) exceeds number of keys in block signing authority (${num_keys})",
                 ("num_block_signatures", 1 + additional_signatures.size())
                 ("num_keys", num_keys_in_authority)
                 ("authority", valid_block_signing_authority)
      );

      std::set<public_key_type> keys;
      auto digest = sig_digest();
      keys.emplace(fc::crypto::public_key( header.producer_signature, digest, true ));

      for (const auto& s: additional_signatures) {
         auto res = keys.emplace(s, digest, true);
         EOS_ASSERT(res.second, wrong_signing_key, "block signed by same key twice", ("key", *res.first));
      }

      bool is_satisfied = false;
      size_t relevant_sig_count = 0;

      std::tie(is_satisfied, relevant_sig_count) = producer_authority::keys_satisfy_and_relevant(keys, valid_block_signing_authority);

      EOS_ASSERT(relevant_sig_count == keys.size(), wrong_signing_key,
                 "block signed by unexpected key",
                 ("signing_keys", keys)("authority", valid_block_signing_authority));

      EOS_ASSERT(is_satisfied, wrong_signing_key,
                 "block signatures do not satisfy the block signing authority",
                 ("signing_keys", keys)("authority", valid_block_signing_authority));
   }

   /**
    *  Reference cannot outlive *this. Assumes header_exts is not mutated after instatiation.
    */
   const vector<digest_type>& block_header_state::get_new_protocol_feature_activations()const {
      static const vector<digest_type> no_activations{};

      if( header_exts.count(protocol_feature_activation::extension_id()) == 0 )
         return no_activations;

      return header_exts.lower_bound(protocol_feature_activation::extension_id())->second.get<protocol_feature_activation>().protocol_features;
   }

   block_header_state::block_header_state( legacy::snapshot_block_header_state_v2&& snapshot )
   {
      auto& confirmations0 = confirmations.get<detail::confirmation_group_v0>();
      confirmations0.block_num = snapshot.block_num;
      block_num                             = snapshot.block_num;
      confirmations0.dpos_proposed_irreversible_blocknum   = snapshot.dpos_proposed_irreversible_blocknum;
      confirmations0.dpos_irreversible_blocknum            = snapshot.dpos_irreversible_blocknum;
      active_schedule                       = producer_authority_schedule( snapshot.active_schedule );
      blockroot_merkle                      = std::move(snapshot.blockroot_merkle);
      confirmations0.producer_to_last_produced             = std::move(snapshot.producer_to_last_produced);
      confirmations0.producer_to_last_implied_irb          = std::move(snapshot.producer_to_last_implied_irb);
      valid_block_signing_authority         = block_signing_authority_v0{ 1, {{std::move(snapshot.block_signing_key), 1}} };
      confirmations0.confirm_count                         = std::move(snapshot.confirm_count);
      id                                    = std::move(snapshot.id);
      header                                = std::move(snapshot.header);
      pending_schedule.schedule_lib_num     = snapshot.pending_schedule.schedule_lib_num;
      pending_schedule.schedule_hash        = std::move(snapshot.pending_schedule.schedule_hash);
      pending_schedule.schedule             = producer_authority_schedule( snapshot.pending_schedule.schedule );
      activated_protocol_features           = std::move(snapshot.activated_protocol_features);
   }


} } /// namespace eosio::chain
