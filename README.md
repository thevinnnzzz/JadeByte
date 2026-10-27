# Code Quest

**Code Quest** is a 2D programming quiz game built with C++ and the [raylib](https://github.com/raysan5/raylib) game library. It challenges players to test their programming knowledge with 8 multiple-choice questions, a countdown timer, instant feedback, and scoring.

## About

Code Quest takes players through a fast-paced quiz of programming fundamentals. The game was developed with raylib, featuring a custom menu screen, quiz flow, results screen, procedurally generated sound effects, and background music.

- **8 multiple-choice questions** covering programming fundamentals (C++ syntax, data types, loops, operators, functions, containers, arrays, etc.)
- **15-second timer** per question, with a countdown display and warning tick sound in the final 5 seconds
- **Instant feedback** after each answer — correct / incorrect, plus the correct answer
- **Scoring** — 10 points per correct answer, final accuracy and rank

## How to Play

1. Start the game.
2. On the main menu, select **START QUIZ**.
3. Answer each multiple-choice question before the timer runs out.
4. Read the instant feedback after each answer (correct / incorrect, plus the correct answer).
5. After the final question, view the **QUIZ COMPLETE** results screen.
6. Choose **PLAY AGAIN** to restart or **QUIT** to exit.

## Scoring

- You earn **10 points** per correct answer (maximum score: **80**).
- **Accuracy** is the percentage of correctly answered questions.
- Final rank:
  - **80% or higher:** Excellent! Programming star!
  - **50% or higher:** Good job! Keep practicing!
  - **Below 50%:** Keep learning. You can do it!

## Controls

| Input | Action |
| --- | --- |
| Mouse | Select an answer |
| F11 | Toggle fullscreen |
| Window close button / **QUIT** button (results screen) | Exit the game |

## Requirements

- **OS:** Windows 10/11 (64-bit)
- **Graphics:** OpenGL 3.3 or higher
- **Audio:** Sound card / audio output recommended
- **Disk space:** Approximately 4 MB

## Installation

### From GitHub

1. Clone this repository:

```bash
git clone <repository-url>
```

Or download the repository as a ZIP from GitHub and extract it.

2. Make sure the following files are in the project folder:

```
CodeQuest/
├── main.cpp
├── background_music.wav
└── CodeQuest.exe   (only present if the prebuilt exe was committed)
```

3. Run the game.

**Option A – Run the prebuilt game**

If `CodeQuest.exe` is present in the project folder, double-click it, or run it from a terminal:

```bat
CodeQuest.exe
```

**Option B – Build from source** (see [Building from Source](#building-from-source) below).

> **Important:** The repository's `.gitignore` excludes the `CodeQuest.exe` build artifact, so a fresh clone from GitHub will usually **not** include the prebuilt executable. If `CodeQuest.exe` is missing, you must build the game from source.

### Building from Source (Optional)

If you prefer to build the game yourself, you need:

- [raylib](https://github.com/raysan5/raylib) (clone the sources from GitHub)
- A C/C++ compiler such as MinGW GCC (for example, from [MSYS2](https://www.msys2.org/) or [w64devkit](https://github.com/skeeto/w64devkit))
- [CMake](https://cmake.org/) (only needed to build raylib itself)

**Step 1 – Build raylib:**

```bash
git clone https://github.com/raysan5/raylib.git
cd raylib
cmake -B build
cmake --build build
```

**Step 2 – Compile the game:**

From the CodeQuest project folder, run:

```bash
g++ main.cpp -o CodeQuest.exe -IC:/raylib/raylib/src -LC:/raylib/raylib/build -lraylib -lopengl32 -lgdi32 -lwinmm
```

Adjust the paths (`/raylib/raylib/...`) if you cloned or built raylib somewhere else.

Alternatively, if you installed raylib through the MSYS2 package manager, you can build and link in one step:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib
g++ main.cpp -o CodeQuest.exe -i/mingw64/src/raylib -L/mingw64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

### Playing

Once the build (or prebuilt copy) is ready:

```bat
CodeQuest.exe
```

Or simply double-click **CodeQuest.exe**.

## Troubleshooting

- If the game window does not open, update your graphics drivers so OpenGL 3.3 or higher is supported.
- Ensure speakers or headphones are connected for sound effects and music.
- If compilation fails, verify that your MinGW compiler and raylib library are correctly set up.

## License

Code Quest source code is provided for personal use. raylib is licensed under the zlib license.
