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
- Works on any EOSIO node that runs v1.5.0 and up. (see older tags for older versions)

## Building the plugin [Install on your nodeos server]
1. run
  ```
  cd <eosio-source-dir>
  cd plugins
  git clone https://github.com/LiquidEOS/eos-producer-heartbeat-plugin.git producer_heartbeat_plugin
  cd producer_heartbeat_plugin
  git checkout tags/v1.5.01
  cd ../..
  git apply plugins/producer_heartbeat_plugin/install.patch
  ./eosio_build.sh -s "EOS"
  sudo ./eosio_install.sh
  ```
2. Build and install nodeos as usual.

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
heartbeat-period = 1500
heartbeat-signature-provider = HEARTBEAT_PUB_KEY=KEY:HEARTBEAT_PRIVATE_KEY
heartbeat-contract = eosheartbeat
heartbeat-permission = heartbeat
heartbeat-oncall = telegram:johndoe
 ```

Set heartbeat-oncall to your oncall contacts
 
## Check if the plugin has loaded
- You should see an entry for producer_heartbeat_plugin in the logs when you restart nodeos. 

# Feedback & development
- Any suggestions and pull requests are welcome :)
