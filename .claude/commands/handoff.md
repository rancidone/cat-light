Write a session handoff file for the current session using the template below

## Steps: ##
1. Fill in every field based on what was accomplished in this session. Be specific — include exact file paths for every output, exact numbers discovered, and conditional logic established.
2. Write the handoff to `./docs/summaries/handoff_YYYY-MM-DDTHH:MM_topic-slug.md` — underscores separate the three sections (type, timestamp, description); ISO date and hyphenated slug are unchanged.
3. If a previous handoff file exists in `./docs/handoffs/`, move it to `./docs/handoffs/archive`.
4. Tell me the file path of the new handoff and summarize what it contains.

----

## Session Handoff Template

```markdown
# Session Handoff: [Topic]
**Date:** [YYYY-MM-DD]
**Session Duration:** [approximate]
**Session Focus:** [one sentence]
**Context Usage at Handoff:** [estimated percentage if known]

## What Was Accomplished
<!-- Be specific. Include file paths for every output. -->
1. [task completed] → output at `[file path]`
2. [task completed] → output at `[file path]`

## Exact State of Work in Progress
<!-- If anything is mid-stream, describe exactly where it stopped -->
- [work item]: completed through [specific point], next step is [specific action]
- [work item]: blocked on [specific issue]

## Decisions Made This Session
<!-- Reference decision records if created, otherwise summarize here -->
- DR-[number]: [decision] (see `./docs/summaries/decision-[file]`)
- [Ad-hoc decision]: [what] BECAUSE [why] — STATUS: [confirmed/provisional]

## Key Numbers Generated or Discovered This Session
<!-- Every metric, estimate, or figure produced. Exact values. -->
- [metric]: [value] — [context for where/how this was derived]

## Conditional Logic Established
<!-- Any IF/THEN/BUT/EXCEPT reasoning that future sessions must respect -->
- IF [condition] THEN [approach] BECAUSE [rationale]

## Files Created or Modified
| File Path | Action | Description |
|-----------|--------|-------------|
| `[path]`  | Created | [what it contains] |
| `[path]`  | Modified | [what changed and why] |

## What the NEXT Session Should Do
<!-- Ordered, specific instructions. The next session starts by reading this. -->
1. **First**: [specific action with file paths]
2. **Then**: [specific action]
3. **Then**: [specific action]

## Open Questions Requiring User Input
<!-- Do NOT proceed on these without explicit user confirmation -->
- [ ] [question] — impacts [what downstream deliverable]
- [ ] [question]

## Assumptions That Need Validation
<!-- Things treated as true this session but not confirmed -->
- ASSUMED: [assumption] — validate by [method/person]

## What NOT to Re-Read
<!-- Prevent the next session from wasting context on already-processed material -->
- `[file path]` — already summarized in `[summary file path]`

## Files to Load Next Session
<!-- Explicit index of what the next session should read. Acts as progressive disclosure index layer. -->
- `[file path]` — needed for [reason]
- `[file path]` — needed for [reason]
```