#include <accounts.hpp>
#include <boot/boot.hpp>
#include <eden-atomicassets.hpp>
#include <eden.hpp>
#include <eosio/tester.hpp>
#include <members.hpp>
#include <token/token.hpp>

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

using namespace eosio;
using namespace std::literals;
namespace atomicassets = eden::atomicassets;
using atomicassets::attribute_map;
namespace actions = eden::actions;
using user_context = test_chain::user_context;
using eden::accounts;
using eden::members;

void chain_setup(test_chain& t)
{
   t.set_code("eosio"_n, "boot.wasm");
   t.as("eosio"_n).act<boot::actions::boot>();
   t.start_block();  // preactivate feature activates protocol features at the start of a block
   t.set_code("eosio"_n, "bios.wasm");
}

void token_setup(test_chain& t)
{
   t.create_code_account("eosio.token"_n);
   t.set_code("eosio.token"_n, "token.wasm");
   t.as("eosio.token"_n).act<token::actions::create>("eosio.token"_n, s2a("1000000.0000 EOS"));
   t.as("eosio.token"_n).act<token::actions::issue>("eosio.token"_n, s2a("1000000.0000 EOS"), "");
   t.as("eosio.token"_n).act<token::actions::create>("eosio.token"_n, s2a("1000000.0000 OTHER"));
   t.as("eosio.token"_n).act<token::actions::issue>("eosio.token"_n, s2a("1000000.0000 OTHER"), "");
}

void atomicmarket_setup(test_chain& t)
{
   t.create_code_account("atomicmarket"_n);
   //t.set_code("atomicmarket"_n, "atomicmarket.wasm");
}

void atomicassets_setup(test_chain& t)
{
   t.create_code_account("atomicassets"_n);
   t.set_code("atomicassets"_n, "atomicassets.wasm");
   t.as("atomicassets"_n).act<atomicassets::actions::init>();
}

void eden_setup(test_chain& t)
{
   token_setup(t);
   atomicassets_setup(t);
   atomicmarket_setup(t);
   t.create_code_account("eden.gm"_n);
   t.set_code("eden.gm"_n, "eden.wasm");
}

auto get_token_balance(eosio::name owner)
{
   return token::contract::get_balance("eosio.token"_n, "alice"_n, symbol_code{"EOS"});
}

auto get_eden_account(eosio::name owner)
{
   return accounts{"eden.gm"_n}.get_account(owner);
}

const eden::member& get_eden_membership(eosio::name account)
{
   return members{"eden.gm"_n}.get_member(account);
}

static const eden::new_member_profile alice_profile{
    "Alice", "Qmb7WmZiSDXss5HfuKfoSf6jxTDrHzr8AoAUDeDMLNDuws",
    "Alice was beginning to get very tired of sitting by her sister on the bank, and of "
    "having nothing to do: once or twice she had peeped into the book her sister was "
    "reading, but it had no pictures or conversations in it, \"and what is the use of a "
    "book,\" thought Alice \"without pictures or conversations?\"",
    "{\"blog\":\"alice.example.com\"}"};

static const eden::new_member_profile pip_profile{
    "Philip Pirrip", "Qmb7WmZiSDXss5HfuKfoSf6jxTDrHzr8AoAUDeDMLNDuws",
    "My father's family name being Pirrip and my Christian name Phillip, my infant "
    "tongue could make of both names nothing longer or more explicit than Pip.  So, I "
    "called myself Pip and came to be called Pip.",
    "{\"blog\":\"pip.example.com\"}"};

static const eden::new_member_profile egeon_profile{
    "Egeon", "Qmb7WmZiSDXss5HfuKfoSf6jxTDrHzr8AoAUDeDMLNDuws",
    "In Syracusa was I born, and wed\nUnto a woman happy but for me,\nAnd by me, had not "
    "our hap been bad.\nWith her I liv'd in joy; our wealth increas'd\nBy prosperous "
    "voyages I often made\nTo Epidamnum, till my factor's death,",
    "{\"blog\":\"egeon.example.com\"}"};

static const eden::new_member_profile bertie_profile{
    "Bertie Wooster", "Qmb7WmZiSDXss5HfuKfoSf6jxTDrHzr8AoAUDeDMLNDuws",
    "I'm a bit short on brain myself; the old bean would appear to have been constructed more for "
    "ornament than for use, don't you know; but give me five minutes to talk the thing over with "
    "Jeeves, and I'm game to advise any one about anything.",
    "{\"blog\":\"bertie.example.com\"}"};

static const eden::new_member_profile ahab_profile{
    "Captain Ahab", "Qmb7WmZiSDXss5HfuKfoSf6jxTDrHzr8AoAUDeDMLNDuws",
    "Ahab's been in colleges, as well as 'mong the cannibals; been used to deeper wonders than the "
    "waves; fixed his fiery lance in mightier, stranger foes than whales.  His lance! aye, the "
    "keenest and the surest that out of all our isle"
    "{\"blog\":\"ahab.example.com\"}"};

struct eden_tester
{
   test_chain chain;
   user_context eosio_token = chain.as("eosio.token"_n);
   user_context eden_gm = chain.as("eden.gm"_n);
   user_context alice = chain.as("alice"_n);
   user_context pip = chain.as("pip"_n);
   user_context egeon = chain.as("egeon"_n);
   user_context bertie = chain.as("bertie"_n);
   user_context ahab = chain.as("ahab"_n);

   eden_tester()
   {
      chain_setup(chain);
      eden_setup(chain);
      for (auto account : {"alice"_n, "pip"_n, "egeon"_n, "bertie"_n, "ahab"_n})
      {
         chain.create_account(account);
         chain.as("eosio.token"_n)
             .act<token::actions::transfer>("eosio.token"_n, account, s2a("1000.0000 EOS"), "memo");
         chain.as("eosio.token"_n)
             .act<token::actions::transfer>("eosio.token"_n, account, s2a("1000.0000 OTHER"),
                                            "memo");
      }
   }
   void genesis()
   {
      eden_gm.act<actions::genesis>("Eden", eosio::symbol("EOS", 4), s2a("10.0000 EOS"),
                                    std::vector{"alice"_n, "pip"_n, "egeon"_n},
                                    "QmTYqoPYf7DiVebTnvwwFdTgsYXg2RnuPrt8uddjfW2kHS",
                                    attribute_map{}, s2a("1.0000 EOS"), 7 * 24 * 60 * 60, "");

      alice.act<actions::inductprofil>(1, alice_profile);
      pip.act<actions::inductprofil>(2, pip_profile);
      egeon.act<actions::inductprofil>(3, egeon_profile);

      alice.act<token::actions::transfer>("alice"_n, "eden.gm"_n, s2a("100.0000 EOS"), "memo");
      pip.act<token::actions::transfer>("pip"_n, "eden.gm"_n, s2a("10.0000 EOS"), "memo");
      egeon.act<token::actions::transfer>("egeon"_n, "eden.gm"_n, s2a("10.0000 EOS"), "memo");

      alice.act<actions::inductdonate>("alice"_n, 1, s2a("10.0000 EOS"));
      pip.act<actions::inductdonate>("pip"_n, 2, s2a("10.0000 EOS"));
      egeon.act<actions::inductdonate>("egeon"_n, 3, s2a("10.0000 EOS"));
   }
};

TEST_CASE("genesis NFT pre-setup")
{
   eden_tester t;

   t.eden_gm.act<atomicassets::actions::createcol>(
       "eden.gm"_n, "eden.gm"_n, true, std::vector{"eden.gm"_n}, std::vector{"eden.gm"_n}, 0.05,
       atomicassets::attribute_map{});
   std::vector<atomicassets::format> schema{{"account", "string"}, {"name", "string"},
                                            {"img", "ipfs"},       {"bio", "string"},
                                            {"social", "string"},  {"video", "ipfs"}};
   t.eden_gm.act<atomicassets::actions::createschema>("eden.gm"_n, "eden.gm"_n, eden::schema_name,
                                                      schema);

   t.eden_gm.act<actions::genesis>("Eden", eosio::symbol("EOS", 4), s2a("10.0000 EOS"),
                                   std::vector{"alice"_n, "pip"_n, "egeon"_n},
                                   "QmTYqoPYf7DiVebTnvwwFdTgsYXg2RnuPrt8uddjfW2kHS",
                                   attribute_map{}, s2a("1.0000 EOS"), 7 * 24 * 60 * 60, "");
}

TEST_CASE("genesis NFT pre-setup with incorrect schema")
{
   eden_tester t;

   t.eden_gm.act<atomicassets::actions::createcol>(
       "eden.gm"_n, "eden.gm"_n, true, std::vector{"eden.gm"_n}, std::vector{"eden.gm"_n}, 0.05,
       atomicassets::attribute_map{});
   std::vector<atomicassets::format> schema{{"account", "uint64"}, {"name", "string"}};
   t.eden_gm.act<atomicassets::actions::createschema>("eden.gm"_n, "eden.gm"_n, eden::schema_name,
                                                      schema);

   auto trace = t.eden_gm.trace<actions::genesis>(
       "Eden", eosio::symbol("EOS", 4), s2a("10.0000 EOS"),
       std::vector{"alice"_n, "pip"_n, "egeon"_n}, "QmTYqoPYf7DiVebTnvwwFdTgsYXg2RnuPrt8uddjfW2kHS",
       attribute_map{}, s2a("1.0000 EOS"), 7 * 24 * 60 * 60, "");
   expect(trace, "there already is an attribute with the same name");
}

TEST_CASE("genesis NFT pre-setup with compatible schema")
{
   eden_tester t;

   t.eden_gm.act<atomicassets::actions::createcol>(
       "eden.gm"_n, "eden.gm"_n, true, std::vector{"eden.gm"_n}, std::vector{"eden.gm"_n}, 0.05,
       atomicassets::attribute_map{});
   std::vector<atomicassets::format> schema{{"social", "string"},
                                            {"pi", "float"},
                                            {"video", "ipfs"},
                                            {"account", "string"},
                                            {"name", "string"}};
   t.eden_gm.act<atomicassets::actions::createschema>("eden.gm"_n, "eden.gm"_n, eden::schema_name,
                                                      schema);

   t.eden_gm.act<actions::genesis>("Eden", eosio::symbol("EOS", 4), s2a("10.0000 EOS"),
                                   std::vector{"alice"_n, "pip"_n, "egeon"_n},
                                   "QmTYqoPYf7DiVebTnvwwFdTgsYXg2RnuPrt8uddjfW2kHS",
                                   attribute_map{}, s2a("1.0000 EOS"), 7 * 24 * 60 * 60, "");
}

TEST_CASE("genesis")
{
   eden_tester t;
   t.eden_gm.act<actions::genesis>("Eden", eosio::symbol("EOS", 4), s2a("10.0000 EOS"),
                                   std::vector{"alice"_n, "pip"_n, "egeon"_n},
                                   "QmTYqoPYf7DiVebTnvwwFdTgsYXg2RnuPrt8uddjfW2kHS",
                                   attribute_map{}, s2a("1.0000 EOS"), 7 * 24 * 60 * 60, "");

   CHECK(get_eden_membership("alice"_n).status() == eden::member_status::pending_membership);
   CHECK(get_eden_membership("pip"_n).status() == eden::member_status::pending_membership);
   CHECK(get_eden_membership("egeon"_n).status() == eden::member_status::pending_membership);

   t.alice.act<actions::inductprofil>(1, alice_profile);
   t.pip.act<actions::inductprofil>(2, pip_profile);
   t.egeon.act<actions::inductprofil>(3, egeon_profile);

   CHECK(get_eden_account("alice"_n) == std::nullopt);
   CHECK(get_token_balance("alice"_n) == s2a("1000.0000 EOS"));

   t.alice.act<token::actions::transfer>("alice"_n, "eden.gm"_n, s2a("100.0000 EOS"), "memo");
   t.pip.act<token::actions::transfer>("pip"_n, "eden.gm"_n, s2a("10.0000 EOS"), "memo");
   t.egeon.act<token::actions::transfer>("egeon"_n, "eden.gm"_n, s2a("10.0000 EOS"), "memo");

   CHECK(get_eden_account("alice"_n) != std::nullopt);
   CHECK(get_eden_account("alice"_n)->balance() == s2a("100.0000 EOS"));
   CHECK(get_token_balance("alice"_n) == s2a("900.0000 EOS"));

   t.alice.act<actions::inductdonate>("alice"_n, 1, s2a("10.0000 EOS"));
   t.pip.act<actions::inductdonate>("pip"_n, 2, s2a("10.0000 EOS"));
   t.egeon.act<actions::inductdonate>("egeon"_n, 3, s2a("10.0000 EOS"));

   CHECK(get_eden_account("alice"_n) != std::nullopt);
   CHECK(get_eden_account("alice"_n)->balance() == s2a("90.0000 EOS"));
   CHECK(get_token_balance("alice"_n) == s2a("900.0000 EOS"));

   CHECK(get_eden_membership("alice"_n).status() == eden::member_status::active_member);
   CHECK(get_eden_membership("pip"_n).status() == eden::member_status::active_member);
   CHECK(get_eden_membership("egeon"_n).status() == eden::member_status::active_member);

   eden::tester_clear_global_singleton();
   eden::globals globals("eden.gm"_n);
   CHECK(globals.get().stage == eden::contract_stage::active);
}

TEST_CASE("induction")
{
   eden_tester t;
   t.genesis();

   t.alice.act<actions::inductinit>(4, "alice"_n, "bertie"_n, std::vector{"pip"_n, "egeon"_n});
   t.bertie.act<token::actions::transfer>("bertie"_n, "eden.gm"_n, s2a("10.0000 EOS"), "memo");
   CHECK(get_eden_membership("bertie"_n).status() == eden::member_status::pending_membership);

   // cannot endorse before video and profile are set
   auto blank_hash_data =
       eosio::convert_to_bin(std::tuple(std::string{}, eden::new_member_profile{}));
   expect(t.alice.trace<actions::inductendorse>(
              "alice"_n, 4, eosio::sha256(blank_hash_data.data(), blank_hash_data.size())),
          "not set");

   expect(t.bertie.trace<actions::inductinit>(5, "bertie"_n, "ahab"_n,
                                              std::vector{"pip"_n, "egeon"_n}),
          "inactive member bertie");

   std::string bertie_video = "QmTYqoPYf7DiVebTnvwwFdTgsYXg2RnuPrt8uddjfW2kHS";
   t.bertie.act<actions::inductprofil>(4, bertie_profile);
   t.alice.act<actions::inductvideo>("alice"_n, 4, bertie_video);  // Can be inviter or witness

   expect(t.bertie.trace<actions::inductdonate>("bertie"_n, 4, s2a("10.0000 EOS")),
          "induction is not fully endorsed");

   auto hash_data = eosio::convert_to_bin(std::tuple(bertie_video, bertie_profile));
   auto induction_hash = eosio::sha256(hash_data.data(), hash_data.size());
   expect(t.bertie.trace<actions::inductendorse>("alice"_n, 4, induction_hash),
          "missing authority of alice");
   expect(t.bertie.trace<actions::inductendorse>("bertie"_n, 4, induction_hash),
          "Induction can only be endorsed by inviter or a witness");
   expect(t.alice.trace<actions::inductendorse>(
              "alice"_n, 4, eosio::sha256(hash_data.data(), hash_data.size() - 1)),
          "Outdated endorsement");

   auto endorse_all = [&] {
      t.alice.act<actions::inductendorse>("alice"_n, 4, induction_hash);
      t.pip.act<actions::inductendorse>("pip"_n, 4, induction_hash);
      t.egeon.act<actions::inductendorse>("egeon"_n, 4, induction_hash);
   };
   endorse_all();
   t.chain.start_block();
   expect(t.alice.trace<actions::inductendorse>("alice"_n, 4, induction_hash), "Already endorsed");

   // changing the profile resets endorsements
   t.bertie.act<actions::inductprofil>(4, bertie_profile);
   expect(t.bertie.trace<actions::inductdonate>("bertie"_n, 4, s2a("10.0000 EOS")),
          "induction is not fully endorsed");
   t.chain.start_block();
   endorse_all();

   // changing the video resets endorsements
   t.pip.act<actions::inductvideo>("pip"_n, 4, bertie_video);
   expect(t.bertie.trace<actions::inductdonate>("bertie"_n, 4, s2a("10.0000 EOS")),
          "induction is not fully endorsed");
   t.chain.start_block();
   endorse_all();

   t.bertie.act<actions::inductdonate>("bertie"_n, 4, s2a("10.0000 EOS"));
   CHECK(get_eden_membership("bertie"_n).status() == eden::member_status::active_member);
}

TEST_CASE("deposit and spend")
{
   eden_tester t;
   t.eden_gm.act<actions::genesis>("Eden", eosio::symbol("EOS", 4), s2a("10.0000 EOS"),
                                   std::vector{"alice"_n, "pip"_n, "egeon"_n},
                                   "QmTYqoPYf7DiVebTnvwwFdTgsYXg2RnuPrt8uddjfW2kHS",
                                   attribute_map{}, s2a("1.0000 EOS"), 7 * 24 * 60 * 60, "");
   expect(t.alice.trace<token::actions::transfer>("alice"_n, "eden.gm"_n, s2a("10.0000 OTHER"),
                                                  "memo"),
          "token must be a valid 4,EOS");
   expect(
       t.alice.trace<token::actions::transfer>("alice"_n, "eden.gm"_n, s2a("9.9999 EOS"), "memo"),
       "insufficient deposit to open an account");
   CHECK(get_eden_account("alice"_n) == std::nullopt);
   t.alice.act<token::actions::transfer>("alice"_n, "eden.gm"_n, s2a("10.0000 EOS"), "memo");
   CHECK(get_eden_account("alice"_n) != std::nullopt);
   CHECK(get_eden_account("alice"_n)->balance() == s2a("10.0000 EOS"));
   CHECK(get_token_balance("alice"_n) == s2a("990.0000 EOS"));

   expect(t.pip.trace<actions::withdraw>("pip"_n, s2a("10.0000 EOS")), "insufficient balance");
   expect(t.pip.trace<actions::withdraw>("alice"_n, s2a("10.0000 EOS")),
          "missing authority of alice");
   expect(t.alice.trace<actions::withdraw>("alice"_n, s2a("10.0001 EOS")), "insufficient balance");
   CHECK(get_eden_account("alice"_n)->balance() == s2a("10.0000 EOS"));
   CHECK(get_token_balance("alice"_n) == s2a("990.0000 EOS"));
   t.alice.act<actions::withdraw>("alice"_n, s2a("4.0000 EOS"));
   CHECK(get_eden_account("alice"_n)->balance() == s2a("6.0000 EOS"));
   CHECK(get_token_balance("alice"_n) == s2a("994.0000 EOS"));
   t.alice.act<actions::withdraw>("alice"_n, s2a("6.0000 EOS"));
   CHECK(get_eden_account("alice"_n) == std::nullopt);
   CHECK(get_token_balance("alice"_n) == s2a("1000.0000 EOS"));
}
