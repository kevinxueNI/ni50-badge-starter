# Copilot Starter Prompts

## Prompt 1: Visual Style

```text
I want to update the UI style of this badge project.

First determine whether the current target is the 2.8-inch board or the 4-inch board:
- For 2.8-inch, check `main/LVGL_UI/Badge_UI.c` first.
- For 4-inch, check `main/ui/ui.c` and `main/ui/page_card.c` first.

Goals:
1. Keep the existing page structure and navigation logic.
2. Only adjust colors, visual hierarchy, button style, and local spacing first.
3. Prefer the smallest change that produces a visible result and is easy to validate with a build.

Please first output:
1. the best 3 files to edit first,
2. the 1-2 best places to change in each file,
3. a minimal first modification plan.
```

## Prompt 2: Content And Images

```text
I want to personalize this badge project without changing the system architecture.

Please locate the content and image entry points based on the project structure:
- For 2.8-inch, check `main/LVGL_UI/img_avatar.c`, `img_ni5040.c`, `img_bg_pattern.c`, and `main/LVGL_UI/gallery/*.c`.
- For 4-inch, check `main/assets/img_avatar_160.c`, `img_ni5040_logo.c`, `img_bg_pattern.c`, and `main/assets/gallery/*.c`.

Goals:
1. replace avatar or main image,
2. replace default text with my own name, team, or project intro,
3. keep the existing functionality unchanged.

Please first:
1. identify the image and text entry files,
2. tell me which strings or files should be replaced,
3. explain any image size or format constraints,
4. give me a minimal modification plan that I can validate with a build.
```

## Prompt 3: Add A Simple Page

```text
I want to add one simple page to this badge project, such as About Me, Team Intro, or Project Intro.

Please determine the best reference files based on the project structure:
- For 2.8-inch, check `main/LVGL_UI/page_calendar.c`, `page_config.c`, and `Badge_UI.c` if page assembly needs changes.
- For 4-inch, check `main/ui/page_card.c`, `page_calendar.c`, `page_config.c`, and verify whether `main/ui/ui.c` or `main/CMakeLists.txt` needs updates.

Requirements:
1. reuse the existing page structure and navigation where possible,
2. avoid large architecture changes,
3. implement the smallest runnable version first.

Please first:
1. identify the best reference pages,
2. list the files to modify,
3. propose the minimum implementation steps,
4. keep it suitable for immediate build validation.
```
