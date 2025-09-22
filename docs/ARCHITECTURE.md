# Architecture 

- Single binary: `chub`
- Modules:
  - `tui` — minimal ncurses/PDCurses list UI
  - `db` — SQLite helpers (init, insert, query, prune)
  - `clip` — bridges to PowerShell/clip.exe for read/write
  - `transform` — basic text transforms
  - `util` — logging and helpers
  - `platform_win` — process spawn / platform quirks
