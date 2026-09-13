/**
 * OTL Generator - Hono + Chanfana v3 框架版本
 */

import { Hono } from "hono";
import { fromHono, OpenAPIRoute } from "chanfana";
import { z } from "zod";
import * as crypto from "node:crypto";

// ============================================================================
// 环境和类型定义
// ============================================================================

interface Env {
  OTL_STORE: KVNamespace;
  MASTER_PASSWORD?: string;
}

interface TokenRecord {
  token: string;
  targetUrl: string;
  createdAt: number;
  expirationTime: number;
  used: boolean;
  usedAt: number | null;
}

interface AccessLog {
  token: string;
  timestamp: number;
  userAgent: string;
  ip: string;
}

// ============================================================================
// Zod Schema（用于验证与 OpenAPI 文档）
// ============================================================================

const TokenRecordSchema = z.object({
  token: z.string().describe("令牌 UUID"),
  targetUrl: z.string().url().describe("目标 URL"),
  createdAt: z.number().describe("创建时间戳"),
  expirationTime: z.number().describe("过期时间（秒）"),
  used: z.boolean().describe("是否已使用"),
  usedAt: z.number().nullable().describe("使用时间戳"),
});

const CreateTokenRequestSchema = z.object({
  token: z.string().optional().describe("令牌（可选，默认自动生成）"),
  targetUrl: z.string().url().describe("目标 URL"),
  expirationSeconds: z.number().optional().default(3600).describe("过期时间（秒）"),
});

const VerifyTokenRequestSchema = z.object({
  token: z.string().describe("要验证的令牌"),
});

// ============================================================================
// 工具函数
// ============================================================================

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

// ============================================================================
// KV 操作函数
// ============================================================================

async function createToken(
  kv: KVNamespace,
  token: string,
  targetUrl: string,
  expirationSeconds: number = 3600
): Promise<void> {
  const record: TokenRecord = {
    token,
    targetUrl,
    createdAt: Date.now(),
    expirationTime: expirationSeconds,
    used: false,
    usedAt: null,
  };

  await kv.put(`token:${token}`, JSON.stringify(record), {
    expirationTtl: expirationSeconds,
  });
}

async function getToken(
  kv: KVNamespace,
  token: string
): Promise<TokenRecord | null> {
  const data = await kv.get(`token:${token}`, "json");
  return data as TokenRecord | null;
}

async function markTokenAsUsed(
  kv: KVNamespace,
  token: string
): Promise<void> {
  const record = await getToken(kv, token);

  if (record) {
    record.used = true;
    record.usedAt = Date.now();

    await kv.put(`token:${token}`, JSON.stringify(record), {
      expirationTtl: record.expirationTime,
    });
  }
}

async function logAccess(
  kv: KVNamespace,
  token: string,
  userAgent: string,
  ip: string
): Promise<void> {
  const log: AccessLog = {
    token,
    timestamp: Date.now(),
    userAgent,
    ip,
  };

  await kv.put(`access:${token}:${Date.now()}`, JSON.stringify(log), {
    expirationTtl: 7 * 24 * 3600,
  });
}

async function listActiveTokens(kv: KVNamespace): Promise<TokenRecord[]> {
  const result = await kv.list({ prefix: "token:" });
  const tokens: TokenRecord[] = [];

  for (const key of result.keys) {
    const record = (await kv.get(key.name, "json")) as TokenRecord;
    if (record && !record.used) {
      tokens.push(record);
    }
  }

  return tokens;
}

async function getTokenStats(
  kv: KVNamespace
): Promise<{ total: number; active: number; used: number }> {
  const result = await kv.list({ prefix: "token:" });
  let total = 0;
  let active = 0;
  let used = 0;

  for (const key of result.keys) {
    total++;
    const record = (await kv.get(key.name, "json")) as TokenRecord;
    if (record) {
      if (record.used) used++;
      else active++;
    }
  }

  return { total, active, used };
}

// ============================================================================
// Chanfana v3 OpenAPI Routes
// ============================================================================

class CreateTokenRoute extends OpenAPIRoute {
  static schema = {
    tags: ["OTL Tokens"],
    summary: "创建一次性令牌",
    request: {
      body: {
        content: {
          "application/json": {
            schema: CreateTokenRequestSchema,
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
      "400": {
        description: "请求参数错误",
      },
    },
  };

  async handle(c: any) {
    const env = c.env as Env;
    let body: any;
    try {
      body = await c.req.json();
    } catch {
      return c.json({ error: "Invalid JSON body" }, 400);
    }

    const parsed = CreateTokenRequestSchema.safeParse(body);
    if (!parsed.success) {
      return c.json({ error: "Validation failed", details: parsed.error.format() }, 400);
    }

    const { targetUrl, token: customToken, expirationSeconds } = parsed.data;
    const token = customToken || generateUUID();
    const expSeconds = expirationSeconds || 3600;

    await createToken(env.OTL_STORE, token, targetUrl, expSeconds);

    const url = new URL(c.req.url);
    return c.json(
      {
        success: true,
        token,
        targetUrl,
        expirationSeconds: expSeconds,
        redirectUrl: `${url.origin}/api/redirect?token=${token}`,
      },
      201
    );
  }
}

class VerifyTokenRoute extends OpenAPIRoute {
  static schema = {
    tags: ["OTL Tokens"],
    summary: "验证令牌",
    request: {
      body: {
        content: {
          "application/json": {
            schema: VerifyTokenRequestSchema,
          },
        },
      },
    },
    responses: {
      "200": {
        description: "令牌验证结果",
      },
    },
  };

  async handle(c: any) {
    const env = c.env as Env;
    let body: any;
    try {
      body = await c.req.json();
    } catch {
      return c.json({ error: "Invalid JSON body" }, 400);
    }

    const parsed = VerifyTokenRequestSchema.safeParse(body);
    if (!parsed.success) {
      return c.json({ error: "Validation failed" }, 400);
    }

    const { token } = parsed.data;
    const record = await getToken(env.OTL_STORE, token);

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
      usedAt: record.usedAt ? new Date(record.usedAt).toISOString() : undefined,
    });
  }
}

class UseTokenRoute extends OpenAPIRoute {
  static schema = {
    tags: ["OTL Tokens"],
    summary: "使用令牌获取目标 URL",
    request: {
      body: {
        content: {
          "application/json": {
            schema: VerifyTokenRequestSchema,
          },
        },
      },
    },
    responses: {
      "200": {
        description: "令牌有效，返回目标 URL",
      },
    },
  };

  async handle(c: any) {
    const env = c.env as Env;
    let body: any;
    try {
      body = await c.req.json();
    } catch {
      return c.json({ error: "Invalid JSON body" }, 400);
    }

    const parsed = VerifyTokenRequestSchema.safeParse(body);
    if (!parsed.success) {
      return c.json({ error: "Validation failed" }, 400);
    }

    const { token } = parsed.data;
    const record = await getToken(env.OTL_STORE, token);

    if (!record) {
      return c.json({ error: "Token not found or expired" }, 404);
    }

    if (record.used) {
      return c.json({ error: "Token already used" }, 403);
    }

    const userAgent = c.req.header("user-agent") || "unknown";
    const ip = c.req.header("cf-connecting-ip") || "unknown";
    await logAccess(env.OTL_STORE, token, userAgent, ip);
    await markTokenAsUsed(env.OTL_STORE, token);

    return c.json({
      success: true,
      token,
      targetUrl: record.targetUrl,
    });
  }
}

class ListTokensRoute extends OpenAPIRoute {
  static schema = {
    tags: ["OTL Tokens"],
    summary: "列出所有活跃令牌",
    responses: {
      "200": {
        description: "活跃令牌列表",
      },
    },
  };

  async handle(c: any) {
    const env = c.env as Env;
    const tokens = await listActiveTokens(env.OTL_STORE);

    return c.json({
      success: true,
      tokens,
      count: tokens.length,
    });
  }
}

class DeleteTokenRoute extends OpenAPIRoute {
  static schema = {
    tags: ["OTL Tokens"],
    summary: "删除令牌",
    request: {
      body: {
        content: {
          "application/json": {
            schema: VerifyTokenRequestSchema,
          },
        },
      },
    },
    responses: {
      "200": {
        description: "令牌删除成功",
      },
    },
  };

  async handle(c: any) {
    const env = c.env as Env;
    let body: any;
    try {
      body = await c.req.json();
    } catch {
      return c.json({ error: "Invalid JSON body" }, 400);
    }

    const parsed = VerifyTokenRequestSchema.safeParse(body);
    if (!parsed.success) {
      return c.json({ error: "Validation failed" }, 400);
    }

    const { token } = parsed.data;
    await env.OTL_STORE.delete(`token:${token}`);

    return c.json({
      success: true,
      token,
    });
  }
}

class StatsRoute extends OpenAPIRoute {
  static schema = {
    tags: ["OTL Tokens"],
    summary: "获取令牌统计",
    responses: {
      "200": {
        description: "令牌统计信息",
      },
    },
  };

  async handle(c: any) {
    const env = c.env as Env;
    const stats = await getTokenStats(env.OTL_STORE);

    return c.json({
      ...stats,
      usagePercent:
        stats.total > 0 ? ((stats.used / stats.total) * 100).toFixed(2) : "0",
    });
  }
}

// ============================================================================
// Hono 应用與 Chanfana v3 整合
// ============================================================================

const app = new Hono<{ Bindings: Env }>();

// 健康检查
app.get("/health", (c) => {
  return c.json({
    status: "ok",
    timestamp: new Date().toISOString(),
  });
});

// 重定向端点
app.get("/api/redirect", async (c) => {
  const env = c.env as Env;
  const token = c.req.query("token");

  if (!token) {
    return c.json({ error: "Missing token parameter" }, 400);
  }

  const record = await getToken(env.OTL_STORE, token);
  if (!record) {
    return c.json({ error: "Token not found or expired" }, 404);
  }
  if (record.used) {
    return c.json({ error: "Token already used" }, 403);
  }

  const userAgent = c.req.header("user-agent") || "unknown";
  const ip = c.req.header("cf-connecting-ip") || "unknown";
  await logAccess(env.OTL_STORE, token, userAgent, ip);
  await markTokenAsUsed(env.OTL_STORE, token);

  return c.redirect(record.targetUrl, 302);
});

// 使用 chanfana v3 的 fromHono 建立 OpenAPI 路由
const openapi = fromHono(app, {
  docs_url: "/docs",
});

openapi.post("/api/tokens/create", CreateTokenRoute);
openapi.post("/api/tokens/verify", VerifyTokenRoute);
openapi.post("/api/tokens/use", UseTokenRoute);
openapi.get("/api/tokens/list", ListTokensRoute);
openapi.post("/api/tokens/delete", DeleteTokenRoute);
openapi.get("/api/tokens/stats", StatsRoute);

export default app;