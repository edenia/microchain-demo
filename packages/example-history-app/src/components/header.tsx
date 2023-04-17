import Link from "next/link";
import { useQuery } from "@microchain/subchain-client/dist/ReactSubchain";

export default function Header() {
    const info = useQuery(`{
        blockLog {
            head {
                num
                eosioBlock {
                    num
                }
            }
        }
    }`);
    return (
        <div>
            <div style={{ margin: 10 }}>
                <table>
                    <tbody>
                        <tr>
                            <td>block:</td>
                            <td>{info.data?.blockLog.head?.num}</td>
                        </tr>
                        <tr>
                            <td>eosio block:</td>
                            <td>{info.data?.blockLog.head?.eosioBlock.num}</td>
                        </tr>
                    </tbody>
                </table>
            </div>
            <ul>
                <li>
                    <Link href="/">
                        <a>Greetings</a>
                    </Link>
                </li>
                <li>
                    <Link href="/simple_members">
                        <a>Simple Greetings</a>
                    </Link>
                </li>
                <li>
                    <Link href="/graphiql">
                        <a>GraphiQL</a>
                    </Link>
                </li>
            </ul>
        </div>
    );
}
