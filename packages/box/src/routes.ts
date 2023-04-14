import { Router } from "express";
import { subchainConfig } from "./config";

import { infoHandler, subchainHandler } from "./handlers";

const router: Router = Router();

router.get("/", infoHandler);
if (subchainConfig.enable) router.use("/v1/subchain", subchainHandler);

export default router;
