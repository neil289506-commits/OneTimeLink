import { OpenAPIRoute } from "chanfana";
import { z } from "zod";
import { type AppContext, CreateOTLTokenRequest } from "../types";
import * as crypto from "crypto";

function generateUUID(): string {
  const bytes = crypto.getRandomValues(new Uint8Array(16));
  bytes[6] = (bytes[6] & 0x0f) | 0x40;
  bytes[8] = (bytes[8] & 0x3f) | 0x80;

  const hex = Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, "0"))
    .join("");

  return [
    hex.slice(0, 8),
    hex.slice(8, 12),
    hex.slice(12, 16),
    hex.slice(16, 20),
    hex.slice(20),
  ].join("-");
}

export class TokenCreate extends OpenAPIRoute {
  schema = {
    tags: ["OTL Tokens"],
    summary: "创建一次性令牌",
    request: {
      body: {
        content: {
          "application/json": {
            schema: CreateOTLTokenRequest,
          },
        },
      },
    },
    responses: {
      "201": {
        description: "令牌创建成功",
        content: {
          "application/json": {
            schema: z.object({
              success: z.boolean(),
              token: z.string(),
              targetUrl: z.string(),
              expirationSeconds: z.number(),
              redirectUrl: z.string(),
            }),
          },
        },
      },
    },
  };

  async handle(c: AppContext) {
    const data = await this.getValidatedData<typeof this.schema>();
    const { targetUrl, expirationSeconds } = data.body;
    const token = data.body.token || generateUUID();

    const record = {
      token,
      targetUrl,
      createdAt: Date.now(),
      expirationTime: expirationSeconds || 3600,
      used: false,
      usedAt: null,
    };

    await c.env.OTL_STORE.put(
      `token:${token}`,
      JSON.stringify(record),
      {
        expirationTtl: expirationSeconds || 3600,
      }
    );

    const url = new URL(c.req.url);

    return c.json(
      {
        success: true,
        token,
        targetUrl,
        expirationSeconds: expirationSeconds || 3600,
        redirectUrl: `${url.origin}/api/tokens/redirect?token=${token}`,
      },
      201
    );
  }
}
