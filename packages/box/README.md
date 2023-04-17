# Microchain Demo Box

Box is a utility server to extend Microchain Demo App functionalities like configuring dfuse to tweak the data before it is sent to the microchain.

## Box Settings

When deploying the box you can set up all of the configuration using environment variables. Most of them are self explanatory and can be easily digestible by looking at the `.env` file. Here are some important ones that you must know how to set:

-   `EOS_CHAIN_ID`, `EOS_RPC_PROTOCOL`, `EOS_RPC_HOST`, `EOS_RPC_PORT`: This is the configuration related to the EOS Blockchain that the box supports (generally for parsing aciton ABIs and broadcasting transactions).

### Setting up local environment

To have your app talk to a locally-running instance of the box, make sure in your webapp's config.ts,

1. you set your NEXT_PUBLIC_BOX_ADDRESS to "http://localhost:3032".

### Subchain settings

The default settings enable subchain support, using `eos.dfuse.eosnation.io` to grab history related to the `chainstarter` contract on `EOS`.

-   `SUBCHAIN_DISABLE`: if present disables subchain support
-   `DFUSE_PREVENT_CONNECT`: if present disables connecting to dfuse
-   `SUBCHAIN_CONTRACT`, `SUBCHAIN_TOKEN_CONTRACT`, `SUBCHAIN_AA_CONTRACT`, and `SUBCHAIN_AA_MARKET_CONTRACT`: contracts to filter
-   `SUBCHAIN_WASM`: location of `starter-micro-chain.wasm`
-   `SUBCHAIN_STATE`: location where to store the wasm's state
-   `DFUSE_API_KEY` is optional. Not currently necessary with the document rate this consumes.
-   `DFUSE_API_NETWORK` defaults to `eos.dfuse.eosnation.io`. Do not include the protocol in this field.
-   `DFUSE_AUTH_NETWORK` defaults to `https://auth.eosnation.io`. This requires the protocol (https).
-   `DFUSE_FIRST_BLOCK`: which block to start at. For `chainstarter` on `Jungle 4`, use 68424246. For test environments, you can generally use bloks.io on your targeted network with your targeted contract and then filter for contract name and action name = 'genesis'. Click the trx link, and grab the block height from there.
-   `DFUSE_JSON_TRX_FILE`: location to cache dfuse results. Defaults to `dfuse-transactions.json`
