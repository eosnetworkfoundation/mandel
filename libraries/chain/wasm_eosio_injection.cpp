#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
#include <fc/exception/exception.hpp>
#include "IR/Module.h"
#include "WASM/WASM.h"

namespace eosio { namespace chain { namespace wasm_injections {
using namespace IR;
using namespace eosio::chain::wasm_constraints;

std::map<std::vector<uint16_t>, uint32_t> injector_utils::type_slots;
std::map<std::string, uint32_t>           injector_utils::registered_injected;
std::map<uint32_t, uint32_t>              injector_utils::injected_index_mapping;
uint32_t                                  injector_utils::next_injected_index;


void noop_injection_visitor::inject( Module& m ) { /* just pass */ }
void noop_injection_visitor::initializer() { /* just pass */ }

void memories_injection_visitor::inject( Module& m ) {
}
void memories_injection_visitor::initializer() {
}

void data_segments_injection_visitor::inject( Module& m ) {
}
void data_segments_injection_visitor::initializer() {
}

}}} // namespace eosio, chain, injectors
