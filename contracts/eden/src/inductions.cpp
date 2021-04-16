#include <inductions.hpp>

namespace eden
{
   void inductions::initialize_induction(uint64_t id,
                                         eosio::name inviter,
                                         eosio::name invitee,
                                         const std::vector<eosio::name>& witnesses)
   {
      check_new_induction(invitee, inviter);

      induction_tb.emplace(contract, [&](auto& row) {
         row.id = id;
         row.inviter = inviter;
         row.invitee = invitee;
         row.witnesses = witnesses;
         row.endorsements = {};
         row.created_at = eosio::current_block_time();
         row.video = "";
         row.new_member_profile = {};
      });
   }

   void inductions::set_profile(eosio::name inviter,
                                eosio::name invitee,
                                const new_member_profile& new_member_profile)
   {
      validate_profile(new_member_profile);

      auto induction = get_valid_induction(invitee, inviter);

      induction_tb.modify(induction_tb.iterator_to(induction), eosio::same_payer, [&](auto& row) {
         row.new_member_profile = new_member_profile;
         row.endorsements = {};
      });
   }

   void inductions::check_new_induction(eosio::name invitee, eosio::name inviter) const
   {
      auto invitee_index = induction_tb.get_index<"byinvitee"_n>();

      auto invitee_key = combine_names(invitee, inviter);
      auto itr = invitee_index.find(invitee_key);

      eosio::check(itr == invitee_index.end(),
                   "induction for this invitation is already in progress");
   }

   induction inductions::get_valid_induction(eosio::name invitee, eosio::name inviter) const
   {
      auto invitee_index = induction_tb.get_index<"byinvitee"_n>();
      auto invitee_key = combine_names(invitee, inviter);

      auto induction = invitee_index.get(invitee_key);
      auto induction_lifetime = eosio::current_time_point() - induction.created_at.to_time_point();
      eosio::check(induction_lifetime.to_seconds() <= induction_expiration_secs,
                   "induction has expired");

      return induction;
   }

   void inductions::validate_profile(const new_member_profile& new_member_profile) const
   {
      eosio::check(!new_member_profile.name.empty(), "new member profile name is empty");
      eosio::check(!new_member_profile.img.empty(), "new member profile img is empty");
      eosio::check(!new_member_profile.bio.empty(), "new member profile bio is empty");
      // TODO: add more checks (valid ipfs img, bio and name minimum length)
   }

}  // namespace eden