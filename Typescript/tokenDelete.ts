import { OpenAPIRoute } from "chanfana";
import { z } from "zod";
import { type AppContext, VerifyOTLTokenRequest } from "../types";

export class TokenDelete extends OpenAPIRoute {
  schema = {
    tags: ["OTL Tokens"],
    summary: "删除令牌",
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
        description: "令牌删除成功",
        content: {
          "application/json": {
            schema: z.object({
              success: z.boolean(),
              token: z.string(),
            }),
          },
        },
      },
    },
  };

  async handle(c: AppContext) {
    const data = await this.getValidatedData<typeof this.schema>();
    const { token } = data.body;

    await c.env.OTL_STORE.delete(`token:${token}`);

    return c.json({
      success: true,
      token,
    });
  }
}
