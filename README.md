Copy these into your openFrameworks app’s src folder:

- `main.cpp`
- `ofApp.h`
- `ofApp.cpp`
- `Chess.h`
- `Chess.cpp`

Also useful: `README.md` (this file).

Copy-paste setup (Windows / PowerShell)


1. Run **projectGenerator.exe** from your OF folder (e.g. `C:\of_v0.12.0_vs_release\projectGenerator\`).
2. **Import** path: point to your OF root.
3. **Name:** e.g. `chessApp`, **Path:** `apps\myApps\`.
4. Click **UPDATE** (or Generate). This creates `...\apps\myApps\chessApp\`.

Copy source files from this repo into src

Edit the two paths, then paste into PowerShell:

powershell
# --- edit these two lines ---
$ChessRepo = "C:\path\to\Chess"
$OpenFrameworksRoot = "C:\path\to\of_v0.12.0_vs_release"
