#include <eosio/chain/protocol_feature_manager.hpp>
#include <eosio/chain/protocol_state_object.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/deep_mind.hpp>

#include <fc/scoped_exit.hpp>

#include <algorithm>
#include <boost/assign/list_of.hpp>

namespace eosio { namespace chain {

   const std::unordered_map<builtin_protocol_feature_t, builtin_protocol_feature_spec, enum_hash<builtin_protocol_feature_t>>
   builtin_protocol_feature_codenames =
      boost::assign::map_list_of<builtin_protocol_feature_t, builtin_protocol_feature_spec>
         (  builtin_protocol_feature_t::preactivate_feature, builtin_protocol_feature_spec{
            "PREACTIVATE_FEATURE",
            fc::variant("64fe7df32e9b86be2b296b3f81dfd527f84e82b98e363bc97e40bc7a83733310").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: PREACTIVATE_FEATURE

Adds privileged intrinsic to enable a contract to pre-activate a protocol feature specified by its digest.
Pre-activated protocol features must be activated in the next block.
*/
            {},
            {time_point{}, false, true} // enabled without preactivation and ready to go at any time
         } )
         (  builtin_protocol_feature_t::evm_precompiles, builtin_protocol_feature_spec{
            "EVM_PRECOMPILES",
            fc::variant("7fc04d1e925fd57bfe584b0146186560b8e73371af475978196698f2d26cf0a2").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: EVM_PRECOMPILES

Adds new host functions to support the EVM runtime
- Big integer modular exponentiation (mod_exp)
- Add, multiply, and pairing check functions for the alt_bn128 elliptic curve. (alt_bn128_add, alt_bn128_mul, alt_bn128_pair)
- BLAKE2b F compression function (blake2_f)
- sha3 hash function (with Keccak256 support)
- ecrecover (safe ECDSA uncompressed pubkey recover)
*/
            {}
         } )
         (  builtin_protocol_feature_t::only_link_to_existing_permission, builtin_protocol_feature_spec{
            "ONLY_LINK_TO_EXISTING_PERMISSION",
            fc::variant("f3c3d91c4603cde2397268bfed4e662465293aab10cd9416db0d442b8cec2949").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: ONLY_LINK_TO_EXISTING_PERMISSION

Disallows linking an action to a non-existing permission.
*/
            {}
         } )
         (  builtin_protocol_feature_t::replace_deferred, builtin_protocol_feature_spec{
            "REPLACE_DEFERRED",
            fc::variant("9908b3f8413c8474ab2a6be149d3f4f6d0421d37886033f27d4759c47a26d944").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: REPLACE_DEFERRED

Fix the problems associated with replacing an existing deferred transaction.
Also corrects the RAM usage of accounts affected by the replace deferred transaction bug.
*/
            {}
         } )
         (  builtin_protocol_feature_t::no_duplicate_deferred_id, builtin_protocol_feature_spec{
            "NO_DUPLICATE_DEFERRED_ID",
            fc::variant("45967387ee92da70171efd9fefd1ca8061b5efe6f124d269cd2468b47f1575a0").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: NO_DUPLICATE_DEFERRED_ID
Depends on: REPLACE_DEFERRED

Ensures transactions generated by contracts for deferred execution are adjusted to avoid transaction ID conflicts.
Also allows a contract to send a deferred transaction in a manner that enables the contract to know the transaction ID ahead of time.
*/
            {builtin_protocol_feature_t::replace_deferred}
         } )
         (  builtin_protocol_feature_t::fix_linkauth_restriction, builtin_protocol_feature_spec{
            "FIX_LINKAUTH_RESTRICTION",
            fc::variant("a98241c83511dc86c857221b9372b4aa7cea3aaebc567a48604e1d3db3557050").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: FIX_LINKAUTH_RESTRICTION

Removes the restriction on eosio::linkauth for non-native actions named one of the five special action names:
updateauth, deleteauth, linkauth, unlinkauth, or canceldelay.
*/
            {}
         } )
         (  builtin_protocol_feature_t::disallow_empty_producer_schedule, builtin_protocol_feature_spec{
            "DISALLOW_EMPTY_PRODUCER_SCHEDULE",
            fc::variant("2853617cec3eabd41881eb48882e6fc5e81a0db917d375057864b3befbe29acd").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: DISALLOW_EMPTY_PRODUCER_SCHEDULE

Disallows proposing an empty producer schedule.
*/
            {}
         } )
         (  builtin_protocol_feature_t::restrict_action_to_self, builtin_protocol_feature_spec{
            "RESTRICT_ACTION_TO_SELF",
            fc::variant("e71b6712188391994c78d8c722c1d42c477cf091e5601b5cf1befd05721a57f3").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: RESTRICT_ACTION_TO_SELF

Disallows bypass of authorization checks by unprivileged contracts when sending inline actions or deferred transactions.
The original protocol rules allow a bypass of authorization checks for actions sent by a contract to itself.
This protocol feature removes that bypass.
*/
            {}
         } )
         (  builtin_protocol_feature_t::only_bill_first_authorizer, builtin_protocol_feature_spec{
            "ONLY_BILL_FIRST_AUTHORIZER",
            fc::variant("2f1f13e291c79da5a2bbad259ed7c1f2d34f697ea460b14b565ac33b063b73e2").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: ONLY_BILL_FIRST_AUTHORIZER

Adds CPU and network bandwidth usage to only the first authorizer of a transaction.
*/
            {}
         } )
         (  builtin_protocol_feature_t::forward_setcode, builtin_protocol_feature_spec{
            "FORWARD_SETCODE",
            fc::variant("898082c59f921d0042e581f00a59d5ceb8be6f1d9c7a45b6f07c0e26eaee0222").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: FORWARD_SETCODE

Forward eosio::setcode actions to the WebAssembly code deployed on the eosio account.
*/
            {}
         } )
         (  builtin_protocol_feature_t::get_sender, builtin_protocol_feature_spec{
            "GET_SENDER",
            fc::variant("1eab748b95a2e6f4d7cb42065bdee5566af8efddf01a55a0a8d831b823f8828a").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: GET_SENDER

Allows contracts to determine which account is the sender of an inline action.
*/
            {}
         } )
         (  builtin_protocol_feature_t::ram_restrictions, builtin_protocol_feature_spec{
            "RAM_RESTRICTIONS",
            fc::variant("1812fdb5096fd854a4958eb9d53b43219d114de0e858ce00255bd46569ad2c68").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: RAM_RESTRICTIONS

Modifies the restrictions on operations within actions that increase RAM usage of accounts other than the receiver.

An unprivileged contract responding to a notification:
is not allowed to schedule a deferred transaction in which the RAM costs are paid by an account other than the receiver;
but is allowed to execute database operations that increase RAM usage of an account other than the receiver as long as
the action's net effect on RAM usage for the account is to not increase it.

An unprivileged contract executing an action (but not as a response to a notification):
is not allowed to schedule a deferred transaction in which the RAM costs are paid by an account other than the receiver
unless that account authorized the action;
but is allowed to execute database operations that increase RAM usage of an account other than the receiver as long as
either the account authorized the action or the action's net effect on RAM usage for the account is to not increase it.
*/
            {}
         } )
         (  builtin_protocol_feature_t::webauthn_key, builtin_protocol_feature_spec{
            "WEBAUTHN_KEY",
            fc::variant("927fdf78c51e77a899f2db938249fb1f8bb38f4e43d9c1f75b190492080cbc34").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: WEBAUTHN_KEY

Enables usage of WebAuthn keys and signatures.
*/
            {}
         } )
         (  builtin_protocol_feature_t::wtmsig_block_signatures, builtin_protocol_feature_spec{
            "WTMSIG_BLOCK_SIGNATURES",
            fc::variant("ab76031cad7a457f4fd5f5fca97a3f03b8a635278e0416f77dcc91eb99a48e10").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: WTMSIG_BLOCK_SIGNATURES

Allows producers to specify a multisig of weighted keys as the authority for signing blocks.

A valid block header:
is no longer allowed to have a non-empty `new_producers` field;
must announce new producer schedules using a block header extension with ID `1`.

A valid signed block:
must continue to have exactly one signature in its `signatures` field;
and may have additional signatures in a block extension with ID `2`.

Privileged Contracts:
may continue to use `set_proposed_producers` as they have;
may use a new `set_proposed_producers_ex` intrinsic to access extended features.
*/
            {}
         } )
         (  builtin_protocol_feature_t::action_return_value, builtin_protocol_feature_spec{
            "ACTION_RETURN_VALUE",
            fc::variant("69b064c5178e2738e144ed6caa9349a3995370d78db29e494b3126ebd9111966").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: ACTION_RETURN_VALUE

Enables new `set_action_return_value` intrinsic which sets a value that is included in action_receipt.
*/
            {}
         } )
         (  builtin_protocol_feature_t::configurable_wasm_limits, builtin_protocol_feature_spec{
            "CONFIGURABLE_WASM_LIMITS2",
            fc::variant("8139e99247b87f18ef7eae99f07f00ea3adf39ed53f4d2da3f44e6aa0bfd7c62").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: CONFIGURABLE_WASM_LIMITS2

Allows privileged contracts to set the constraints on WebAssembly code.
Includes the behavior of GET_WASM_PARAMETERS_PACKED_FIX and
also removes an inadvertent restriction on custom sections.
*/
            {}
         } )
         (  builtin_protocol_feature_t::blockchain_parameters, builtin_protocol_feature_spec{
            "BLOCKCHAIN_PARAMETERS",
            fc::variant("70787548dcea1a2c52c913a37f74ce99e6caae79110d7ca7b859936a0075b314").as<digest_type>(),
            {}
         } )
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: BLOCKCHAIN_PARAMETERS

Allows privileged contracts to get and set subsets of blockchain parameters.
*/
         (  builtin_protocol_feature_t::get_code_hash, builtin_protocol_feature_spec{
            "GET_CODE_HASH",
            fc::variant("d2596697fed14a0840013647b99045022ae6a885089f35a7e78da7a43ad76ed4").as<digest_type>(),
            // SHA256 hash of the raw message below within the comment delimiters (do not modify message below).
/*
Builtin protocol feature: GET_CODE_HASH

Enables new `get_code_hash` intrinsic which gets the current code hash of an account.
*/
            {}
         } )
   ;


   const char* builtin_protocol_feature_codename( builtin_protocol_feature_t codename ) {
      auto itr = builtin_protocol_feature_codenames.find( codename );
      EOS_ASSERT( itr != builtin_protocol_feature_codenames.end(), protocol_feature_validation_exception,
                  "Unsupported builtin_protocol_feature_t passed to builtin_protocol_feature_codename: ${codename}",
                  ("codename", static_cast<uint32_t>(codename)) );

      return itr->second.codename;
   }

   protocol_feature_base::protocol_feature_base( protocol_feature_t feature_type,
                                                 const digest_type& description_digest,
                                                 flat_set<digest_type>&& dependencies,
                                                 const protocol_feature_subjective_restrictions& restrictions )
   :description_digest( description_digest )
   ,dependencies( std::move(dependencies) )
   ,subjective_restrictions( restrictions )
   ,_type( feature_type )
   {
      switch( feature_type ) {
         case protocol_feature_t::builtin:
            protocol_feature_type = builtin_protocol_feature::feature_type_string;
         break;
         default:
         {
            EOS_THROW( protocol_feature_validation_exception,
                       "Unsupported protocol_feature_t passed to constructor: ${type}",
                       ("type", static_cast<uint32_t>(feature_type)) );
         }
         break;
      }
   }

   void protocol_feature_base::reflector_init() {
      static_assert( fc::raw::has_feature_reflector_init_on_unpacked_reflected_types,
                     "protocol_feature_activation expects FC to support reflector_init" );

      if( protocol_feature_type == builtin_protocol_feature::feature_type_string ) {
         _type = protocol_feature_t::builtin;
      } else {
         EOS_THROW( protocol_feature_validation_exception,
                    "Unsupported protocol feature type: ${type}", ("type", protocol_feature_type) );
      }
   }

   const char* builtin_protocol_feature::feature_type_string = "builtin";

   builtin_protocol_feature::builtin_protocol_feature( builtin_protocol_feature_t codename,
                                                       const digest_type& description_digest,
                                                       flat_set<digest_type>&& dependencies,
                                                       const protocol_feature_subjective_restrictions& restrictions )
   :protocol_feature_base( protocol_feature_t::builtin, description_digest, std::move(dependencies), restrictions )
   ,_codename(codename)
   {
      auto itr = builtin_protocol_feature_codenames.find( codename );
      EOS_ASSERT( itr != builtin_protocol_feature_codenames.end(), protocol_feature_validation_exception,
                  "Unsupported builtin_protocol_feature_t passed to constructor: ${codename}",
                  ("codename", static_cast<uint32_t>(codename)) );

      builtin_feature_codename = itr->second.codename;
   }

   void builtin_protocol_feature::reflector_init() {
      protocol_feature_base::reflector_init();

      for( const auto& p : builtin_protocol_feature_codenames ) {
         if( builtin_feature_codename.compare( p.second.codename ) == 0 ) {
            _codename = p.first;
            return;
         }
      }

      EOS_THROW( protocol_feature_validation_exception,
                 "Unsupported builtin protocol feature codename: ${codename}",
                 ("codename", builtin_feature_codename) );
   }


   digest_type builtin_protocol_feature::digest()const {
      digest_type::encoder enc;
      fc::raw::pack( enc, _type );
      fc::raw::pack( enc, description_digest  );
      fc::raw::pack( enc, dependencies );
      fc::raw::pack( enc, _codename );

      return enc.result();
   }

   fc::variant protocol_feature::to_variant( bool include_subjective_restrictions,
                                             fc::mutable_variant_object* additional_fields )const
   {
      EOS_ASSERT( builtin_feature, protocol_feature_exception, "not a builtin protocol feature" );

      fc::mutable_variant_object mvo;

      mvo( "feature_digest", feature_digest );

      if( additional_fields ) {
         for( const auto& e : *additional_fields ) {
            if( e.key().compare( "feature_digest" ) != 0 )
               mvo( e.key(), e.value() );
         }
      }

      if( include_subjective_restrictions ) {
         fc::mutable_variant_object subjective_restrictions;

         subjective_restrictions( "enabled", enabled );
         subjective_restrictions( "preactivation_required", preactivation_required );
         subjective_restrictions( "earliest_allowed_activation_time", earliest_allowed_activation_time );

         mvo( "subjective_restrictions", std::move( subjective_restrictions ) );
      }

      mvo( "description_digest", description_digest );
      mvo( "dependencies", dependencies );
      mvo( "protocol_feature_type", builtin_protocol_feature::feature_type_string );

      fc::variants specification;
      auto add_to_specification = [&specification]( const char* key_name, auto&& value ) {
         fc::mutable_variant_object obj;
         obj( "name", key_name );
         obj( "value", std::forward<decltype(value)>( value ) );
         specification.emplace_back( std::move(obj) );
      };


      add_to_specification( "builtin_feature_codename", builtin_protocol_feature_codename( *builtin_feature ) );

      mvo( "specification", std::move( specification ) );

      return fc::variant( std::move(mvo) );
   }

   protocol_feature_set::protocol_feature_set()
   {
      _recognized_builtin_protocol_features.reserve( builtin_protocol_feature_codenames.size() );
   }


   protocol_feature_set::recognized_t
   protocol_feature_set::is_recognized( const digest_type& feature_digest, time_point now )const {
      auto itr = _recognized_protocol_features.find( feature_digest );

      if( itr == _recognized_protocol_features.end() )
         return recognized_t::unrecognized;

      if( !itr->enabled )
         return recognized_t::disabled;

      if( itr->earliest_allowed_activation_time > now )
         return recognized_t::too_early;

      return recognized_t::ready;
   }

   std::optional<digest_type> protocol_feature_set::get_builtin_digest( builtin_protocol_feature_t feature_codename )const {
      uint32_t indx = static_cast<uint32_t>( feature_codename );

      if( indx >= _recognized_builtin_protocol_features.size() )
         return {};

      if( _recognized_builtin_protocol_features[indx] == _recognized_protocol_features.end() )
         return {};

      return _recognized_builtin_protocol_features[indx]->feature_digest;
   }

   const protocol_feature& protocol_feature_set::get_protocol_feature( const digest_type& feature_digest )const {
      auto itr = _recognized_protocol_features.find( feature_digest );

      EOS_ASSERT( itr != _recognized_protocol_features.end(), protocol_feature_exception,
                  "unrecognized protocol feature with digest: ${digest}",
                  ("digest", feature_digest)
      );

      return *itr;
   }

   bool protocol_feature_set::validate_dependencies(
                                    const digest_type& feature_digest,
                                    const std::function<bool(const digest_type&)>& validator
   )const {
      auto itr = _recognized_protocol_features.find( feature_digest );

      if( itr == _recognized_protocol_features.end() ) return false;

      for( const auto& d : itr->dependencies ) {
         if( !validator(d) ) return false;
      }

      return true;
   }

   builtin_protocol_feature
   protocol_feature_set::make_default_builtin_protocol_feature(
      builtin_protocol_feature_t codename,
      const std::function<digest_type(builtin_protocol_feature_t dependency)>& handle_dependency
   ) {
      auto itr = builtin_protocol_feature_codenames.find( codename );

      EOS_ASSERT( itr != builtin_protocol_feature_codenames.end(), protocol_feature_validation_exception,
                  "Unsupported builtin_protocol_feature_t: ${codename}",
                  ("codename", static_cast<uint32_t>(codename)) );

      flat_set<digest_type> dependencies;
      dependencies.reserve( itr->second.builtin_dependencies.size() );

      for( const auto& d : itr->second.builtin_dependencies ) {
         dependencies.insert( handle_dependency( d ) );
      }

      return {itr->first, itr->second.description_digest, std::move(dependencies), itr->second.subjective_restrictions};
   }

   const protocol_feature& protocol_feature_set::add_feature( const builtin_protocol_feature& f ) {
      auto builtin_itr = builtin_protocol_feature_codenames.find( f._codename );
      EOS_ASSERT( builtin_itr != builtin_protocol_feature_codenames.end(), protocol_feature_validation_exception,
                  "Builtin protocol feature has unsupported builtin_protocol_feature_t: ${codename}",
                  ("codename", static_cast<uint32_t>( f._codename )) );

      uint32_t indx = static_cast<uint32_t>( f._codename );

      if( indx < _recognized_builtin_protocol_features.size() ) {
         EOS_ASSERT( _recognized_builtin_protocol_features[indx] == _recognized_protocol_features.end(),
                     protocol_feature_exception,
                     "builtin protocol feature with codename '${codename}' already added",
                     ("codename", f.builtin_feature_codename) );
      }

      auto feature_digest = f.digest();

      const auto& expected_builtin_dependencies = builtin_itr->second.builtin_dependencies;
      flat_set<builtin_protocol_feature_t> satisfied_builtin_dependencies;
      satisfied_builtin_dependencies.reserve( expected_builtin_dependencies.size() );

      for( const auto& d : f.dependencies ) {
         auto itr = _recognized_protocol_features.find( d );
         EOS_ASSERT( itr != _recognized_protocol_features.end(), protocol_feature_exception,
            "builtin protocol feature with codename '${codename}' and digest of ${digest} has a dependency on a protocol feature with digest ${dependency_digest} that is not recognized",
            ("codename", f.builtin_feature_codename)
            ("digest",  feature_digest)
            ("dependency_digest", d )
         );

         if( itr->builtin_feature
             && expected_builtin_dependencies.find( *itr->builtin_feature )
                  != expected_builtin_dependencies.end() )
         {
            satisfied_builtin_dependencies.insert( *itr->builtin_feature );
         }
      }

      if( expected_builtin_dependencies.size() > satisfied_builtin_dependencies.size() ) {
         flat_set<builtin_protocol_feature_t> missing_builtins;
         missing_builtins.reserve( expected_builtin_dependencies.size() - satisfied_builtin_dependencies.size() );
         std::set_difference( expected_builtin_dependencies.begin(), expected_builtin_dependencies.end(),
                              satisfied_builtin_dependencies.begin(), satisfied_builtin_dependencies.end(),
                              end_inserter( missing_builtins )
         );

         vector<string> missing_builtins_with_names;
         missing_builtins_with_names.reserve( missing_builtins.size() );
         for( const auto& builtin_codename : missing_builtins ) {
            auto itr = builtin_protocol_feature_codenames.find( builtin_codename );
            EOS_ASSERT( itr != builtin_protocol_feature_codenames.end(),
                        protocol_feature_exception,
                        "Unexpected error"
            );
            missing_builtins_with_names.emplace_back( itr->second.codename );
         }

         EOS_THROW(  protocol_feature_validation_exception,
                     "Not all the builtin dependencies of the builtin protocol feature with codename '${codename}' and digest of ${digest} were satisfied.",
                     ("missing_dependencies", missing_builtins_with_names)
         );
      }

      auto res = _recognized_protocol_features.insert( protocol_feature{
         feature_digest,
         f.description_digest,
         f.dependencies,
         f.subjective_restrictions.earliest_allowed_activation_time,
         f.subjective_restrictions.preactivation_required,
         f.subjective_restrictions.enabled,
         f._codename
      } );

      EOS_ASSERT( res.second, protocol_feature_exception,
                  "builtin protocol feature with codename '${codename}' has a digest of ${digest} but another protocol feature with the same digest has already been added",
                  ("codename", f.builtin_feature_codename)("digest", feature_digest) );

      if( indx >= _recognized_builtin_protocol_features.size() ) {
         for( auto i =_recognized_builtin_protocol_features.size(); i <= indx; ++i ) {
            _recognized_builtin_protocol_features.push_back( _recognized_protocol_features.end() );
         }
      }

      _recognized_builtin_protocol_features[indx] = res.first;
      return *res.first;
   }



   protocol_feature_manager::protocol_feature_manager(
      protocol_feature_set&& pfs,
      std::function<deep_mind_handler*()> get_deep_mind_logger
   ):_protocol_feature_set( std::move(pfs) ), _get_deep_mind_logger(get_deep_mind_logger)
   {
      _builtin_protocol_features.resize( _protocol_feature_set._recognized_builtin_protocol_features.size() );
   }

   void protocol_feature_manager::init( chainbase::database& db ) {
      EOS_ASSERT( !is_initialized(), protocol_feature_exception, "cannot initialize protocol_feature_manager twice" );


      auto reset_initialized = fc::make_scoped_exit( [this]() { _initialized = false; } );
      _initialized = true;

      for( const auto& f : db.get<protocol_state_object>().activated_protocol_features ) {
         activate_feature( f.feature_digest, f.activation_block_num );
      }

      reset_initialized.cancel();
   }

   const protocol_feature* protocol_feature_manager::const_iterator::get_pointer()const {
      //EOS_ASSERT( _pfm, protocol_feature_iterator_exception, "cannot dereference singular iterator" );
      //EOS_ASSERT( _index != end_index, protocol_feature_iterator_exception, "cannot dereference end iterator" );
      return &*(_pfm->_activated_protocol_features[_index].iterator_to_protocol_feature);
   }

   uint32_t protocol_feature_manager::const_iterator::activation_ordinal()const {
      EOS_ASSERT( _pfm,
                   protocol_feature_iterator_exception,
                  "called activation_ordinal() on singular iterator"
      );
      EOS_ASSERT( _index != end_index,
                   protocol_feature_iterator_exception,
                  "called activation_ordinal() on end iterator"
      );

      return _index;
   }

   uint32_t protocol_feature_manager::const_iterator::activation_block_num()const {
      EOS_ASSERT( _pfm,
                   protocol_feature_iterator_exception,
                  "called activation_block_num() on singular iterator"
      );
      EOS_ASSERT( _index != end_index,
                   protocol_feature_iterator_exception,
                  "called activation_block_num() on end iterator"
      );

      return _pfm->_activated_protocol_features[_index].activation_block_num;
   }

   protocol_feature_manager::const_iterator& protocol_feature_manager::const_iterator::operator++() {
      EOS_ASSERT( _pfm, protocol_feature_iterator_exception, "cannot increment singular iterator" );
      EOS_ASSERT( _index != end_index, protocol_feature_iterator_exception, "cannot increment end iterator" );

      ++_index;
      if( _index >= _pfm->_activated_protocol_features.size() ) {
         _index = end_index;
      }

      return *this;
   }

   protocol_feature_manager::const_iterator& protocol_feature_manager::const_iterator::operator--() {
      EOS_ASSERT( _pfm, protocol_feature_iterator_exception, "cannot decrement singular iterator" );
      if( _index == end_index ) {
         EOS_ASSERT( _pfm->_activated_protocol_features.size() > 0,
                     protocol_feature_iterator_exception,
                     "cannot decrement end iterator when no protocol features have been activated"
         );
         _index = _pfm->_activated_protocol_features.size() - 1;
      } else {
         EOS_ASSERT( _index > 0,
                     protocol_feature_iterator_exception,
                     "cannot decrement iterator at the beginning of protocol feature activation list" )
         ;
         --_index;
      }
      return *this;
   }

   protocol_feature_manager::const_iterator protocol_feature_manager::cbegin()const {
      if( _activated_protocol_features.size() == 0 ) {
         return cend();
      } else {
         return const_iterator( this, 0 );
      }
   }

   protocol_feature_manager::const_iterator
   protocol_feature_manager::at_activation_ordinal( uint32_t activation_ordinal )const {
      if( activation_ordinal >= _activated_protocol_features.size() ) {
         return cend();
      }

      return const_iterator{this, static_cast<std::size_t>(activation_ordinal)};
   }

   protocol_feature_manager::const_iterator
   protocol_feature_manager::lower_bound( uint32_t block_num )const {
      const auto begin = _activated_protocol_features.cbegin();
      const auto end   = _activated_protocol_features.cend();
      auto itr = std::lower_bound( begin, end, block_num, []( const protocol_feature_entry& lhs, uint32_t rhs ) {
         return lhs.activation_block_num < rhs;
      } );

      if( itr == end ) {
         return cend();
      }

      return const_iterator{this, static_cast<std::size_t>(itr - begin)};
   }

   protocol_feature_manager::const_iterator
   protocol_feature_manager::upper_bound( uint32_t block_num )const {
      const auto begin = _activated_protocol_features.cbegin();
      const auto end   = _activated_protocol_features.cend();
      auto itr = std::upper_bound( begin, end, block_num, []( uint32_t lhs, const protocol_feature_entry& rhs ) {
         return lhs < rhs.activation_block_num;
      } );

      if( itr == end ) {
         return cend();
      }

      return const_iterator{this, static_cast<std::size_t>(itr - begin)};
   }

   bool protocol_feature_manager::is_builtin_activated( builtin_protocol_feature_t feature_codename,
                                                        uint32_t current_block_num )const
   {
      uint32_t indx = static_cast<uint32_t>( feature_codename );

      if( indx >= _builtin_protocol_features.size() ) return false;

      return (_builtin_protocol_features[indx].activation_block_num <= current_block_num);
   }

   void protocol_feature_manager::activate_feature( const digest_type& feature_digest,
                                                    uint32_t current_block_num )
   {
      EOS_ASSERT( is_initialized(), protocol_feature_exception, "protocol_feature_manager is not yet initialized" );

      auto itr = _protocol_feature_set.find( feature_digest );

      EOS_ASSERT( itr != _protocol_feature_set.end(), protocol_feature_exception,
                  "unrecognized protocol feature digest: ${digest}", ("digest", feature_digest) );

      if( _activated_protocol_features.size() > 0 ) {
         const auto& last = _activated_protocol_features.back();
         EOS_ASSERT( last.activation_block_num <= current_block_num,
                     protocol_feature_exception,
                     "last protocol feature activation block num is ${last_activation_block_num} yet "
                     "attempting to activate protocol feature with a current block num of ${current_block_num}"
                     "protocol features is ${last_activation_block_num}",
                     ("current_block_num", current_block_num)
                     ("last_activation_block_num", last.activation_block_num)
         );
      }

      EOS_ASSERT( itr->builtin_feature,
                  protocol_feature_exception,
                  "invariant failure: encountered non-builtin protocol feature which is not yet supported"
      );

      uint32_t indx = static_cast<uint32_t>( *itr->builtin_feature );

      EOS_ASSERT( indx < _builtin_protocol_features.size(), protocol_feature_exception,
                  "invariant failure while trying to activate feature with digest '${digest}': "
                  "unsupported builtin_protocol_feature_t ${codename}",
                  ("digest", feature_digest)
                  ("codename", indx)
      );

      EOS_ASSERT( _builtin_protocol_features[indx].activation_block_num == builtin_protocol_feature_entry::not_active,
                  protocol_feature_exception,
                  "cannot activate already activated builtin feature with digest: ${digest}",
                  ("digest", feature_digest)
      );

      if (auto dm_logger = _get_deep_mind_logger()) {
         dm_logger->on_activate_feature(*itr);
      }

      _activated_protocol_features.push_back( protocol_feature_entry{itr, current_block_num} );
      _builtin_protocol_features[indx].previous = _head_of_builtin_activation_list;
      _builtin_protocol_features[indx].activation_block_num = current_block_num;
      _head_of_builtin_activation_list = indx;
   }

   void protocol_feature_manager::popped_blocks_to( uint32_t block_num ) {
      EOS_ASSERT( is_initialized(), protocol_feature_exception, "protocol_feature_manager is not yet initialized" );

      while( _head_of_builtin_activation_list != builtin_protocol_feature_entry::no_previous ) {
         auto& e = _builtin_protocol_features[_head_of_builtin_activation_list];
         if( e.activation_block_num <= block_num ) break;

         _head_of_builtin_activation_list = e.previous;
         e.previous = builtin_protocol_feature_entry::no_previous;
         e.activation_block_num = builtin_protocol_feature_entry::not_active;
      }

      while( _activated_protocol_features.size() > 0
              && block_num < _activated_protocol_features.back().activation_block_num )
      {
         _activated_protocol_features.pop_back();
      }
   }

} }  // eosio::chain
