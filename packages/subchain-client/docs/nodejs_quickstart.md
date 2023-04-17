# Nodejs Quickstart

## Minimal Example

This starts up, runs a single query, and exits.

It needs the following NPM packages:

-   `@microchain/subchain-client`
-   `eosjs` (subchain-client needs it)
-   `node-fetch`
-   `ws`

```js
const SubchainClient = require("@microchain/subchain-client/dist/SubchainClient.js")
    .default;
const ws = require("ws");

const box = "127.0.0.1:3032/v1/subchain";

(async () => {
    try {
        const fetch = (await import("node-fetch")).default;

        // Start up the client
        const client = new SubchainClient(ws);
        await client.instantiateStreaming({
            wasmResponse: fetch(`https://${box}/starter-micro-chain.wasm`),
            stateResponse: fetch(`https://${box}/state`),
            blocksUrl: `wss://${box}/starter-microchain`,
        });

        // Run a query
        const greeting = client.subchain.query(`
        {
          greetings(@page@) {
            pageInfo {
              hasPreviousPage
              hasNextPage
              startCursor
              endCursor
            }
            edges {
              node {
                account
                message
              }
            }
          }
        }`).data?.greetings.edges[0]?.node;

        // Show result
        console.log(JSON.stringify(greeting, null, 4));
        process.exit(0);
    } catch (e) {
        console.error(e);
        process.exit(1);
    }
})();
```
