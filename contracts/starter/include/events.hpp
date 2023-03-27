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

namespace eden
{
   struct session_del_event
   {
      eosio::name eden_account;
      eosio::public_key key;
   };
   EOSIO_REFLECT(session_del_event, eden_account, key)

   using event = std::variant<session_del_event>;

   void push_event(const event& e, eosio::name self);
   void send_events(eosio::name self);
}  // namespace eden
