# Building and World Layout TODO

The current backlog contains implementation claims, but the complete building
workflow has not yet received one end-to-end product acceptance.

| ID | Capability | Maturity | Delivery | Evidence | Next |
| --- | --- | --- | --- | --- | --- |
| BLD-001 | Building blockout and template generation | Integrated | Manual Test Needed | Blockout planner, UI command, and workflow tests exist | Run the building baseline and record visible defects |
| BLD-002 | Room layout and floor-plan editing | Prototype | Needs Audit | Room operations and topology kernels exist | Define complete room create, resize, split, merge, and delete behavior |
| BLD-003 | Storeys and level ownership | Integrated | Needs Audit | Level operations and storey settings exist | Verify real scale, equal floor spacing, active-level editing, and exterior shell height |
| BLD-004 | Floors and ceilings | Prototype | Needs Audit | Slab generation exists | Consolidate top plane, thickness, collision, and render ownership |
| BLD-005 | Exterior walls and partitions | Stable Recipe | Needs Audit | Wall recipe and wall operations exist | Separate continuous exterior shell from storey-height interior partitions |
| BLD-006 | Generic openings and wall repair | Stable Recipe | Needs Audit | Opening and repair kernels exist | Prove arbitrary valid placement and deterministic host-wall reconstruction |
| BLD-007 | Doors | Integrated | Needs Audit | Door recipe and generated-source support exist | Complete hinge, swing, clearance, runtime state, and replacement behavior |
| BLD-008 | Windows | Integrated | Needs Audit | Window recipe and aperture support exist | Complete sill, lintel, glazing, snapping, resizing, and host-wall updates |
| BLD-009 | Stairs | Integrated | Needs Audit | Stair recipe and vertical connectors exist | Prove storey alignment, headroom, landings, railing sockets, and traversability |
| BLD-010 | Ramps | Integrated | Needs Audit | Ramp recipe exists | Prove slope limits, landings, collision, and accessibility |
| BLD-011 | Roofs | Integrated | Needs Audit | Flat and gable roof paths exist | Complete intersections, apertures, overhangs, ridge editing, and multi-wing joins |
| BLD-012 | Building refinement and validation | Prototype | Planned | Diagnostics, hierarchy, and repair surfaces exist | Create one canonical estate acceptance fixture and defect list |
