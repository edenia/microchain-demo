# Subchain Client

## Overview

The Microchain Demo webapp uses a subchain to track history. This subchain contains a subset of eosio blocks relevant to the `chainstarter` contract's history. `starter-micro-chain.wasm`, which runs in both nodejs and in browsers, produces and consumes this subchain. It answers GraphQL queries about the contract history. Here's a typical setup:

### Box server maintains the chain

```
+--------+    +----------+    +------+    +----------+    +-----------+
| dfuse  | => | Relevant | => | wasm | => | subchain | => | websocket |
| client |    | History  |    |      |    | blocks   |    |           |
+--------+    +----------+    |      |    +----------+    +-----------+
                              |      |    +----------+    +-----------+
                              |      | => | current  | => | http GET  |
                              |      |    | state    |    |           |
                              +------+    +----------+    +-----------+
```

### nodejs and web clients consume the chain

```
+-----------+    +----------+    +------+    +----------+    +--------+
| websocket | => | subchain | => | wasm | <= | GraphQL  | <= | client |
|           |    | blocks   |    |      |    | Query    |    | code   |
+-----------+    +----------+    |      |    +----------+    |        |
+-----------+    +----------+    |      |    +----------+    |        |
| http GET  | => | initial  | => |      | => | GraphQL  | => |        |
|           |    | state    |    |      |    | Response |    |        |
+-----------+    +----------+    +------+    +----------+    +--------+
```

When a client starts up, it fetches a copy of `starter-micro-chain.wasm` and a copy of the most-recent state from the Box server. It then subscribes to block updates through a websocket connection.
