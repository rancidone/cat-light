---
status: draft
date: YYYY-MM-DD
---

# [Unit N] Design — [Name]

## Problem
## Responsibilities
## Boundaries
## Interface
## Concurrency
## Tradeoffs & Decisions

---

# Frontmatter Contract

| Field | Required | Values |
|-------|----------|--------|
| `status` | yes | `draft` → `ready` → `implemented` → `superseded` |
| `date` | yes | ISO date of last significant change |

**Status rules:**
- All docs stay `draft` until a full feature set is promoted together
- `ready` = approved for implementation
- `implemented` = built and merged
- `superseded` = replaced by another doc
