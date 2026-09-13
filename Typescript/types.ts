import type { Context } from "hono";
import { z } from "zod";

export type AppContext = Context<{ Bindings: Env }>;

// 现有的 Task schema
export const Task = z.object({
  name: z.string().openapi({ example: "lorem" }),
  slug: z.string(),
  description: z.string().optional(),
  completed: z.boolean().default(false),
  due_date: z.date(),
});

// OTL Token Schema
export const OTLToken = z.object({
  token: z.string().openapi({ example: "abc-123-def" }),
  targetUrl: z.string().url().openapi({ example: "https://example.com" }),
  createdAt: z.string().openapi({ example: new Date().toISOString() }),
  expirationSeconds: z.number().openapi({ example: 3600 }),
  used: z.boolean().openapi({ example: false }),
  usedAt: z.string().nullable().optional(),
});

export const CreateOTLTokenRequest = z.object({
  token: z.string().optional(),
  targetUrl: z.string().url(),
  expirationSeconds: z.number().optional().default(3600),
});

export const VerifyOTLTokenRequest = z.object({
  token: z.string(),
});
