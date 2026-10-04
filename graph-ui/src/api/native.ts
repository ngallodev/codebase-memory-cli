/* Same-origin, read-only graph API. Responses are operation JSON payloads. */
export async function fetchRead<T>(
  endpoint: "projects" | "schema" | "snippet",
  args: Record<string, string | number | boolean> = {},
): Promise<T> {
  const query = new URLSearchParams();
  for (const [key, value] of Object.entries(args)) query.set(key, String(value));
  const res = await fetch(`/api/${endpoint}?${query}`);
  const json = await res.json();
  if (!res.ok) {
    throw new Error(json.error ?? `HTTP ${res.status}: ${res.statusText}`);
  }
  return json as T;
}
