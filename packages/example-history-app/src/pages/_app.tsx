import { AppProps } from "next/app";
import {
    useCreateChain,
    ChainContext,
} from "@microchain/subchain-client/dist/ReactSubchain";
import "../../../../node_modules/graphiql/graphiql.min.css";

if (
    !process.env.NEXT_PUBLIC_STARTER_CONTRACT ||
    !process.env.NEXT_PUBLIC_SUBCHAIN_WASM_URL ||
    !process.env.NEXT_PUBLIC_SUBCHAIN_STATE_URL ||
    !process.env.NEXT_PUBLIC_SUBCHAIN_WS_URL ||
    !process.env.NEXT_PUBLIC_SUBCHAIN_SLOW_MO
) {
    throw new Error("ExampleHistoryApp Environment Variables are not set");
}

const MyApp = ({ Component, pageProps }: AppProps) => {
    const subchain = useCreateChain({
        fetch: global.window?.fetch, // undefined for nodejs to prevent lambda perf issues
        account: process.env.NEXT_PUBLIC_STARTER_CONTRACT,
        wasmUrl: process.env.NEXT_PUBLIC_SUBCHAIN_WASM_URL!,
        stateUrl:
            process.env.NEXT_PUBLIC_SUBCHAIN_SLOW_MO === "true"
                ? "bad_state_file_name_for_slow_mo"
                : process.env.NEXT_PUBLIC_SUBCHAIN_STATE_URL!,
        blocksUrl: process.env.NEXT_PUBLIC_SUBCHAIN_WS_URL!,
        slowmo: process.env.NEXT_PUBLIC_SUBCHAIN_SLOW_MO === "true",
    });
    return (
        <ChainContext.Provider value={subchain}>
            <Component {...{ ...pageProps, subchain }} />
        </ChainContext.Provider>
    );
};
export default MyApp;
