#include <boost/multi_index/key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <chainbase/chainbase.hpp>
#include <clchain/crypto.hpp>
#include <clchain/graphql_connection.hpp>
#include <clchain/subchain.hpp>
// #include <eden.hpp>
#include <eosio/abi.hpp>
#include <eosio/from_bin.hpp>
#include <eosio/ship_protocol.hpp>
#include <eosio/to_bin.hpp>
#include <events.hpp>

using namespace eosio::literals;

eosio::name eden_account;
eosio::name token_account;
eosio::name atomic_account;
eosio::name atomicmarket_account;

constexpr eosio::name pool_account(eosio::name pool)
{
   return eosio::name{pool.value | 0x0f};
}
constexpr eosio::name master_pool = pool_account("master"_n);
eosio::name distribution_fund;

const eosio::name account_min = eosio::name{0};
const eosio::name account_max = eosio::name{~uint64_t(0)};
const eosio::block_timestamp block_timestamp_min = eosio::block_timestamp{0};
const eosio::block_timestamp block_timestamp_max = eosio::block_timestamp{~uint32_t(0)};

const eosio::ecc_public_key ecc_public_key_min = {};

const eosio::ecc_public_key ecc_public_key_max = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

const eosio::public_key public_key_min_k1{std::in_place_index_t<0>{}, ecc_public_key_min};
const eosio::public_key public_key_max_r1{std::in_place_index_t<1>{}, ecc_public_key_max};

// TODO: switch to uint64_t (js BigInt) after we upgrade to nodejs >= 15
extern "C" void __wasm_call_ctors();
[[clang::export_name("initialize")]] void initialize(uint32_t eden_account_low,
                                                     uint32_t eden_account_high,
                                                     uint32_t token_account_low,
                                                     uint32_t token_account_high,
                                                     uint32_t atomic_account_low,
                                                     uint32_t atomic_account_high,
                                                     uint32_t atomicmarket_account_low,
                                                     uint32_t atomicmarket_account_high)
{
   __wasm_call_ctors();
   eden_account.value = (uint64_t(eden_account_high) << 32) | eden_account_low;
   token_account.value = (uint64_t(token_account_high) << 32) | token_account_low;
   atomic_account.value = (uint64_t(atomic_account_high) << 32) | atomic_account_low;
   atomicmarket_account.value =
       (uint64_t(atomicmarket_account_high) << 32) | atomicmarket_account_low;

   distribution_fund.value = eden_account.value + 1;
}

[[clang::export_name("allocateMemory")]] void* allocateMemory(uint32_t size)
{
   return malloc(size);
}
[[clang::export_name("freeMemory")]] void freeMemory(void* p)
{
   free(p);
}

std::variant<std::string, std::vector<char>> result;
[[clang::export_name("getResultSize")]] uint32_t getResultSize()
{
   return std::visit([](auto& data) { return data.size(); }, result);
}
[[clang::export_name("getResult")]] const char* getResult()
{
   return std::visit([](auto& data) { return data.data(); }, result);
}

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

uint64_t available_pk(const auto& table, const auto& first)
{
   auto& idx = table.template get<by_pk>();
   if (idx.empty())
      return first;
   return (--idx.end())->by_pk() + 1;
}

enum tables
{
   induction_table,
   member_table,
};

struct Induction;
constexpr const char InductionConnection_name[] = "InductionConnection";
constexpr const char InductionEdge_name[] = "InductionEdge";
using InductionConnection = clchain::Connection<
    clchain::ConnectionConfig<Induction, InductionConnection_name, InductionEdge_name>>;

struct MemberElection;
constexpr const char MemberElectionConnection_name[] = "MemberElectionConnection";
constexpr const char MemberElectionEdge_name[] = "MemberElectionEdge";
using MemberElectionConnection =
    clchain::Connection<clchain::ConnectionConfig<MemberElection,
                                                  MemberElectionConnection_name,
                                                  MemberElectionEdge_name>>;

using InductionEndorser = std::pair<eosio::name, bool>;

struct induction
{
   uint64_t id = 0;
   InductionEndorser inviter;
   eosio::name invitee;
   std::vector<InductionEndorser> witnesses;
   // eden::new_member_profile profile;
   std::string video;
   eosio::block_timestamp createdAt;
};
EOSIO_REFLECT(induction, id, inviter, invitee, witnesses, /* profile, */ video, createdAt)

using InductionCreatedAtKey = std::pair<eosio::block_timestamp, uint64_t>;

struct induction_object : public chainbase::object<induction_table, induction_object>
{
   CHAINBASE_DEFAULT_CONSTRUCTOR(induction_object)

   id_type id;
   induction induction;

   uint64_t by_pk() const { return induction.id; }
   std::pair<eosio::name, uint64_t> by_invitee() const { return {induction.invitee, induction.id}; }
   InductionCreatedAtKey by_createdAt() const { return {induction.createdAt, induction.id}; }
};
using induction_index = mic<induction_object,
                            ordered_by_id<induction_object>,
                            ordered_by_pk<induction_object>,
                            ordered_by_createdAt<induction_object>>;

using MemberCreatedAtKey = std::pair<eosio::block_timestamp, eosio::name>;

struct member
{
   eosio::name account;
   eosio::name inviter;
   std::vector<eosio::name> inductionWitnesses;
   // eden::new_member_profile profile;
   std::string inductionVideo;
   bool participating = false;
   eosio::block_timestamp createdAt;
};

struct member_object : public chainbase::object<member_table, member_object>
{
   CHAINBASE_DEFAULT_CONSTRUCTOR(member_object)

   id_type id;
   member member;

   eosio::name by_pk() const { return member.account; }
   MemberCreatedAtKey by_createdAt() const { return {member.createdAt, member.account}; }
};
using member_index = mic<member_object,
                         ordered_by_id<member_object>,
                         ordered_by_pk<member_object>,
                         ordered_by_createdAt<member_object>>;

struct database
{
   chainbase::database db;
   chainbase::generic_index<induction_index> inductions;
   chainbase::generic_index<member_index> members;

   database()
   {
      db.add_index(inductions);
      db.add_index(members);
   }
};
database db;

// TEMPLATE FUNCTIONS
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
// END TEMPLATE FUNCTIONS

const auto& get_status()
{
   auto& idx = db.status.get<by_id>();
   eosio::check(idx.size() == 1, "missing genesis action");
   return *idx.begin();
}

struct Member;
std::optional<Member> get_member(eosio::name account, bool allow_lsb = false);
std::vector<Member> get_members(const std::vector<eosio::name>& v);

struct Member
{
   eosio::name account;
   const member* member;

   // TODO: restore the following function
   // auto balance() const { return get_balance(account); }
   auto inviter() const { return get_member(member ? member->inviter : ""_n); }
   // TODO: restore the following function
   // const eden::new_member_profile* profile() const { return member ? &member->profile : nullptr; }
   const std::string* inductionVideo() const { return member ? &member->inductionVideo : nullptr; }
   bool participating() const { return member && member->participating; }
   eosio::block_timestamp createdAt() const { return member->createdAt; }
};
EOSIO_REFLECT2(Member,
               account,
               // balance,
               // inviter,
               // profile,
               inductionVideo,
               participating,
               createdAt)

std::optional<Member> get_member(eosio::name account, bool allow_lsb)
{
   if (auto* member_object = get_ptr<by_pk>(db.members, account))
      return Member{account, &member_object->member};
   else if (account.value && (!(account.value & 0x0f) || allow_lsb))
      return Member{account, nullptr};
   else
      return std::nullopt;
}

std::vector<Member> get_members(const std::vector<eosio::name>& v)
{
   std::vector<Member> result;
   result.reserve(v.size());
   for (auto n : v)
   {
      auto m = get_member(n);
      if (m)
         result.push_back(*m);
   }
   return result;
}

struct InductionEndorsingMemberStatus
{
   eosio::name endorserAccount;
   bool endorsed;

   InductionEndorsingMemberStatus(const InductionEndorser& endorser)
   {
      endorserAccount = endorser.first;
      endorsed = endorser.second;
   }

   auto member() const { return get_member(endorserAccount); }
};
EOSIO_REFLECT2(InductionEndorsingMemberStatus, member, endorsed)

struct Induction
{
   uint64_t id;
   const induction* induction;

   auto inviteeAccount() const { return induction->invitee; }
   auto inviter() const { return InductionEndorsingMemberStatus{induction->inviter}; }
   std::vector<InductionEndorsingMemberStatus> witnesses() const
   {
      std::vector<InductionEndorsingMemberStatus> endorsers;
      for (const auto& witness : induction->witnesses)
      {
         endorsers.push_back(InductionEndorsingMemberStatus{witness});
      }
      return endorsers;
   }
   auto profile() const { return induction->profile; }
   auto video() const { return induction->video; }
   eosio::block_timestamp createdAt() const { return induction->createdAt; }
};
EOSIO_REFLECT2(Induction, id, inviteeAccount, inviter, witnesses, profile, video, createdAt)

void add_genesis_member(const status& status, eosio::name member)
{
   db.inductions.emplace(
       [&](auto& obj)
       {
          obj.induction.id = available_pk(db.inductions, 1);
          obj.induction.inviter = {eden_account, false};
          obj.induction.invitee = member;
          for (auto witness : status.initialMembers)
             if (witness != member)
                obj.induction.witnesses.push_back({witness, false});
       });
}

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
   clear_table(db.inductions);
   clear_table(db.members);
}

// EVENTS HANDLER
void handle_event(const eden::migration_event& event)
{
   db.status.modify(get_status(),
                    [&](auto& status) { status.status.migrationIndex = event.index; });
}

void handle_event(const eden::election_event_schedule& event)
{
   db.status.modify(get_status(),
                    [&](auto& status)
                    {
                       status.status.nextElection = event.election_time;
                       status.status.electionThreshold = event.election_threshold;
                    });
   for (auto& member : db.members)
   {
      db.members.modify(member, [&](auto& member) { member.member.participating = false; });
   }
}

void handle_event(const eden::election_event_begin& event)
{
   db.elections.emplace([&](auto& election) { election.time = event.election_time; });
}

void handle_event(const eden::election_event_seeding& event)
{
   modify<by_pk>(db.elections, event.election_time,
                 [&](auto& election)
                 {
                    election.seeding = true;
                    election.seeding_start_time = event.start_time;
                    election.seeding_end_time = event.end_time;
                    election.seed = event.seed;
                 });
}

void handle_event(const eden::election_event_end_seeding& event)
{
   modify<by_pk>(db.elections, event.election_time,
                 [&](auto& election)
                 {
                    election.seeding = false;
                    election.seeding_start_time = std::nullopt;
                    election.seeding_end_time = std::nullopt;
                 });
}

void handle_event(const eden::election_event_config_summary& event)
{
   modify<by_pk>(db.elections, event.election_time,
                 [&](auto& election)
                 {
                    election.num_rounds = event.num_rounds;
                    election.num_participants = event.num_participants;
                 });
}

void handle_event(const eden::election_event_create_round& event)
{
   db.election_rounds.emplace(
       [&](auto& round)
       {
          round.election_time = event.election_time;
          round.round = event.round;
          round.num_participants = event.num_participants;
          round.num_groups = event.num_groups;
          round.requires_voting = event.requires_voting;
       });
}

void handle_event(const eden::election_event_create_group& event)
{
   eosio::check(!event.voters.empty(), "group has no voters");
   auto& group = db.election_groups.emplace(
       [&](auto& group)
       {
          group.election_time = event.election_time;
          group.round = event.round;
          group.first_member = *std::min_element(event.voters.begin(), event.voters.end());
       });
   for (auto voter : event.voters)
   {
      db.votes.emplace(
          [&](auto& vote)
          {
             vote.election_time = group.election_time;
             vote.round = event.round;
             vote.group_id = group.id._id;
             vote.voter = voter;
          });
   }
}

void handle_event(const eden::election_event_begin_round_voting& event)
{
   modify<by_round>(db.election_rounds, ElectionRoundKey{event.election_time, event.round},
                    [&](auto& round)
                    {
                       round.groups_available = true;
                       round.voting_started = true;
                       round.voting_begin = event.voting_begin;
                       round.voting_end = event.voting_end;
                    });
}

void handle_event(const eden::election_event_end_round_voting& event)
{
   modify<by_round>(db.election_rounds, ElectionRoundKey{event.election_time, event.round},
                    [&](auto& round) { round.voting_finished = true; });
}

void handle_event(const eden::election_event_report_group& event)
{
   eosio::check(!event.votes.empty(), "group has no votes");
   auto first_member = std::min_element(event.votes.begin(), event.votes.end(),
                                        [](auto& a, auto& b) { return a.voter < b.voter; })
                           ->voter;
   auto& group = get<by_pk>(db.election_groups,
                            ElectionGroupKey{event.election_time, event.round, first_member});
   db.election_groups.modify(group, [&](auto& group) { group.winner = event.winner; });
   for (auto& v : event.votes)
   {
      auto& vote = get<by_pk>(db.votes, std::tuple{v.voter, event.election_time, event.round});
      db.votes.modify(vote, [&](auto& vote) { vote.candidate = v.candidate; });
   }
}

void handle_event(const eden::election_event_end_round& event)
{
   modify<by_round>(db.election_rounds, ElectionRoundKey{event.election_time, event.round},
                    [&](auto& round) { round.results_available = true; });
}

void handle_event(const eden::election_event_end& event)
{
   modify<by_pk>(db.elections, event.election_time,
                 [&](auto& election)
                 {
                    election.results_available = true;
                    if (!election.num_rounds)
                       return;
                    auto& idx = db.election_groups.get<by_pk>();
                    auto it = idx.lower_bound(
                        ElectionGroupKey{event.election_time, *election.num_rounds - 1, ""_n});
                    if (it != idx.end() && it->election_time == event.election_time &&
                        it->round == *election.num_rounds - 1)
                       election.final_group_id = it->id._id;
                 });
   clear_participating();
}

void handle_event(const eden::distribution_event_schedule& event)
{
   db.distributions.emplace([&](auto& dist) { dist.time = event.distribution_time; });
}

void handle_event(const action_context& context, const eden::distribution_event_reserve& event)
{
   modify<by_pk>(db.distributions, event.distribution_time,
                 [&](auto& dist)
                 {
                    transfer_funds(context.block.timestamp, pool_account(event.pool),
                                   distribution_fund, event.target_amount,
                                   history_desc::reserve_distribution);
                    dist.target_amount = event.target_amount;
                 });
}

void handle_event(const eden::distribution_event_begin& event)
{
   modify<by_pk>(db.distributions, event.distribution_time,
                 [&](auto& dist)
                 {
                    dist.started = true;
                    dist.target_rank_distribution = event.rank_distribution;
                 });
}

void handle_event(const action_context& context,
                  const eden::distribution_event_return_excess& event)
{
   transfer_funds(context.block.timestamp, distribution_fund, pool_account(event.pool),
                  event.amount, history_desc::return_excess_distribution);
}

void handle_event(const action_context& context, const eden::distribution_event_return& event)
{
   transfer_funds(context.block.timestamp, distribution_fund, pool_account(event.pool),
                  event.amount, history_desc::return_distribution);
   modify<by_pk>(db.distribution_funds,
                 distribution_fund_key{event.owner, event.distribution_time, event.rank},
                 [&](auto& fund) { fund.current_balance -= event.amount; });
}

void handle_event(const eden::distribution_event_fund& event)
{
   db.distribution_funds.emplace(
       [&](auto& fund)
       {
          fund.owner = event.owner;
          fund.distribution_time = event.distribution_time;
          fund.rank = event.rank;
          fund.initial_balance = event.balance;
          fund.current_balance = event.balance;
       });
}

void handle_event(const eden::session_new_event& event)
{
   db.sessions.emplace(
       [&](auto& session)
       {
          session.eden_account = event.eden_account;
          session.key = event.key;
          session.expiration = event.expiration;
          session.description = event.description;
       });
}

void handle_event(const eden::session_del_event& event)
{
   remove_if_exists<by_pk>(db.sessions, SessionKey{event.eden_account, event.key});
}

void handle_event(const auto& event) {}

void handle_event(const action_context& context, const auto& event)
{
   handle_event(event);
}

void handle_event(const action_context& context, const eden::event& event)
{
   std::visit([&](const auto& event) { handle_event(context, event); }, event);
}

// END EVENTS HANDLER

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

void clean_data(const subchain::eosio_block& block)
{
   auto& idx = db.status.get<by_id>();
   if (idx.size() < 1)
      return;  // skip if genesis is not complete

   const auto& status = get_status();
   if (!status.status.active)
      return;  // skip if contract is not active

   // remove_expired_inductions(block.timestamp, status.status);
}

bool dispatch(eosio::name action_name, const action_context& context, eosio::input_stream& s);

void run(const action_context& context, eosio::input_stream& s)
{
   eden::run_auth auth;
   eosio::varuint32 num_verbs;
   from_bin(auth, s);
   from_bin(num_verbs, s);
   for (uint32_t i = 0; i < num_verbs.value; ++i)
   {
      auto index = eosio::varuint32_from_bin(s);
      auto name = eden::actions::get_name_for_session_action(index);
      if (!dispatch(name, context, s))
         // fatal because this throws off the rest of the stream
         eosio::check(false,
                      "run: verb not found: " + std::to_string(index) + " " + name.to_string());
   }
   eosio::check(!s.remaining(), "unpack error (extra data) within run");
}

bool dispatch(eosio::name action_name, const action_context& context, eosio::input_stream& s)
{
   if (action_name == "run"_n)
      run(context, s);
   else if (action_name == "clearall"_n)
      call(clearall, context, s);
   else if (action_name == "delsession"_n)
      call(delsession, context, s);
   else if (action_name == "withdraw"_n)
      call(withdraw, context, s);
   else if (action_name == "donate"_n)
      call(donate, context, s);
   else if (action_name == "transfer"_n)
      call(transfer, context, s);
   else if (action_name == "fundtransfer"_n)
      call(fundtransfer, context, s);
   else if (action_name == "usertransfer"_n)
      call(usertransfer, context, s);
   else if (action_name == "genesis"_n)
      call(genesis, context, s);
   else if (action_name == "addtogenesis"_n)
      call(addtogenesis, context, s);
   else if (action_name == "inductinit"_n)
      call(inductinit, context, s);
   else if (action_name == "inductprofil"_n)
      call(inductprofil, context, s);
   else if (action_name == "inductmeetin"_n)
      call(inductmeetin, context, s);
   else if (action_name == "inductvideo"_n)
      call(inductvideo, context, s);
   else if (action_name == "inductcancel"_n)
      call(inductcancel, context, s);
   else if (action_name == "inductdonate"_n)
      call(inductdonate, context, s);
   else if (action_name == "inductendors"_n)
      call(inductendors, context, s);
   else if (action_name == "resign"_n)
      call(resign, context, s);
   else if (action_name == "removemember"_n)
      call(removemember, context, s);
   else if (action_name == "rename"_n)
      call(rename, context, s);
   else if (action_name == "electopt"_n)
      call(electopt, context, s);
   else if (action_name == "electvote"_n)
      call(electvote, context, s);
   else if (action_name == "electmeeting"_n)
      call(electmeeting, context, s);
   else if (action_name == "electvideo"_n)
      call(electvideo, context, s);
   else if (action_name == "setencpubkey"_n)
      call(setencpubkey, context, s);
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
         if (action.firstReceiver == eden_account)
         {
            eosio::input_stream s(action.hexData.data);
            dispatch(action.name, context, s);
         }
         else if (action.firstReceiver == token_account && action.receiver == eden_account &&
                  action.name == "transfer"_n)
         {
            eosio::input_stream s(action.hexData.data);
            call(notify_transfer, context, s);
         }
         else if (action.firstReceiver == "eosio.null"_n && action.name == "eden.events"_n &&
                  action.creatorAction && action.creatorAction->receiver == eden_account)
         {
            // TODO: prevent abort, indicate what failed
            auto events = eosio::convert_from_bin<std::vector<eden::event>>(action.hexData.data);
            for (auto& event : events)
               handle_event(context, event);
         }
      }  // for(action)

      // garbage collection housekeeping
      clean_data(block);

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

subchain::block_log block_log;

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
   // printf("%s block: %d %d log: %d irreversible: %d db: %d-%d %s\n", block_log.status_str[status],
   //        (int)bi.eosioBlock.num, (int)bi.num, (int)block_log.blocks.size(),
   //        block_log.irreversible,  //
   //        (int)db.db.undo_stack_revision_range().first,
   //        (int)db.db.undo_stack_revision_range().second,  //
   //        to_string(bi.eosioBlock.id).c_str());
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
   // printf("%d blocks processed, %d blocks now in log\n", (int)eosio_blocks.size(),
   //        (int)block_log.blocks.size());
   // for (auto& b : block_log.blocks)
   //    printf("%d\n", (int)b->num);
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

struct Query
{
   subchain::BlockLog blockLog;

   // std::optional<Status> status() const
   // {
   //    auto& idx = db.status.get<by_id>();
   //    if (idx.size() != 1)
   //       return std::nullopt;
   //    return Status{&idx.begin()->status};
   // }

   MemberConnection members(std::optional<eosio::name> gt,
                            std::optional<eosio::name> ge,
                            std::optional<eosio::name> lt,
                            std::optional<eosio::name> le,
                            std::optional<uint32_t> first,
                            std::optional<uint32_t> last,
                            std::optional<std::string> before,
                            std::optional<std::string> after) const
   {
      return clchain::make_connection<MemberConnection, eosio::name>(
          gt, ge, lt, le, first, last, before, after,    //
          db.members.get<by_pk>(),                       //
          [](auto& obj) { return obj.member.account; },  //
          [](auto& obj) {
             return Member{obj.member.account, &obj.member};
          },
          [](auto& members, auto key) { return members.lower_bound(key); },
          [](auto& members, auto key) { return members.upper_bound(key); });
   }

   MemberConnection membersByCreatedAt(std::optional<eosio::block_timestamp> gt,
                                       std::optional<eosio::block_timestamp> ge,
                                       std::optional<eosio::block_timestamp> lt,
                                       std::optional<eosio::block_timestamp> le,
                                       std::optional<uint32_t> first,
                                       std::optional<uint32_t> last,
                                       std::optional<std::string> before,
                                       std::optional<std::string> after) const
   {
      return clchain::make_connection<MemberConnection, MemberCreatedAtKey>(
          gt ? std::optional{MemberCreatedAtKey{*gt, account_max}}  //
             : std::nullopt,                                        //
          ge ? std::optional{MemberCreatedAtKey{*ge, account_min}}  //
             : std::nullopt,                                        //
          lt ? std::optional{MemberCreatedAtKey{*lt, account_min}}  //
             : std::nullopt,                                        //
          le ? std::optional{MemberCreatedAtKey{*le, account_max}}  //
             : std::nullopt,                                        //
          first, last, before, after,                               //
          db.members.get<by_createdAt>(),                           //
          [](auto& obj) { return obj.by_createdAt(); },             //
          [](auto& obj) {
             return Member{obj.member.account, &obj.member};
          },
          [](auto& members, auto key) { return members.lower_bound(key); },
          [](auto& members, auto key) { return members.upper_bound(key); });
   }

   InductionConnection inductions(std::optional<uint64_t> gt,
                                  std::optional<uint64_t> ge,
                                  std::optional<uint64_t> lt,
                                  std::optional<uint64_t> le,
                                  std::optional<uint32_t> first,
                                  std::optional<uint32_t> last,
                                  std::optional<std::string> before,
                                  std::optional<std::string> after) const
   {
      return clchain::make_connection<InductionConnection, uint64_t>(
          gt, ge, lt, le, first, last, before, after,  //
          db.inductions.get<by_pk>(),                  //
          [](auto& obj) { return obj.induction.id; },  //
          [](auto& obj) {
             return Induction{obj.induction.id, &obj.induction};
          },
          [](auto& inductions, auto key) { return inductions.lower_bound(key); },
          [](auto& inductions, auto key) { return inductions.upper_bound(key); });
   }

   InductionConnection inductionsByCreatedAt(std::optional<eosio::block_timestamp> gt,
                                             std::optional<eosio::block_timestamp> ge,
                                             std::optional<eosio::block_timestamp> lt,
                                             std::optional<eosio::block_timestamp> le,
                                             std::optional<uint32_t> first,
                                             std::optional<uint32_t> last,
                                             std::optional<std::string> before,
                                             std::optional<std::string> after) const
   {
      return clchain::make_connection<InductionConnection, InductionCreatedAtKey>(
          gt ? std::optional{InductionCreatedAtKey{*gt, ~uint64_t(0)}}  //
             : std::nullopt,                                            //
          ge ? std::optional{InductionCreatedAtKey{*ge, 0}}             //
             : std::nullopt,                                            //
          lt ? std::optional{InductionCreatedAtKey{*lt, 0}}             //
             : std::nullopt,                                            //
          le ? std::optional{InductionCreatedAtKey{*le, ~uint64_t(0)}}  //
             : std::nullopt,                                            //
          first, last, before, after,                                   //
          db.inductions.get<by_createdAt>(),                            //
          [](auto& obj) { return obj.by_createdAt(); },                 //
          [](auto& obj) {
             return Induction{obj.induction.id, &obj.induction};
          },
          [](auto& inductions, auto key) { return inductions.lower_bound(key); },
          [](auto& inductions, auto key) { return inductions.upper_bound(key); });
   }
};
EOSIO_REFLECT2(
    Query,
    blockLog,
    method(members, "gt", "ge", "lt", "le", "first", "last", "before", "after"),
    method(membersByCreatedAt, "gt", "ge", "lt", "le", "first", "last", "before", "after"),
    method(inductions, "gt", "ge", "lt", "le", "first", "last", "before", "after"),
    method(inductionsByCreatedAt, "gt", "ge", "lt", "le", "first", "last", "before", "after"))

auto schema = clchain::get_gql_schema<Query>();
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
