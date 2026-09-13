import { OpenAPIRoute } from "chanfana";
import { z } from "zod";
import { type AppContext } from "../types";

export class TokenList extends OpenAPIRoute {
  schema = {
    tags: ["OTL Tokens"],
    summary: "列出所有活跃令牌",
    responses: {
      "200": {
        description: "活跃令牌列表",
        content: {
          "application/json": {
            schema: z.object({
              success: z.boolean(),
              tokens: z.array(
                z.object({
                  token: z.string(),
                  targetUrl: z.string(),
                  createdAt: z.string(),
                  expirationSeconds: z.number(),
                })
              ),
              count: z.number(),
            }),
          },
        },
      },
    },
  };

  async handle(c: AppContext) {
    const result = await c.env.OTL_STORE.list({ prefix: "token:" });

    const tokens = [];

    for (const key of result.keys) {
      const record = (await c.env.OTL_STORE.get(key.name, "json")) as any;
      if (record && !record.used) {
        tokens.push({
          token: record.token,
          targetUrl: record.targetUrl,
          createdAt: new Date(record.createdAt).toISOString(),
          expirationSeconds: record.expirationTime,
        });
      }
    }

    return c.json({
      success: true,
      tokens,
      count: tokens.length,
    });
  }
}
