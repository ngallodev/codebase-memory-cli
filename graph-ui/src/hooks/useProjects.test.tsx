/* @vitest-environment jsdom */
import { renderHook, waitFor } from "@testing-library/react";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { useProjects } from "./useProjects";

const fetchMock = vi.fn();
describe("useProjects machine-readable pagination", () => {
  beforeEach(() => {
    fetchMock.mockReset();
    vi.stubGlobal("fetch", fetchMock);
  });

  afterEach(() => vi.unstubAllGlobals());

  it("requests JSON and merges every project and schema page", async () => {
    fetchMock.mockImplementation(async (input: string) => {
      const [name, query] = input.slice(5).split("?");
      const args = Object.fromEntries(new URLSearchParams(query));
      const page = await (async () => {
        if (name === "projects") {
          if (args.offset === "0") {
            return {
              projects: [{ name: "alpha", root_path: "/alpha", indexed_at: "now" }],
              has_more: true,
              next_offset: 1,
            };
          }
          return {
            projects: [{ name: "beta", root_path: "/beta", indexed_at: "now" }],
            has_more: false,
          };
        }
        if (name === "schema" && args.project === "alpha") {
          if (args.offset === "0") {
            return {
              node_labels: [{ label: "Function", count: 3 }],
              edge_types: [],
              has_more: true,
              next_offset: 1,
            };
          }
          return {
            node_labels: [],
            edge_types: [{ type: "CALLS", count: 2 }],
            has_more: false,
          };
        }
        return {
          node_labels: [{ label: "Class", count: 1 }],
          edge_types: [],
          has_more: false,
        };
      })();
      return { ok: true, json: async () => page };
    });

    const { result } = renderHook(() => useProjects());
    await waitFor(() => expect(result.current.loading).toBe(false));

    expect(result.current.projects).toHaveLength(2);
    expect(result.current.projects[0].schema?.node_labels).toEqual([
      { label: "Function", count: 3 },
    ]);
    expect(result.current.projects[0].schema?.edge_types).toEqual([
      { type: "CALLS", count: 2 },
    ]);
    expect(result.current.projects[0].schema?.total_nodes).toBe(3);
    expect(result.current.projects[0].schema?.total_edges).toBe(2);
    expect(fetchMock).toHaveBeenCalledWith("/api/projects?limit=500&offset=0");
    expect(fetchMock).toHaveBeenCalledWith("/api/projects?limit=500&offset=1");
    expect(fetchMock).toHaveBeenCalledWith("/api/schema?project=alpha&limit=500&offset=1");
  });
  it("reports native API errors", async () => {
    fetchMock.mockResolvedValue({
      ok: false,
      status: 400,
      json: async () => ({ error: "cannot read cache directory" }),
    });
    const { result } = renderHook(() => useProjects());
    await waitFor(() => expect(result.current.loading).toBe(false));
    expect(result.current.error).toBe("cannot read cache directory");
    expect(result.current.projects).toEqual([]);
  });
});
