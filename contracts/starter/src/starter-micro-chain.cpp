#include <boost/multi_index/key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <chainbase/chainbase.hpp>
#include <clchain/crypto.hpp>
#include <clchain/graphql_connection.hpp>
#include <clchain/subchain.hpp>
#include <eosio/abi.hpp>
#include <eosio/from_bin.hpp>
#include <eosio/ship_protocol.hpp>
#include <eosio/to_bin.hpp>
#include <events.hpp>
#include <starter.hpp>

using namespace eosio::literals;

eosio::name starter_account;

std::variant<std::string, std::vector<char>> result;

subchain::block_log block_log;

template <typename T>
void dump(const T& ind)
{
   printf("%s\n", eosio::format_json(ind).c_str());
}

namespace boost
{
   BOOST_NORETURN void throw_exception(std::exception const& e)
   {
      eosio::detail::assert_or_throw(e.what());
   }
   BOOST_NORETURN void throw_exception(std::exception const& e, boost::source_location const& loc)
   {
      eosio::detail::assert_or_throw(e.what());
   }
}  // namespace boost

struct by_id;
struct by_pk;
struct by_invitee;
struct by_group;
struct by_round;
struct by_createdAt;
struct by_member;
struct by_owner;

template <typename T, typename... Indexes>
using mic = boost::
    multi_index_container<T, boost::multi_index::indexed_by<Indexes...>, chainbase::allocator<T>>;

template <typename T>
using ordered_by_id = boost::multi_index::ordered_unique<  //
    boost::multi_index::tag<by_id>,
    boost::multi_index::key<&T::id>>;

template <typename T>
using ordered_by_pk = boost::multi_index::ordered_unique<  //
    boost::multi_index::tag<by_pk>,
    boost::multi_index::key<&T::by_pk>>;

template <typename T>
using ordered_by_createdAt = boost::multi_index::ordered_unique<  //
    boost::multi_index::tag<by_createdAt>,
    boost::multi_index::key<&T::by_createdAt>>;

template <typename Tag, typename Table, typename Key, typename F>
void add_or_modify(Table& table, const Key& key, F&& f)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   if (it != idx.end())
      table.modify(*it, [&](auto& obj) { return f(false, obj); });
   else
      table.emplace([&](auto& obj) { return f(true, obj); });
}

template <typename Tag, typename Table, typename Key, typename F>
void add_or_replace(Table& table, const Key& key, F&& f)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   if (it != idx.end())
      table.remove(*it);
   table.emplace(f);
}

template <typename Tag, typename Table, typename Key, typename F>
void modify(Table& table, const Key& key, F&& f)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   eosio::check(it != idx.end(), "missing record");
   table.modify(*it, [&](auto& obj) { return f(obj); });
}

template <typename Tag, typename Table, typename Key>
void remove_if_exists(Table& table, const Key& key)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   if (it != idx.end())
      table.remove(*it);
}

template <typename Tag, typename Table, typename Key>
const auto& get(Table& table, const Key& key)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   eosio::check(it != idx.end(), "missing record");
   return *it;
}

template <typename Tag, typename Table, typename Key>
const typename Table::value_type* get_ptr(Table& table, const Key& key)
{
   auto& idx = table.template get<Tag>();
   auto it = idx.find(key);
   if (it == idx.end())
      return nullptr;
   return &*it;
}

enum tables
{
   greeting_table,
};

struct Greeting;
constexpr const char GreetingConnection_name[] = "GreetingConnection";
constexpr const char GreetingEdge_name[] = "GreetingEdge";
using GreetingConnection = clchain::Connection<
    clchain::ConnectionConfig<Greeting, GreetingConnection_name, GreetingEdge_name>>;

struct greeting
{
   eosio::name account;
   std::string message;
};

struct greeting_object : public chainbase::object<greeting_table, greeting_object>
{
   CHAINBASE_DEFAULT_CONSTRUCTOR(greeting_object)

   id_type id;
   greeting greeting;

   eosio::name by_pk() const { return greeting.account; }
};
using greeting_index =
    mic<greeting_object, ordered_by_id<greeting_object>, ordered_by_pk<greeting_object>>;

struct database
{
   chainbase::database db;
   chainbase::generic_index<greeting_index> greetings;

   database() { db.add_index(greetings); }
};
database db;

struct Greeting
{
   eosio::name account;
   const greeting* greeting;

   const std::string* message() const { return greeting ? &greeting->message : nullptr; }
};
EOSIO_REFLECT2(Greeting, account, message)

struct block_state
{
   bool in_withdraw = false;
   bool in_manual_transfer = false;
};

struct action_context
{
   const subchain::eosio_block& block;
   block_state& block_state;
   const subchain::transaction& transaction;
   const subchain::action& action;
};

void clear_table(auto& table)
{
   for (auto it = table.begin(); it != table.end();)
   {
      auto next = it;
      ++next;
      table.remove(*it);
      it = next;
   }
}

void clearall()
{
   clear_table(db.greetings);
}

void hi(eosio::name account, const std::string& message)
{
   add_or_replace<by_pk>(db.greetings, account,
                         [&](auto& obj)
                         {
                            obj.greeting.account = account;
                            obj.greeting.message = message;
                         });
}

void handle_event(const starter::greeting_event& event) {}

void handle_event(const auto& event) {}

void handle_event(const action_context& context, const starter::event& event)
{
   std::visit([&](const auto& event) { handle_event(context, event); }, event);
}

template <typename... Args>
void call(void (*f)(Args...), const action_context& context, eosio::input_stream& s)
{
   std::tuple<eosio::remove_cvref_t<Args>...> t;
   // TODO: prevent abort, indicate what failed
   eosio::from_bin(t, s);
   std::apply([f](auto&&... args) { f(std::move(args)...); }, t);
}

template <typename... Args>
void call(void (*f)(const action_context&, Args...),
          const action_context& context,
          eosio::input_stream& s)
{
   std::tuple<eosio::remove_cvref_t<Args>...> t;
   // TODO: prevent abort, indicate what failed
   eosio::from_bin(t, s);
   std::apply([&](auto&&... args) { f(context, std::move(args)...); }, t);
}

bool dispatch(eosio::name action_name, const action_context& context, eosio::input_stream& s);

// TODO: study this function
void run(const action_context& context, eosio::input_stream& s)
{
   //    eden::run_auth auth;
   //    eosio::varuint32 num_verbs;
   //    from_bin(auth, s);
   //    from_bin(num_verbs, s);
   //    for (uint32_t i = 0; i < num_verbs.value; ++i)
   //    {
   //       auto index = eosio::varuint32_from_bin(s);
   //       auto name = eden::actions::get_name_for_session_action(index);
   //       if (!dispatch(name, context, s))
   //          // fatal because this throws off the rest of the stream
   //          eosio::check(false,
   //                       "run: verb not found: " + std::to_string(index) + " " + name.to_string());
   //    }
   //    eosio::check(!s.remaining(), "unpack error (extra data) within run");
}

bool dispatch(eosio::name action_name, const action_context& context, eosio::input_stream& s)
{
   if (action_name == "run"_n)
      run(context, s);
   else if (action_name == "clearall"_n)
      call(clearall, context, s);
   else if (action_name == "hi"_n)
      call(hi, context, s);
   else
      return false;
   return true;
}

void filter_block(const subchain::eosio_block& block)
{
   block_state block_state{};
   for (auto& trx : block.transactions)
   {
      for (auto& action : trx.actions)
      {
         action_context context{block, block_state, trx, action};
         if (action.firstReceiver == starter_account)
         {
            eosio::input_stream s(action.hexData.data);
            dispatch(action.name, context, s);
         }
         else if (action.firstReceiver == "eosio.null"_n && action.name == "chain.events"_n &&
                  action.creatorAction && action.creatorAction->receiver == starter_account)
         {
            // TODO: prevent abort, indicate what failed
            auto events = eosio::convert_from_bin<std::vector<starter::event>>(action.hexData.data);
            for (auto& event : events)
               handle_event(context, event);
         }
      }  // for(action)

      // garbage collection housekeeping

      eosio::check(!block_state.in_withdraw && !block_state.in_manual_transfer,
                   "missing transfer notification");
   }  // for(trx)
}  // filter_block

std::vector<subchain::transaction> ship_to_eden_transactions(
    std::vector<eosio::ship_protocol::transaction_trace>& traces)
{
   std::vector<subchain::transaction> transactions;

   for (const auto& transaction_trace : traces)
   {
      std::visit(
          [&](const auto& trx_trace)
          {
             subchain::transaction transaction{
                 .id = trx_trace.id,
             };

             for (const auto& action_trace : trx_trace.action_traces)
             {
                std::visit(
                    [&](const auto& act_trace)
                    {
                       std::optional<subchain::creator_action> creatorAction;
                       if (act_trace.creator_action_ordinal.value > 0)
                       {
                          std::visit(
                              [&](const auto& creator_action_trace)
                              {
                                 std::visit(
                                     [&](const auto& receipt)
                                     {
                                        creatorAction = subchain::creator_action{
                                            .seq = receipt.global_sequence,
                                            .receiver = creator_action_trace.receiver,
                                        };
                                     },
                                     *creator_action_trace.receipt);
                              },
                              trx_trace.action_traces[act_trace.creator_action_ordinal.value - 1]);
                       }

                       std::vector<char> data(act_trace.act.data.pos, act_trace.act.data.end);
                       eosio::bytes hexData{data};

                       std::visit(
                           [&](const auto& receipt)
                           {
                              subchain::action action{
                                  .seq = receipt.global_sequence,
                                  .firstReceiver = act_trace.act.account,
                                  .receiver = act_trace.receiver,
                                  .name = act_trace.act.name,
                                  .creatorAction = creatorAction,
                                  .hexData = std::move(hexData),
                              };

                              transaction.actions.push_back(std::move(action));
                           },
                           *act_trace.receipt);
                    },
                    action_trace);
             }

             transactions.push_back(std::move(transaction));
          },
          transaction_trace);
   }

   return transactions;
}

void forked_n_blocks(size_t n)
{
   if (n)
      printf("forked %d blocks, %d now in log\n", (int)n, (int)block_log.blocks.size());
   while (n--)
      db.db.undo();
}

bool add_block(subchain::block_with_id&& bi, uint32_t eosio_irreversible)
{
   auto [status, num_forked] = block_log.add_block(bi);
   if (status)
      return false;
   forked_n_blocks(num_forked);
   if (auto* b = block_log.block_before_eosio_num(eosio_irreversible + 1))
      block_log.irreversible = std::max(block_log.irreversible, b->num);
   db.db.commit(block_log.irreversible);
   bool need_undo = bi.num > block_log.irreversible;
   auto session = db.db.start_undo_session(bi.num > block_log.irreversible);
   filter_block(bi.eosioBlock);
   session.push();
   if (!need_undo)
      db.db.set_revision(bi.num);
   return true;
}

bool add_block(subchain::block&& eden_block, uint32_t eosio_irreversible)
{
   auto bin = eosio::convert_to_bin(eden_block);
   subchain::block_with_id bi;
   static_cast<subchain::block&>(bi) = std::move(eden_block);
   bi.id = clchain::sha256(bin.data(), bin.size());
   auto bin_with_id = eosio::convert_to_bin(bi.id);
   bin_with_id.insert(bin_with_id.end(), bin.begin(), bin.end());
   result = std::move(bin_with_id);
   return add_block(std::move(bi), eosio_irreversible);
}

bool add_block(subchain::eosio_block&& eosioBlock, uint32_t eosio_irreversible)
{
   subchain::block eden_block;
   eden_block.eosioBlock = std::move(eosioBlock);

   auto* eden_prev = block_log.block_before_eosio_num(eden_block.eosioBlock.num);
   if (eden_prev)
   {
      eden_block.num = eden_prev->num + 1;
      eden_block.previous = eden_prev->id;
   }
   else
      eden_block.num = 1;

   return add_block(std::move(eden_block), eosio_irreversible);
}

bool add_block(eosio::ship_protocol::block_position block,
               eosio::ship_protocol::block_position prev,
               uint32_t eosio_irreversible,
               eosio::block_timestamp timestamp,
               std::vector<eosio::ship_protocol::transaction_trace> traces)
{
   subchain::eosio_block eosio_block;
   eosio_block.num = block.block_num;
   eosio_block.id = block.block_id;
   eosio_block.previous = prev.block_id;
   eosio_block.timestamp = timestamp.to_time_point();
   eosio_block.transactions = ship_to_eden_transactions(traces);
   return add_block(std::move(eosio_block), eosio_irreversible);
}

struct Query
{
   subchain::BlockLog blockLog;

   GreetingConnection greetings(std::optional<eosio::name> gt,
                                std::optional<eosio::name> ge,
                                std::optional<eosio::name> lt,
                                std::optional<eosio::name> le,
                                std::optional<uint32_t> first,
                                std::optional<uint32_t> last,
                                std::optional<std::string> before,
                                std::optional<std::string> after) const
   {
      return clchain::make_connection<GreetingConnection, eosio::name>(
          gt, ge, lt, le, first, last, before, after,      //
          db.greetings.get<by_pk>(),                       //
          [](auto& obj) { return obj.greeting.account; },  //
          [](auto& obj) {
             return Greeting{obj.greeting.account, &obj.greeting};
          },
          [](auto& greetings, auto key) { return greetings.lower_bound(key); },
          [](auto& greetings, auto key) { return greetings.upper_bound(key); });
   }
};
EOSIO_REFLECT2(Query,
               blockLog,
               method(greetings, "gt", "ge", "lt", "le", "first", "last", "before", "after"))

auto schema = clchain::get_gql_schema<Query>();

// TODO: prevent from_json from aborting
[[clang::export_name("addEosioBlockJson")]] bool addEosioBlockJson(const char* json,
                                                                   uint32_t size,
                                                                   uint32_t eosio_irreversible)
{
   std::string str(json, size);
   eosio::json_token_stream s(str.data());
   subchain::eosio_block eosio_block;
   eosio::from_json(eosio_block, s);
   return add_block(std::move(eosio_block), eosio_irreversible);
}

// TODO: prevent from_bin from aborting
[[clang::export_name("addBlock")]] bool addBlock(const char* data,
                                                 uint32_t size,
                                                 uint32_t eosio_irreversible)
{
   // TODO: verify id integrity
   eosio::input_stream bin{data, size};
   subchain::block_with_id block;
   eosio::from_bin(block, bin);
   return add_block(std::move(block), eosio_irreversible);
}

[[clang::export_name("getShipBlocksRequest")]] bool getShipBlocksRequest(uint32_t block_num)
{
   eosio::ship_protocol::request request = eosio::ship_protocol::get_blocks_request_v0{
       .start_block_num = block_num,
       .end_block_num = 0xffff'ffff,
       .max_messages_in_flight = 0xffff'ffff,
       .fetch_block = true,
       .fetch_traces = true,
   };
   result = eosio::convert_to_bin(request);

   return true;
}

[[clang::export_name("pushShipMessage")]] bool pushShipMessage(const char* data, uint32_t size)
{
   eosio::input_stream bin{data, size};
   eosio::ship_protocol::result result;
   eosio::from_bin(result, bin);

   if (auto* blocks_result = std::get_if<eosio::ship_protocol::get_blocks_result_v0>(&result))
   {
      eosio::ship_protocol::signed_block signed_block;
      if (blocks_result->block)
      {
         eosio::from_bin(signed_block, blocks_result->block.value());
      }

      std::vector<eosio::ship_protocol::transaction_trace> traces;
      if (blocks_result->traces)
      {
         eosio::from_bin(traces, blocks_result->traces.value());
      }

      auto prev_block = blocks_result->prev_block ? blocks_result->prev_block.value()
                                                  : eosio::ship_protocol::block_position{};

      return add_block(blocks_result->this_block.value(), prev_block,
                       blocks_result->last_irreversible.block_num, signed_block.timestamp, traces);
   }
   return false;
}

[[clang::export_name("setIrreversible")]] uint32_t setIrreversible(uint32_t irreversible)
{
   if (auto* b = block_log.block_before_num(irreversible + 1))
      block_log.irreversible = std::max(block_log.irreversible, b->num);
   db.db.commit(block_log.irreversible);
   return block_log.irreversible;
}

[[clang::export_name("trimBlocks")]] void trimBlocks()
{
   block_log.trim();
}

[[clang::export_name("undoBlockNum")]] void undoBlockNum(uint32_t blockNum)
{
   forked_n_blocks(block_log.undo(blockNum));
}

[[clang::export_name("undoEosioNum")]] void undoEosioNum(uint32_t eosioNum)
{
   if (auto* b = block_log.block_by_eosio_num(eosioNum))
      forked_n_blocks(block_log.undo(b->num));
}

[[clang::export_name("getBlock")]] bool getBlock(uint32_t num)
{
   auto block = block_log.block_by_num(num);
   if (!block)
      return false;
   result = eosio::convert_to_bin(*block);
   return true;
}

// TODO: switch to uint64_t (js BigInt) after we upgrade to nodejs >= 15
extern "C" void __wasm_call_ctors();
[[clang::export_name("initialize")]] void initialize(uint32_t starter_account_low,
                                                     uint32_t starter_account_high)
{
   __wasm_call_ctors();
   starter_account.value = (uint64_t(starter_account_high) << 32) | starter_account_low;
}

[[clang::export_name("allocateMemory")]] void* allocateMemory(uint32_t size)
{
   return malloc(size);
}

[[clang::export_name("freeMemory")]] void freeMemory(void* p)
{
   free(p);
}

[[clang::export_name("getResultSize")]] uint32_t getResultSize()
{
   return std::visit([](auto& data) { return data.size(); }, result);
}

[[clang::export_name("getResult")]] const char* getResult()
{
   return std::visit([](auto& data) { return data.data(); }, result);
}

[[clang::export_name("getSchemaSize")]] uint32_t getSchemaSize()
{
   return schema.size();
}

[[clang::export_name("getSchema")]] const char* getSchema()
{
   return schema.c_str();
}

[[clang::export_name("query")]] void query(const char* query,
                                           uint32_t size,
                                           const char* variables,
                                           uint32_t variables_size)
{
   Query root{block_log};
   result = clchain::gql_query(root, {query, size}, {variables, variables_size});
}
