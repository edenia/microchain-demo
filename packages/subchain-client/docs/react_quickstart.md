# Nodejs Quickstart

## Minimal Example

It automatically updates whenever any update happens.

It needs the following NPM packages:

-   `@microchain/subchain-client`
-   `eosjs` (subchain-client needs it)
-   `react`
-   `react-dom`

```js
import React from "react";
import ReactDOM from "react-dom";
import {
    ChainContext,
    useCreateChain,
    useQuery,
} from "@microchain/subchain-client/dist/ReactSubchain";

const box = "127.0.0.1:3032/v1/subchain";

// Top of the react tree
function Top() {
    // Start up the client
    const subchain = useCreateChain({
        fetch,
        wasmUrl: `https://${box}/starter-micro-chain.wasm`,
        stateUrl: `https://${box}/state`,
        blocksUrl: `wss://${box}/starter-microchain`,
    });

    return (
        // Provide the client to the children
        <ChainContext.Provider value={subchain}></ChainContext.Provider>
    );
}
```
