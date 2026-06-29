---
title: "Tile Collision — Superseded"
category: systems
tags: [systems, tile, collision]
---

# Tile Collision — Superseded

> **This document was superseded on 2026-04-21 and the content merged into
> [collision-complete.md](collision-complete.md).** The older version here
> predated the slope-height analysis, color-tinting zones, and semi-solid
> override documentation.
>
> - **Canonical reference:** [collision-complete.md](collision-complete.md)
> - **Overview / navigation:** [collision.md](collision.md)
> - **Cheatsheet:** [../tile-collision-quick-ref.md](../tile-collision-quick-ref.md)

Reason for deprecation: duplicated `PlayerProcessTileCollision` switch-statement
analysis already present in `collision-complete.md`, but omitted the slope-height
table, color-tint zones (0x15–0x28), and semi-solid override mechanic.
