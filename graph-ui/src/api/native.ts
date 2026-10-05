/* Same-origin, read-only graph API. Responses are operation JSON payloads. */
export async function fetchRead<T>(
  endpoint: "projects" | "schema" | "snippet",
  args: Record<string, string | number | boolean> = {},
): Promise<T> {
  const query = new URLSearchParams();
  for (const [key, value] of Object.entries(args)) query.set(key, String(value));
  const res = await fetch(`/api/${endpoint}?${query}`);
  if (!res.ok) {
    // Error bodies are not always JSON (plain-text 404, proxy error pages).
    const body = (await res.json().catch(() => null)) as { error?: string } | null;
    throw new Error(body?.error ?? `HTTP ${res.status}: ${res.statusText}`);
  }
  return (await res.json()) as T;
}
