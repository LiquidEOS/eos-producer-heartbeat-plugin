/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/producer_heartbeat_plugin/producer_heartbeat_plugin.hpp>
#include <eosio/chain/types.hpp>
#include <boost/asio/steady_timer.hpp>
#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/abi_serializer.hpp>


#include <eosio/utilities/common.hpp>

// HACK TO EXPOSE LOGGER MAP

namespace fc {
   extern std::unordered_map<std::string,logger>& get_logger_map();
}


namespace eosio {
   using namespace eosio::chain;
   
   static appbase::abstract_plugin& _template_plugin = app().register_plugin<producer_heartbeat_plugin>();



class producer_heartbeat_plugin_impl {
   public:
      unique_ptr<boost::asio::steady_timer> timer;
      boost::asio::steady_timer::duration timer_period;
      account_name heartbeat_contract = "";
      std::string heartbeat_permission = "";
      account_name producer_name;
      fc::crypto::private_key _heartbeat_private_key;
      chain::public_key_type _heartbeat_public_key;
      
      void send_heartbeat_transaction(){
            auto& plugin = app().get_plugin<chain_plugin>();
            auto actor_blacklist_hash = 0;
            auto chainid = plugin.get_chain_id();
            auto abi_serializer_max_time = plugin.get_abi_serializer_max_time();
   
            controller& cc = plugin.chain();
            auto* account_obj = cc.db().find<account_object, by_name>(heartbeat_contract);
            if(account_obj == nullptr)
               return;
            abi_def abi;
            if (!abi_serializer::to_abi(account_obj->abi, abi)) 
               return;
            // auto abi = account_obj->get_abi();
            abi_serializer eosio_serializer(abi, abi_serializer_max_time);
            signed_transaction trx;
            action act;
            act.account = heartbeat_contract;
            act.name = N(heartbeat);
            act.authorization = vector<permission_level>{{producer_name,heartbeat_permission}};
            auto metadata_obj = mutable_variant_object()
               ("server_version", eosio::utilities::common::itoh(static_cast<uint32_t>(app().version())))
               ("actor_blacklist_hash", eosio::utilities::common::itoh(actor_blacklist_hash))
               ("head_block_num",  cc.fork_db_head_block_num());
            auto metadata_json = fc::json::to_string( metadata_obj );
            act.data = eosio_serializer.variant_to_binary("heartbeat",mutable_variant_object()
               ("_user", producer_name)
               ("_metadata_json", metadata_json), 
               abi_serializer_max_time);
            trx.actions.push_back(act);
            
            trx.expiration = cc.head_block_time() + fc::seconds(30);
            trx.set_reference_block(cc.head_block_id());
            trx.sign(_heartbeat_private_key, chainid);
            auto trace = cc.push_transaction( std::make_shared<transaction_metadata>(trx) , trx.expiration);
            if (trace->except) {
               elog("heartbeat failed: ${err}", ("err", trace->except->to_detail_string()));
            } else {
               dlog("heartbeat success");
            }

      }
      void start_timer( ) {
         timer->expires_from_now(timer_period);
         timer->async_wait( [this](boost::system::error_code ec) {
               start_timer();
               if(!ec) {
                  try{
                     send_heartbeat_transaction();
                  }
                  FC_LOG_AND_DROP();
                  
               }
               else {
                  elog( "Error from connection check monitor: ${m}",( "m", ec.message()));
               }
            });
      }      
};

producer_heartbeat_plugin::producer_heartbeat_plugin():my(new producer_heartbeat_plugin_impl()){
   my->timer.reset(new boost::asio::steady_timer( app().get_io_service()));

}
producer_heartbeat_plugin::~producer_heartbeat_plugin(){}

void producer_heartbeat_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("heartbeat-period", bpo::value<int>()->default_value(300),
          "Heartbeat transaction period in seconds")
         ("heartbeat-signature-provider", bpo::value<string>()->default_value("X=KEY:Y"),
          "Heartbeat key provider")
         ("heartbeat-contract", bpo::value<string>()->default_value("heartbeat"),
          "Heartbeat Contract")
         ("heartbeat-permission", bpo::value<string>()->default_value("heartbeat"),
          "Heartbeat permission name")          
         ;
}

#define LOAD_VALUE_SET(options, name, container, type) \
if( options.count(name) ) { \
   const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
   std::copy(ops.begin(), ops.end(), std::inserter(container, container.end())); \
}



void producer_heartbeat_plugin::plugin_initialize(const variables_map& options) {
   try {
      if( options.count( "heartbeat-period" )) {
         // Handle the option
         my->timer_period = std::chrono::seconds( options.at( "heartbeat-period" ).as<int>());
      }
      if( options.count( "heartbeat-contract" )) {
         // Handle the option
         my->heartbeat_contract = options.at( "heartbeat-contract" ).as<string>();
      }
      if( options.count( "heartbeat-permission" )) {
         // Handle the option
         my->heartbeat_permission = options.at( "heartbeat-permission" ).as<string>();
      }
      if(options.count("producer-name")){
         const std::vector<std::string>& ops = options["producer-name"].as<std::vector<std::string>>();
         my->producer_name = ops[0];
      }
      if( options.count("heartbeat-signature-provider") ) {
            auto key_spec_pair = options["heartbeat-signature-provider"].as<std::string>();
            
            try {
               auto delim = key_spec_pair.find("=");
               EOS_ASSERT(delim != std::string::npos, eosio::chain::plugin_config_exception, "Missing \"=\" in the key spec pair");
               auto pub_key_str = key_spec_pair.substr(0, delim);
               auto spec_str = key_spec_pair.substr(delim + 1);
   
               auto spec_delim = spec_str.find(":");
               EOS_ASSERT(spec_delim != std::string::npos, eosio::chain::plugin_config_exception, "Missing \":\" in the key spec pair");
               auto spec_type_str = spec_str.substr(0, spec_delim);
               auto spec_data = spec_str.substr(spec_delim + 1);
   
               auto pubkey = public_key_type(pub_key_str);
               
               
               if (spec_type_str == "KEY") {
                  ilog("Loaded heartbeat key");
                  my->_heartbeat_private_key = fc::crypto::private_key(spec_data);
                  my->_heartbeat_public_key = pubkey;   
               } else if (spec_type_str == "KEOSD") {
                  elog("KEOSD heartbeat key not supported");
                  // not supported
               }
   
            } catch (...) {
               elog("Malformed signature provider: \"${val}\", ignoring!", ("val", key_spec_pair));
            }
         }
         
   }
   FC_LOG_AND_RETHROW()
}

void producer_heartbeat_plugin::plugin_startup() {
   my->start_timer();
   ilog("producer heartbeat plugin:  plugin_startup() begin");
}

void producer_heartbeat_plugin::plugin_shutdown() {
   // OK, that's enough magic
}

}
