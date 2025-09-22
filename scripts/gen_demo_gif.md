# Generate demo GIF for README

## Option A: ScreenToGif (Windows-native)
1) Install ScreenToGif (Microsoft Store or screentogif.com).
2) Open Windows Terminal → run your app: `.\build\chub`.
3) Record a short interaction (search, select, copy-back).
4) Export as GIF (keep under ~3–5 MB).

## Option B: asciinema (via WSL)
1) In WSL: `sudo apt install asciinema`
2) Record: `asciinema rec demo.cast` → run `./build/chub` in the WSL terminal.
3) Convert to GIF (from Windows) using `agg` or an online cast→gif tool.

Tips:
- Use a dark theme and a monospace font.
- Keep it short (8–15 seconds).
