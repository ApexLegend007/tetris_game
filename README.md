# Tetris Game

A terminal-based Tetris game implemented in C++ with multithreading, file handling for a leaderboard system, and optimized UI rendering.

## Features
- **Classic Tetris Gameplay**: Rotate, move, and drop tetrominoes to clear lines and score points.
- **Leaderboard System**: Tracks top 5 high scores and stores them in `leaderboard.txt`.
- **Multithreading for Smooth Gameplay**: Handles block falling and user input simultaneously.
- **Terminal UI Adaptation**: Dynamically adjusts the game board based on terminal size.
- **High Score Tracking**: Saves the highest score in `~/.local/share/Tetris highscore`.

## Installation
### Prerequisites
- **C++ Compiler**: Ensure `g++` is installed.
- **Make** (Optional): To use `Makefile` for easy compilation.

### Compilation
```sh
# Compile the Tetris game
 g++ -o tetris main.cpp -pthread
```

### Running the Game
```sh
./tetris
```

## Controls
- `Arrow Keys` → Move tetromino left/right.
- `Up Arrow` → Rotate tetromino.
- `Down Arrow` → Drop tetromino faster.
- `Spacebar` → Instantly drop the tetromino.
- `P` → Pause/Resume game.
- `ESC` → Exit the game.

## Leaderboard System
- The leaderboard is stored in `leaderboard.txt`.
- Only the **top 5 scores** are displayed.
- Scores are automatically saved at the end of each game.

## Future Improvements
- Implement **wall-kick** for better tetromino rotation.
- Add **event-driven input handling** to optimize CPU usage.
- Enhance **pause screen UI** for better user feedback.
- Introduce **animations and sound effects**.

## License
This project is open-source under the **MIT License**.
