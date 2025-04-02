# 🎮 Tetris: Terminal Edition 🚀

Welcome to **Tetris: Terminal Edition**, a slick, retro-inspired Tetris game built in C++ that runs right in your terminal! Powered by multithreading and object-oriented wizardry, this game brings the classic block-stacking action to life with a leaderboard, customizable gameplay, and a vibe that’s equal parts nostalgia and modern cool. Ready to stack some blocks and chase high scores? Let’s dive in!

---

## 🌟 Features That Rock

- **Classic Tetris Vibes**: Rotate, move, and drop tetrominoes to clear lines and score points—pure, unfiltered Tetris fun.
- **Leaderboard Glory**: Compete for the top 5 spots, immortalized in `leaderboard.txt`. Your name, your legacy!
- **Multithreaded Magic**: Smooth block falling and responsive controls, thanks to separate threads for input and game logic.
- **Sick Game Menu**: Start a game, check the leaderboard, peek at instructions, or exit with style—all in one sleek interface.
- **Game Over Swagger**: When the blocks hit the top, see your score, the leaderboard, and choose to restart or bounce out like a boss.
- **Extensible AF**: Add new tetrominoes, tweak speeds, or remix the rules—make it *your* Tetris.

---

## 🛠️ Installation

### Prerequisites

- **C++ Compiler**: `g++` (because who doesn’t love a good compile?).
- **Unix-like System**: Linux or macOS (sorry, Windows warriors—this one’s POSIX-powered).
- **Terminal**: A dope terminal emulator (e.g., GNOME Terminal, iTerm2) for maximum vibes.

### Compilation

Fire up your terminal and let’s get this party started:

```bash
g++ -o tetris tetris.cpp -pthread
Running the Game

Launch it and feel the retro rush:
bash
./tetris
🎯 Controls

Master the blocks with these slick moves:
Action	Keys	Pro Tip
Move Left	h or Left Arrow	Slide those blocks!
Move Right	l or Right Arrow	Keep it flowing!
Move Down	j or Down Arrow	Speed up the drop!
Rotate	k or Up Arrow	Twist it like a pro!
Drop	Spacebar	Slam it down, fast!
Pause/Unpause	p or ESC	Take a breather!
Restart (Game Over)	Spacebar (in-game)	Bounce back quick!
🎲 Gameplay

    Name Drop: Enter your gamer tag—make it legendary.
    Menu Madness: Pick your poison—start, leaderboard, instructions, or exit.
    Stack ‘Em Up: Clear lines (100 points each) and watch the speed crank up as your score climbs.
    Game Over Flex: When it’s over, check your score, the leaderboard, and decide: restart or peace out?

Leaderboard System

    Where: Saved in leaderboard.txt.
    How: Top 5 scores, auto-updated after every game.
    When: Pops up at game start and end—show off your skills!

🔥 Implementation Highlights
Code Structure

    Tetromino Class: The block master—shape, position, color, and spin moves.
    GameBoard Class: The 10x20 arena where the magic happens—collision checks, line clears, and game-over vibes.
    Game Class: The maestro—runs the show with threads, menus, and extensible tetrominoes.

Tech Stack

    Vectors: Dynamic tetromino shapes, colors, and centers—add new blocks anytime!
    2D Array: The game grid, a lean 10x20 battlefield.
    Multithreading: Input and falling blocks run in parallel for that buttery-smooth feel.

Extensibility Hacks

    New Tetrominoes: Drop a new shape into addTetromino()—boom, instant remix.
    Tune the Vibe: Tweak BASE_FALL_DELAY, SCORE_PER_LINE, or MAX_LEADERBOARD_SIZE for your perfect game.
    Control Freestyle: Add new moves in handleInput()—make it wild!

🚀 Upgrades & Cool Factor

    Visual Pop: Color-coded tetrominoes light up your terminal like a neon dream, with a flashy welcome animation.
    Dynamic Speed: The higher your score, the faster it drops—pure adrenaline, adjustable with SPEED_MULTIPLIER.
    Terminal Flex: Clears and redraws for a clean, pro-level UI, with a slick game-over screen.

Future Dope Ideas

    Wall-Kick Moves: Rotate near walls like a Tetris ninja.
    Ghost Piece: See where your block lands—precision mode activated.
    Soundtrack: Terminal beeps for that retro arcade buzz.
    Themes: Swap colors or grid styles—make it your own aesthetic.

🤓 Installation Notes

    Output: leaderboard.txt in your current directory.
    Compile Flag: -pthread for multithreading goodness.

📜 License

Open-source under the MIT License—fork it, tweak it, own it.
