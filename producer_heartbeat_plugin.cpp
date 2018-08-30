/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/producer_heartbeat_plugin/producer_heartbeat_plugin.hpp>
#include <boost/asio/steady_timer.hpp>
#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/abi_serializer.hpp>


// HACK TO EXPOSE LOGGER MAP

namespace fc {
   extern std::unordered_map<std::string,logger>& get_logger_map();
}

const fc::string logger_name("producer_plugin");
fc::logger _log;

namespace eosio {
   static appbase::abstract_plugin& _template_plugin = app().register_plugin<producer_heartbeat_plugin>();



class producer_heartbeat_plugin_impl {
   public:
      unique_ptr<boost::asio::steady_timer> timer;
      boost::asio::steady_timer::duration timer_period;
      fc::microseconds _keosd_provider_timeout_us;
      std:string heartbeat_contract;
      std:string producer_name;
      fc::crypto::private_key _hearbeat_private_key;
      chain::public_key_type _hearbeat_public_key;
      
      void send_heartbeat_transaction(){
            chain::chain_id_type chainid;
            auto& plugin = _app.get_plugin<chain_plugin>();
            auto actor_blacklist_hash = 0;
            plugin.get_chain_id(chainid);
            controller& cc = plugin.chain();
            signed_transaction trx;
            action act;
            act.account = heartbeat_contract;
            act.name = N(hearbeat);
            act.authorization = vector<permission_level>{{producer_name,"hearbeat"}};
            act.data = eosio_token_serializer.variant_to_binary("create",mutable_variant_object()
               ("server_version", eosio::utilities::common::itoh(static_cast<uint32_t>(app().version())))
               ("actor_blacklist_hash", eosio::utilities::common::itoh(actor_blacklist_hash))
               ("head_block_num",  cc.fork_db_head_block_num()), 
               abi_serializer_max_time);
            trx.actions.push_back(act);
            
            trx.expiration = cc.head_block_time() + fc::seconds(30);
            trx.set_reference_block(cc.head_block_id());
            trx.sign(_hearbeat_private_key, chainid);
      
            try {
               cc.push_transaction( std::make_shared<transaction_metadata>(trx) );
            } catch (const account_name_exists_exception& ) {
               // fallback
            }
      }
      void start_timer( ) {
         timer->expires_from_now(timer_period);
         timer->async_wait( [this](boost::system::error_code ec) {
               start_timer();
               if(!ec) {
                  send_heartbeat_transaction();
               }
               else {
                  elog( "Error from connection check monitor: ${m}",( "m", ec.message()));
               }
            });
      }      
};

producer_heartbeat_plugin::producer_heartbeat_plugin():my(new producer_heartbeat_plugin_impl()){}
producer_heartbeat_plugin::~producer_heartbeat_plugin(){}

void producer_heartbeat_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("heartbeat-period", bpo::value<int>()->default_value(300),
          "Heartbeat transaction period in seconds")
         ("heartbeat-signature-provider", bpo::value<string>()->default_value(""),
          "Heartbeat key provider")
         ("heartbeat-contract", bpo::value<string>()->default_value("heartbeat"),
          "Heartbeat Contract")          
         ;
}




void producer_heartbeat_plugin::plugin_initialize(const variables_map& options) {
   try {
      if( options.count( "heartbeat-period" )) {
         // Handle the option
         my->timer_period = std::chrono::seconds( options.at( "heartbeat-period" ).as<int>());
      }
      if( options.count( "heartbeat-contract" )) {
         // Handle the option
         my->heartbeat_contract = options.at( "heartbeat-contract" ).as<std::string>();
      }
      if(options.count("producer-name")){
         my->producer_name =  options["producer-name"].as<std::vector<std::string>>()
      }
      if( options.count("heartbeat-signature-provider") ) {
            const std::string key_spec_pair = options["heartbeat-signature-provider"].as<std::string>();
            
            try {
               auto delim = key_spec_pair.find("=");
               EOS_ASSERT(delim != std::string::npos, plugin_config_exception, "Missing \"=\" in the key spec pair");
               auto pub_key_str = key_spec_pair.substr(0, delim);
               auto spec_str = key_spec_pair.substr(delim + 1);
   
               auto spec_delim = spec_str.find(":");
               EOS_ASSERT(spec_delim != std::string::npos, plugin_config_exception, "Missing \":\" in the key spec pair");
               auto spec_type_str = spec_str.substr(0, spec_delim);
               auto spec_data = spec_str.substr(spec_delim + 1);
   
               auto pubkey = public_key_type(pub_key_str);
               
               
               if (spec_type_str == "KEY") {
                  my->_hearbeat_private_key = make_key_signature_provider(private_key_type(spec_data));   
                  my->_hearbeat_public_key = pubkey;   
               } else if (spec_type_str == "KEOSD") {
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
   if(fc::get_logger_map().find(logger_name) != fc::get_logger_map().end()) {
      _log = fc::get_logger_map()[logger_name];
   }
   auto& producer = app().get_plugin<producer_plugin>();
   my->start_timer();
   
   
   
   
}

void producer_heartbeat_plugin::plugin_shutdown() {
   // OK, that's enough magic
}

}
