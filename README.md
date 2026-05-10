# NI50 Badge Starter

This repository is the clean share package for the NI 50 badge project.

It is intended for private, read-only sharing with people who want to:

1. understand the two-badge project structure,
2. build and flash the existing demos,
3. use the codebase as a starter package for their own badge version.

## What This Repo Contains

- `boards/2.8-inch/project/`
  - lightweight starting point
  - faster feedback loop
  - better for first-time customization
- `boards/4-inch/project/`
  - richer showcase version
  - stronger UI expression
  - better for deeper extension
- `docs/quick-start.md`
  - minimum path to build, flash, and make the first change
- `docs/share-scope.md`
  - what is intentionally included and excluded
- `docs/copilot-prompts.md`
  - starter prompts for GitHub Copilot
- `docs/github-readonly-setup.md`
  - notes for maintaining private read-only sharing

## Which Board Should You Start With

### 2.8-inch

Start here if you want:

- a smaller and simpler project,
- faster validation,
- lightweight UI and content changes.

Path:

- `boards/2.8-inch/project/`

### 4-inch

Start here if you want:

- a more complete visual showcase,
- richer storytelling pages,
- a stronger base for team intro, gallery, and history-style content.

Path:

- `boards/4-inch/project/`

## Build Environment

- ESP-IDF: `v6.0.1`
- Recommended activation on Windows:

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
& "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"
```

See each board README for board-specific details.

## Suggested Reader Flow

1. Read `docs/quick-start.md`
2. Pick `2.8-inch` or `4-inch`
3. Build once
4. Flash once
5. Make one small visual or content change
6. Use `docs/copilot-prompts.md` for the next step

## Positioning

This repository is not a polished SDK.

It is a practical starter package built around a real NI 50 badge project, shaped by:

- hands-on iteration,
- AI-assisted creation,
- and the story connection to the Lincoln team.
