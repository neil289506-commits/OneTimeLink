import { OpenAPIRoute } from "chanfana";
import { z } from "zod";
import { type AppContext } from "../types";

export class TokenStats extends OpenAPIRoute {
  schema = {
    tags: ["OTL Tokens"],
    summary: "获取令牌统计",
    responses: {
      "200": {
        description: "令牌统计信息",
        content: {
          "application/json": {
            schema: z.object({
              total: z.number(),
              active: z.number(),
              used: z.number(),
              usagePercent: z.string(),
            }),
          },
        },
      },
    },
  };

  async handle(c: AppContext) {
    const result = await c.env.OTL_STORE.list({ prefix: "token:" });

    let total = 0;
    let active = 0;
    let used = 0;

    for (const key of result.keys) {
      total++;
      const record = (await c.env.OTL_STORE.get(key.name, "json")) as any;
      if (record) {
        if (record.used) {
          used++;
        } else {
          active++;
        }
      }
    }

    return c.json({
      total,
      active,
      used,
      usagePercent:
        total > 0 ? ((used / total) * 100).toFixed(2) : "0",
    });
  }
}
