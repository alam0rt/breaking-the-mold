# Boss System Analysis — Superseded

> **This document (dated 2026-01-14, "60% complete") has been superseded.**
> It was written before the naming correction that revealed many "Klogg*"
> functions were actually Joe-Head-Joe's ball-throwing system.
>
> Use these instead:
>
> - **[../bosses.md](../bosses.md)** — Complete boss reference (~97% coverage, 80+ named functions)
> - **[../boss-ai/boss-system-analysis.md](../boss-ai/boss-system-analysis.md)** — Current architecture doc (4 boss-system types)
> - **[../boss-ai/boss-behaviors.md](../boss-ai/boss-behaviors.md)** — Attack patterns
> - **[../boss-entity-pattern.md](../boss-entity-pattern.md)** — Multi-sprite init pattern
> - Per-boss: [boss-klogg](../boss-ai/boss-klogg.md), [boss-monkey-mage](../boss-ai/boss-monkey-mage.md), [boss-glenn-yntis](../boss-ai/boss-glenn-yntis.md), [boss-shriney-guard](../boss-ai/boss-shriney-guard.md), [../../JOE_HEAD_JOE_COMPLETE.md](../../JOE_HEAD_JOE_COMPLETE.md)
>
> Reason for deprecation:
> - Treated `InitBossEntity @ 0x80047fb8` as a generic boss initializer — it is specifically `InitMonkeyMageBoss` (only 1 of 5 bosses uses the 6-part-platform pattern).
> - Attributed ball-throwing system to "Klogg" (incorrect — that's Joe-Head-Joe).
> - Pre-dates the 76-function Ghidra naming pass completed 2026-01-20.
