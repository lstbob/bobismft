# ohbobi — Terminal Diet Planner

A CLI diet planner written in C. Create and view meal plans from the terminal with an interactive TUI grid.

## Install

### Prerequisites

- GCC (or any C11 compiler)
- Make
- Linux (uses `termios`, `ioctl`)

### Build

```sh
git clone <repo-url> && cd bobismft
make
```

This produces the `ohbobi` binary in the current directory.

### Add to PATH

**Option A — copy to a directory already on PATH:**

```sh
sudo cp ohbobi /usr/local/bin/
```

**Option B — add the project directory to PATH (add to `~/.bashrc` or `~/.zshrc`):**

```sh
export PATH="$PATH:/path/to/bobismft"
```

Then reload your shell:

```sh
source ~/.bashrc   # or source ~/.zshrc
```

Verify it works:

```sh
ohbobi --version
```

## Usage

### Create a meal plan

```sh
ohbobi nmp
```

Prompts for a date (default: tomorrow), then meal slots (Breakfast, Lunch, Dinner, Snack). Each meal accepts up to 5 ingredients. If a plan already exists for that date, it shows the existing plan and asks whether to overwrite.

**Weekly plan:**

```sh
ohbobi nmp -w
```

Plans 7 days starting from today.

### View meal plans

```sh
ohbobi smp
```

Opens an interactive TUI grid. Navigate with:

| Key | Action |
|---|---|
| `i` | Cycle view (daily → weekly → monthly) |
| `n` | Next (day / week / month) |
| `N` | Previous (day / week / month) |
| `←` `→` | Previous / next day or week |
| `q` | Quit |

**Start at a specific date:**

```sh
ohbobi smp -d 2026-06-17
```

### Supported date formats

- `2026-06-17` (ISO)
- `June 17, 2026` (US long)
- `17 Jun 2026` (European)
- `today`, `tomorrow`
- `next monday`, `next tuesday`, etc.

## Run tests

```sh
make test
```

## Project structure

```
├── main.c              Entry point
├── core/               Business logic
│   ├── include/        Headers
│   ├── src/            Implementation
│   └── tests/          Unit tests
├── renderer/           Terminal UI
│   ├── include/
│   └── src/
└── data/               Meal plan storage (.ohb files)
```

## Data storage

Plans are saved as binary files in `data/` (one `.ohb` file per day). The format includes a magic header (`OHOB`), version byte, XOR checksum, and packed meal records.
