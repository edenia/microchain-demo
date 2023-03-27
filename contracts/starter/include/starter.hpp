#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

namespace starter
{
   struct greeting
   {
      eosio::name account;
      std::string message;

      uint64_t primary_key() const { return account.value; }
   };
   EOSIO_REFLECT(greeting, account, message)
   using greeting_table = eosio::multi_index<"greeting"_n, greeting>;

   class starter_contract : public eosio::contract
   {
     private:
      greeting_table greeting_tb;

     public:
      using eosio::contract::contract;

      starter_contract(eosio::name receiver, eosio::name code, eosio::datastream<const char*> ds)
          : contract(receiver, code, ds), greeting_tb(receiver, receiver.value)
      {
      }

      void hi(eosio::name account, std::string& message);
   };

   EOSIO_ACTIONS(starter_contract, "starter"_n, action(hi, account, message))

}  // namespace starter
