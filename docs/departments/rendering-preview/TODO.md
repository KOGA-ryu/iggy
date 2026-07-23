# Rendering and Preview TODO

| ID | Capability | Maturity | Delivery | Evidence | Next |
| --- | --- | --- | --- | --- | --- |
| REN-001 | Revision-based scene projection cache | Integrated | Needs Audit | Scene cache and idle-frame tests exist | Prove cache invalidation for every mutation and no work for aim-only changes |
| REN-002 | Placement held item and target preview | Integrated | Manual Test Needed | Preview roles and resource paths exist | Run the preview baseline for valid, invalid, absent, and accepted targets |
| REN-003 | Selection, gizmo, and tool overlays | Integrated | Needs Audit | Overlay owners and role tables exist | Audit depth, occlusion, readability, and view-specific behavior |
| REN-004 | World Layout plan and elevation rendering | Integrated | Needs Audit | Plan projection and drafting styles exist | Prove symbol-role parity and eliminate embedded presentation literals |
| REN-005 | Terrain surface and contour rendering | Integrated | Needs Audit | Terrain preview and contour paths exist | Prove curved surface continuity, normals, scale, and bounded mesh updates |
| REN-006 | Imported static mesh presentation | Integrated | Needs Audit | Static mesh resource and catalog paths exist | Prove materials, pivots, LOD policy, missing resources, and reload |
| REN-007 | Capture and renderer lifecycle | Support | Needs Audit | Capture and Vulkan smoke gates exist | Keep headless capture deterministic and external UI isolated |
| REN-008 | Performance budgets and diagnostics | Prototype | Planned | Render budget policies and counters exist | Establish visible budgets for uploads, triangles, cache rebuilds, and frame time |
