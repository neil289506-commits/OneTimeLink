import { OpenAPIRoute } from "chanfana";
import { z } from "zod";
import { type AppContext, VerifyOTLTokenRequest } from "../types";

export class TokenUse extends OpenAPIRoute {
  schema = {
    tags: ["OTL Tokens"],
    summary: "使用令牌（标记为已使用）",
    request: {
      body: {
        content: {
          "application/json": {
            schema: VerifyOTLTokenRequest,
          },
        },
      },
    },
    responses: {
      "200": {
        description: "令牌已使用，返回目标 URL",
        content: {
          "application/json": {
            schema: z.object({
              success: z.boolean(),
              token: z.string(),
              targetUrl: z.string(),
            }),
          },
        },
      },
      "404": {
        description: "令牌不存在",
      },
      "403": {
        description: "令牌已被使用",
      },
    },
  };

  async handle(c: AppContext) {
    const data = await this.getValidatedData<typeof this.schema>();
    const { token } = data.body;

    const record = (await c.env.OTL_STORE.get(`token:${token}`, "json")) as any;

    if (!record) {
      return c.json(
        { error: "Token not found or expired" },
        404
      );
    }

    if (record.used) {
      return c.json(
        { error: "Token already used" },
        403
      );
    }

    record.used = true;
    record.usedAt = Date.now();
    await c.env.OTL_STORE.put(
      `token:${token}`,
      JSON.stringify(record),
      { expirationTtl: record.expirationTime }
    );

    await c.env.OTL_STORE.put(
      `access:${token}:${Date.now()}`,
      JSON.stringify({
        token,
        timestamp: Date.now(),
        userAgent: c.req.header("user-agent") || "unknown",
        ip: c.req.header("cf-connecting-ip") || "unknown",
      }),
      { expirationTtl: 7 * 24 * 3600 }
    );

    return c.json({
      success: true,
      token,
      targetUrl: record.targetUrl,
    });
  }
}
