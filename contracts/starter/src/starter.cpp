#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <starter.hpp>

void eden::starter_contract::hi( eosio::name account ) {
  require_auth( account );
}

EOSIO_ACTION_DISPATCHER( eden::actions )

EOSIO_ABIGEN( actions( eden::actions ), table( "starter"_n, eden::starter ) )