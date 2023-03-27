#pragma once

#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/fixed_bytes.hpp>
#include <eosio/name.hpp>
#include <eosio/reflection.hpp>
#include <eosio/time.hpp>
#include <eosio/varint.hpp>
#include <variant>
#include <vector>

namespace starter
{

   // WORKING: create event and listen for it in the microchain

   struct greeting_event
   {
      eosio::name account;
      std::string message;
   };
   EOSIO_REFLECT(greeting_event, account, message)

   using event = std::variant<greeting_event>;

   void push_event(const event& e, eosio::name self);
   void send_events(eosio::name self);
}  // namespace starter
