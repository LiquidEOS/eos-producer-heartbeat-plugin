# EOSIO Block Producer Hearbeat Plugin by LiquidEOS (WIP)
An heatbeat plugin to coordinate BP node status and metadata on-chain. 

The goal is to show that a standby or active BP is ready for block production in terms version, blacklist sync, configuration, etc.

This plugin is intended for both standby and active BPs. it is intended to run on the block producing node.

# producer-heartbeat plugin features
## Periodic on-chain heartbeat
Contains version, head_block_num

## Blacklist Hashes - TBD

## Latency data from other BPs - TBD

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

# How to setup on your nodeos

Enable this plugin using `--plugin` option to nodeos or in your config.ini. Use `nodeos --help` to see options used by this plugin.

## Edit your nodeos config.ini (probably easier)

# Setup permissions
Use a dedicated key for this action.

```
cleos create key # HEARBEAT_PRIVATE_KEY and HEARBEAT_PUB_KEY 
cleos set account permission PRODUCERACCT heartbeat '{"threshold":1,"keys":[{"key":"HEARBEAT_PUB_KEY","weight":1}]}' "active" -p PRODUCERACCT@active
cleos set action permission PRODUCERACCT heartbeat123 heartbeat heartbeat
```

# Enable plugin
plugin = eosio::producer_heartbeat_plugin
heartbeat-period = 300
heartbeat-signature-provider = HEARBEAT_PUB_KEY=KEY:HEARBEAT_PRIVATE_KEY
heartbeat-contract = heartbeat123
 ```
 
## Check if the plugin has loaded
- You should see an entry for producer_heartbeat_plugin in the logs when you restart nodeos. 

# Feedback & development
- Any suggestions and pull requests are welcome :)