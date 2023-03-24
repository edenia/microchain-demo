#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

namespace eden {
  struct starter {
    eosio::name account;

    uint64_t primary_key() const { return account.value; }
  };
  EOSIO_REFLECT( starter, account )
  typedef eosio::multi_index< "starter"_n, starter > starter_table;

  struct starter_contract : public eosio::contract {
  public:
    using eosio::contract::contract;

    void hi( eosio::name account );

  private:
    const eosio::name DEFAULT_ACCOUNT = eosio::name( "starter" );
  };

  EOSIO_ACTIONS( starter_contract, "starter"_n, action( hi, account ) )

} // namespace eden
