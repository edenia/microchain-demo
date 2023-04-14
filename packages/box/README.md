# EdenOS Box

Box is a utility server to extend Eden App functionalities like uploading, compressing and pinning big files to IPFS.

## Box Settings

When deploying the box you can set up all of the configuration using environment variables. Most of them are self explanatory and can be easily digestible by looking at the `.env` file. Here are some important ones that you must know how to set:

-   `EOS_CHAIN_ID`, `EOS_RPC_PROTOCOL`, `EOS_RPC_HOST`, `EOS_RPC_PORT`: This is the configuration related to the EOS Blockchain that the box supports (generally for parsing aciton ABIs and broadcasting transactions).
-   `IPFS_PINATA_JWT`: the box has a file upload endpoint that receives a file and upload to IPFS using Pinata API. Set here your Pinata JWT that has permission to `pinFileToIPFS` (the only API endpoint Box needs).

### Setting up local environment

To have your app talk to a locally-running instance of the box, make sure in your webapp's config.ts,

1. you set your NEXT_PUBLIC_BOX_ADDRESS to "http://localhost:3032".
2. Your NEXT_PUBLIC_AA_COLLECTION_NAME and NEXT_PUBLIC_EDEN_CONTRACT_ACCOUNT match the Box's config.ts' EDEN_CONTRACT_ACCOUNT and SUBCHAIN_EDEN_CONTRACT values.

### Subchain settings

The default settings enable subchain support, using `eos.dfuse.eosnation.io` to grab history related to the `genesis.eden` contract on `EOS`.

-   `SUBCHAIN_DISABLE`: if present disables subchain support
-   `DFUSE_PREVENT_CONNECT`: if present disables connecting to dfuse
-   `SUBCHAIN_EDEN_CONTRACT`, `SUBCHAIN_TOKEN_CONTRACT`, `SUBCHAIN_AA_CONTRACT`, and `SUBCHAIN_AA_MARKET_CONTRACT`: contracts to filter
-   `SUBCHAIN_WASM`: location of `eden-micro-chain.wasm`
-   `SUBCHAIN_STATE`: location where to store the wasm's state
-   `DFUSE_API_KEY` is optional. Not currently necessary with the document rate this consumes.
-   `DFUSE_API_NETWORK` defaults to `eos.dfuse.eosnation.io`. Do not include the protocol in this field.
-   `DFUSE_AUTH_NETWORK` defaults to `https://auth.eosnation.io`. This requires the protocol (https).
-   `DFUSE_FIRST_BLOCK`: which block to start at. For `genesis.eden` on `EOS`, use 183705819. For test environments, you can generally use bloks.io on your targeted network with your targeted contract and then filter for contract name and action name = 'genesis'. Click the trx link, and grab the block height from there.
-   `DFUSE_JSON_TRX_FILE`: location to cache dfuse results. Defaults to `dfuse-transactions.json`

## Building Image and Publishing to GHCR

```sh
# from repository root
docker build -t eden/box -f docker/eden-box.Dockerfile .
docker image tag eden/box ghcr.io/eoscommunity/eden-box:latest
docker image push ghcr.io/eoscommunity/eden-box:latest
```

## Deploying Edenbox

Just go to your server docker panel and get the edenbox latest version: `ghcr.io/eoscommunity/eden-box:latest`

All the published versions are listed here: https://github.com/eoscommunity/Eden/pkgs/container/eden-box/versions
