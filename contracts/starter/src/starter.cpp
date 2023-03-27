#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>

#include <events.hpp>
#include <starter.hpp>

namespace starter
{

   void starter_contract::hi(eosio::name account, std::string& message)
   {
      require_auth(account);

      eosio::check(eosio::is_account(account), "account does not exist");
      eosio::check(message.size() <= 256, "message is too long");

      push_event(greeting_event{account, message}, get_self());

      auto itr = greeting_tb.find(account.value);
      if (itr == greeting_tb.end())
      {
         greeting_tb.emplace(account,
                             [&](auto& row)
                             {
                                row.account = account;
                                row.message = message;
                             });
      }
      else
      {
         greeting_tb.modify(itr, eosio::same_payer, [&](auto& row) { row.message = message; });
      }
   }
}  // namespace starter

EOSIO_ACTION_DISPATCHER(starter::actions)

EOSIO_ABIGEN(actions(starter::actions), table("greeting"_n, starter::greeting))