import { OpenAPIRoute } from "chanfana";
import { z } from "zod";
import { type AppContext, VerifyOTLTokenRequest } from "../types";

export class TokenVerify extends OpenAPIRoute {
  schema = {
    tags: ["OTL Tokens"],
    summary: "验证令牌有效性",
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
        description: "令牌验证结果",
        content: {
          "application/json": {
            schema: z.object({
              token: z.string(),
              isValid: z.boolean(),
              used: z.boolean(),
              targetUrl: z.string().optional(),
              createdAt: z.string().optional(),
              usedAt: z.string().optional(),
            }),
          },
        },
      },
      "404": {
        description: "令牌不存在或已过期",
      },
    },
  };

  async handle(c: AppContext) {
    const data = await this.getValidatedData<typeof this.schema>();
    const { token } = data.body;

    const record = await c.env.OTL_STORE.get(`token:${token}`, "json") as any;

    if (!record) {
      return c.json(
        {
          token,
          isValid: false,
          used: false,
          error: "Token not found or expired",
        },
        404
      );
    }

    return c.json({
      token,
      isValid: !record.used,
      used: record.used,
      targetUrl: record.targetUrl,
      createdAt: new Date(record.createdAt).toISOString(),
      usedAt: record.usedAt
        ? new Date(record.usedAt).toISOString()
        : undefined,
    });
  }
}
