# Glossary #

## Account ##

An account is a unique identifier and a requirement to interact with an EOS Network Foundation blockchain. Unlike most other cryptocurrencies, transfers are sent to a human readable account name rather than a public key, while keys attributed to the account are used to sign transactions.

## Block ##

A confirmable unit of a blockchain. Each block contains zero or more transactions, as well as a cryptographic connection to all prior blocks. When a block becomes "irreversibly confirmed" it's because a supermajority of block producers have agreed that the given block contains the correct transactions. Once a block is irreversibly confirmed, it becomes a permanent part of the immutable blockchain.

## Block Producer ##

A Block Producer is an identifiable entity composed of one or more individuals that express interest in participating in running an EOS network. By participating it is meant these entities will provide a full node, gather transactions, verify their validity, add them into blocks, and propose and confirm these blocks. A Block producer is generally required to have experience with system administration and security as it is expected that their full-node have constant availability.

## CPU ##

CPU is processing power granted to an account by an EOSIO based blockchain. The amount of CPU an account has is measured in microseconds, and represents the amount of processing time an account has at its disposal when executing its actions. CPU is recalculated after each block is produced, based on the amount of system tokens the account staked for CPU bandwidth in proportion to the amount of total system tokens staked for CPU bandwidth at that time.

## Net ##

NET is required to store transactions on an EOS Network Foundation based blockchain. The amount of NET an account has is measured in bytes, representing the amount of transaction storage an account has at its disposal when creating a new transaction. NET is recalculated after each block is produced, based on the system tokens staked for NET bandwidth by the account. The amount allocated to an account is proportional with the total system tokens staked for NET by all accounts. Do not confuse NET with RAM, although it is also storage space, NET measures the size of the transactions and not contract state.

## Public-Key ##

A publicly available key that can be authorized to permissions of an account and can be used to identify the origin transaction. A public key can be inferred from a signature.

## Private-Key ##

A private key is a secret key used to sign transactions. In EOS Network Foundation, a private key's authority is determined by it's mapping to an EOS Network Foundation account name.
