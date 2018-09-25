# EOSIO Block Producer Heartbeat Plugin by LiquidEOS
Heartbeat plugin to coordinate BP node status and metadata on-chain. 

The goal is to show that a standby or active BP is ready for block production in terms version, blacklist sync, configuration, etc.

This plugin is intended for both standby and active BPs. it is intended to run on the block producing node.

Contract deployed at: eosheartbeat (on mainnet and on the jungle testnet)

Monitor: http://heartbeat.liquideos.com/

Repository for monitor frontend and contract: https://github.com/bancorprotocol/eos-producer-heartbeat

*Make sure you allocate enough CPU/NET to your account (100 EOS each is enough). so you are still able to perform the daily claim*

# producer-heartbeat plugin features
## Periodic on-chain heartbeat
Contains version, head_block_num

## Blacklist Hashes

actor-blacklist hash

automatic submission to https://github.com/EOSLaoMao/theblacklist

## System resources 
Total system ram, state_db_size, Processor type, VM or not, etc.

## Latency data from other BPs

## Local time, NTP time diff and timezone - TBD

## Sign a field with the real producer private key - TBD

# Installation instructions

## Requirements
- Works on any EOSIO node that runs v1.1.4 and up.

## Building the plugin [Install on your nodeos server]
1. run
  ```
  cd <eosio-source-dir>/plugins
  git clone https://github.com/bancorprotocol/eos-producer-heartbeat-plugin.git producer_heartbeat_plugin
  ```
2. Add the following line to `<eosio-source-dir>/plugins/CMakeLists.txt` with other `add_subdirectory` items
  ```
  add_subdirectory(producer_heartbeat_plugin)
  ```

3. Add the following line to the bottom of `<eosio-source-dir>/programs/nodeos/CMakeLists.txt`
  ```
  target_link_libraries( nodeos PRIVATE -Wl,${whole_archive_flag} producer_heartbeat_plugin -Wl,${no_whole_archive_flag})
  ```
4. Build and install nodeos as usual. You could even just `cd <eosio-source-dir>/build` and then `sudo make install`

# Setup permissions 
Use a dedicated key for this action. This step is not mandatory (especially on testnets), you can use your active key instead and set ```heartbeat-permission = active```

```
cleos create key # HEARTBEAT_PRIVATE_KEY and HEARTBEAT_PUB_KEY 
cleos set account permission PRODUCERACCT heartbeat '{"threshold":1,"keys":[{"key":"HEARTBEAT_PUB_KEY","weight":1}]}' "active" -p PRODUCERACCT@active
cleos set action permission PRODUCERACCT eosheartbeat heartbeat heartbeat
cleos set action permission PRODUCERACCT theblacklist sethash heartbeat
```
# How to setup on your nodeos

## Enable plugin in config.ini

```
plugin = eosio::producer_heartbeat_plugin
heartbeat-period = 300
heartbeat-signature-provider = HEARTBEAT_PUB_KEY=KEY:HEARTBEAT_PRIVATE_KEY
heartbeat-contract = eosheartbeat
heartbeat-permission = heartbeat
 ```
 
## Check if the plugin has loaded
- You should see an entry for producer_heartbeat_plugin in the logs when you restart nodeos. 

# Feedback & development
- Any suggestions and pull requests are welcome :)
