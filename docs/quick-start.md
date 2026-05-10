# Quick Start

## Goal

Get one board working first, then make one small change.

Do not try to understand the entire codebase before the first successful build.

## Step 1: Choose a Board

### Option A: 2.8-inch

Use `boards/2.8-inch/project/` if you want a faster starting point.

Best for:

- first build / flash,
- lightweight UI changes,
- content replacement.

### Option B: 4-inch

Use `boards/4-inch/project/` if you want the fuller showcase version.

Best for:

- richer storytelling,
- team intro pages,
- gallery and history-style screens.

## Step 2: Activate ESP-IDF

```powershell
Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
& "C:\Espressif\tools\Microsoft.v6.0.1.PowerShell_profile.ps1"
```

## Step 3: Build Once

### 2.8-inch

```powershell
Push-Location "boards/2.8-inch/project"
idf.py set-target esp32s3
idf.py build
Pop-Location
```

### 4-inch

```powershell
Push-Location "boards/4-inch/project"
idf.py set-target esp32s3
idf.py build
Pop-Location
```

Note:

- the 4-inch board may require its included patch flow depending on the environment,
- see the board README if the first build does not succeed.

## Step 4: Flash Once

```powershell
idf.py -p COM3 flash monitor
```

Replace `COM3` with the actual serial port on your machine.

## Step 5: Make One Small Change

Recommended first changes:

1. replace a name or team label,
2. replace an avatar or image,
3. adjust colors or one small layout section,
4. add one simple intro page.

## Step 6: Use Copilot for the Next Step

Use the prompts in `docs/copilot-prompts.md`.

The right pattern is:

1. locate the right file,
2. make one minimal change,
3. rebuild,
4. validate,
5. continue.

## Recommended First Files

### 2.8-inch

- `main/LVGL_UI/Badge_UI.c`
- `main/LVGL_UI/page_gallery.c`
- `main/LVGL_UI/img_avatar.c`

### 4-inch

- `main/ui/ui.c`
- `main/ui/page_card.c`
- `main/assets/img_avatar_160.c`
