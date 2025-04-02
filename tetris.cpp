#include <iostream>
#include <string>
#include <fstream>
#include <signal.h>
#include <thread>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <atomic>

using namespace std;
using namespace std::chrono;

// Structure for leaderboard entries
struct ScoreEntry {
    string name;
    int score;
};

// Tetromino class: Represents a single tetromino with shape, position, and behavior
class Tetromino {
public:
    bool shape[4][4];
    int x, y;
    char color;
    int id;
    int centerX, centerY;

    Tetromino() : x(3), y(0), color(1), id(0), centerX(0), centerY(0) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                shape[i][j] = 0;
    }

    void setShape(const bool newShape[4][4], char newColor, int newId, int cx, int cy) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                shape[i][j] = newShape[i][j];
        color = newColor;
        id = newId;
        centerX = cx;
        centerY = cy;
    }

    void rotate() {
        bool temp[4][4];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                temp[j][i] = shape[3 - i][j];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                shape[i][j] = temp[i][j];
    }
};

// GameBoard class: Manages the game grid and core mechanics
class GameBoard {
public:
    static const int WIDTH = 10;
    static const int HEIGHT = 20;
    char grid[WIDTH][HEIGHT];

    GameBoard() { clear(); }

    void clear() {
        for (int i = 0; i < WIDTH; i++)
            for (int j = 0; j < HEIGHT; j++)
                grid[i][j] = 0;
    }

    bool checkCollision(const Tetromino& t, int dx = 0, int dy = 0) const {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (t.shape[i][j]) {
                    int newX = t.x + j + dx;
                    int newY = t.y + i + dy;
                    if (newX < 0 || newX >= WIDTH || newY >= HEIGHT || (newY >= 0 && grid[newX][newY] != 0))
                        return true;
                }
        return false;
    }

    void solidify(const Tetromino& t) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                if (t.shape[i][j])
                    grid[t.x + j][t.y + i] = t.color;
    }

    int clearLines() {
        int linesCleared = 0;
        for (int y = HEIGHT - 1; y >= 0; y--) {
            bool full = true;
            for (int x = 0; x < WIDTH; x++)
                if (grid[x][y] == 0) {
                    full = false;
                    break;
                }
            if (full) {
                linesCleared++;
                for (int yy = y; yy > 0; yy--)
                    for (int x = 0; x < WIDTH; x++)
                        grid[x][yy] = grid[x][yy - 1];
                for (int x = 0; x < WIDTH; x++)
                    grid[x][0] = 0;
                y++;
            }
        }
        return linesCleared;
    }

    bool isGameOver() const {
        for (int x = 0; x < WIDTH; x++)
            if (grid[x][0] != 0)
                return true;
        return false;
    }
};

// Game class: Manages game logic, state, and user interaction
class Game {
private:
    GameBoard board;
    Tetromino current, next;
    int score = 0, highscore = 0;
    bool gameOver = false;
    bool paused = false;
    atomic<bool> fall{false};
    string playerName;
    vector<ScoreEntry> leaderboard;
    atomic<bool> inputActive{true}; // Control input thread

    vector<vector<vector<bool>>> shapes = {
        {{1, 1, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // O
        {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}}, // I
        {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // S
        {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, // Z
        {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}}, // L
        {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}, // J
        {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}  // T
    };
    vector<int> colors = {6, 3, 1, 2, 4, 5, 7};
    vector<int> centersX = {0, 1, 1, 1, 1, 1, 1};
    vector<int> centersY = {0, 1, 1, 1, 1, 1, 1};

    const int BASE_FALL_DELAY = 1000;
    const int SCORE_PER_LINE = 100;
    const int MAX_LEADERBOARD_SIZE = 5;

    void fallThread() {
        while (true) {
            this_thread::sleep_for(milliseconds(getFallDelay()));
            fall = true;
        }
    }

public:
    Game() {
        loadLeaderboard();
        srand(time(0));
        spawnNext();
        spawnNext();
    }

    void enableEcho(struct termios& old) {
        struct termios current;
        tcgetattr(0, &current);
        current.c_lflag |= ECHO;
        current.c_lflag |= ICANON;
        tcsetattr(0, TCSANOW, &current);
    }

    void disableEcho(struct termios& old) {
        tcsetattr(0, TCSANOW, &old);
    }

    void loadLeaderboard() {
        ifstream file("leaderboard.txt");
        if (file.is_open()) {
            ScoreEntry entry;
            while (file >> entry.name >> entry.score)
                leaderboard.push_back(entry);
            file.close();
        }
    }

    void saveLeaderboard() {
        ofstream file("leaderboard.txt");
        for (const auto& entry : leaderboard)
            file << entry.name << " " << entry.score << endl;
        file.close();
    }

    void updateLeaderboard() {
        leaderboard.push_back({playerName, score});
        sort(leaderboard.begin(), leaderboard.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
            return a.score > b.score;
        });
        if (leaderboard.size() > MAX_LEADERBOARD_SIZE) leaderboard.resize(MAX_LEADERBOARD_SIZE);
        saveLeaderboard();
    }

    void displayLeaderboard() const {
        system("clear");
        cout << "\n==== Leaderboard ====\n";
        for (const auto& entry : leaderboard)
            cout << entry.name << " - " << entry.score << "\n";
        if (leaderboard.empty()) cout << "No scores yet!\n";
        cout << "====================\n";
    }

    void displayInstructions() const {
        cout << "\n=== Tetris Instructions ===\n";
        cout << "Controls:\n";
        cout << "  h / Left Arrow  : Move Left\n";
        cout << "  l / Right Arrow : Move Right\n";
        cout << "  j / Down Arrow  : Move Down\n";
        cout << "  k / Up Arrow    : Rotate\n";
        cout << "  Space           : Drop\n";
        cout << "  p / ESC         : Pause/Unpause\n";
        cout << "  Space (Game Over): Restart\n";
        cout << "Goal: Clear lines to score points. Game ends if blocks reach the top.\n";
        cout << "===========================\n";
    }

    void spawnNext() {
        current = next;
        int id = rand() % shapes.size();
        bool shapeArray[4][4];
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                shapeArray[i][j] = shapes[id][i][j];
        next.setShape(shapeArray, colors[id], id, centersX[id], centersY[id]);
    }

    void handleInput(char input) {
        if (input == 'p' || input == '\e') {
            paused = !paused;
            return;
        }
        if (gameOver || paused) {
            if (input == ' ' && gameOver) {
                reset();
                paused = false;
            }
            return;
        }

        if (input == 'h' || input == 20) {
            if (!board.checkCollision(current, -1, 0)) current.x--;
        } else if (input == 'l' || input == 19) {
            if (!board.checkCollision(current, 1, 0)) current.x++;
        } else if (input == 'j' || input == 18) {
            if (!board.checkCollision(current, 0, 1)) current.y++;
            else solidifyAndSpawn();
        } else if (input == 'k' || input == 17) {
            Tetromino temp = current;
            temp.rotate();
            if (!board.checkCollision(temp)) current = temp;
        } else if (input == ' ') {
            while (!board.checkCollision(current, 0, 1)) current.y++;
            solidifyAndSpawn();
        }
    }

    void solidifyAndSpawn() {
        board.solidify(current);
        score += board.clearLines() * SCORE_PER_LINE;
        if (board.isGameOver()) {
            gameOver = true;
            if (score > highscore) highscore = score;
            updateLeaderboard();
        } else {
            spawnNext();
        }
    }

    void update() {
        if (!gameOver && !paused && fall) {
            if (board.checkCollision(current, 0, 1))
                solidifyAndSpawn();
            else
                current.y++;
            fall = false;
        }
    }

    void draw() {
        string buffer = "\e[1H\e[?25l" + string("\n", 5) + "┌────────────────────┬────────────┐\n";
        for (int y = 0; y < GameBoard::HEIGHT; y++) {
            buffer += "│";
            for (int x = 0; x < GameBoard::WIDTH; x++) {
                char color = board.grid[x][y];
                if (!gameOver && !paused)
                    for (int i = 0; i < 4; i++)
                        for (int j = 0; j < 4; j++)
                            if (current.shape[i][j] && current.x + j == x && current.y + i == y)
                                color = current.color;
                buffer += (color ? "\e[4" + to_string(color) + "m  \e[0m" : "- ");
            }
            buffer += "│";
            if (y < 5) {
                buffer += "  ";
                for (int j = 0; j < 4; j++)
                    buffer += (next.shape[y][j] ? "\e[4" + to_string(next.color) + "m  \e[0m" : "- ");
            } else if (y == 7) buffer += " score: " + to_string(score);
            else if (y == 8) buffer += " High score: " + to_string(highscore);
            buffer += "\n";
        }
        buffer += "└────────────────────┘\n";
        if (gameOver) buffer += "    GAME OVER\n";
        else if (paused) buffer += "    PAUSED - Press p to Resume\n";
        cout << buffer;
    }

    void reset() {
        board.clear();
        score = 0;
        gameOver = false;
        spawnNext();
        spawnNext();
    }

    void menu(struct termios& oldTerm) {
        int choice;
        while (true) {
            system("clear");
            cout << "\n=== TETRIS GAME MENU ===\n";
            cout << "1. Start Game\n";
            cout << "2. View Leaderboard\n";
            cout << "3. Instructions\n";
            cout << "4. Exit\n";
            cout << "Choose an option: ";
            enableEcho(oldTerm);
            cin >> choice;
            disableEcho(oldTerm);

            if (cin.fail()) {
                cout << "Invalid input. Please enter a number.\n";
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                this_thread::sleep_for(seconds(1));
                continue;
            }

            switch (choice) {
                case 1:
                    system("clear");
                    displayLeaderboard();
                    cout << "Press Enter to start...\n";
                    enableEcho(oldTerm);
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cin.get();
                    disableEcho(oldTerm);
                    return;
                case 2:
                    displayLeaderboard();
                    cout << "Press Enter to return...\n";
                    enableEcho(oldTerm);
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cin.get();
                    disableEcho(oldTerm);
                    break;
                case 3:
                    displayInstructions();
                    cout << "Press Enter to return...\n";
                    enableEcho(oldTerm);
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cin.get();
                    disableEcho(oldTerm);
                    break;
                case 4:
                    exit(0);
                default:
                    cout << "Invalid choice. Try again.\n";
                    this_thread::sleep_for(seconds(1));
            }
        }
    }

    bool handleGameOver(struct termios& oldTerm) {
        inputActive = false; // Pause input thread
        system("clear");
        displayLeaderboard();
        cout << "Your score: " << score << "\n";
        cout << "Game Over! Do you want to restart? (1 = Yes, 0 = No): ";
        enableEcho(oldTerm);

        // Clear any residual input
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        int choice;
        cin >> choice;
        disableEcho(oldTerm);
        inputActive = true; // Resume input thread

        if (cin.fail() || (choice != 0 && choice != 1)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Choose an option:\n";
            cout << "1. Restart\n2. Quit\nOption: ";
            enableEcho(oldTerm);
            cin >> choice;
            disableEcho(oldTerm);
            if (cin.fail() || (choice != 1 && choice != 2)) {
                cout << "Invalid input. Quitting.\n";
                return false;
            }
            return (choice == 1);
        }
        return (choice == 1);
    }

    void setPlayerName(const string& name) { playerName = name; }

    bool isRunning() const { return !gameOver || paused; }

    int getFallDelay() const {
        return max(BASE_FALL_DELAY - score * 20, 100);
    }

    void startFallThread() {
        thread(&Game::fallThread, this).detach();
    }

    void addTetromino(const vector<vector<bool>>& newShape, int newColor, int cx, int cy) {
        shapes.push_back(newShape);
        colors.push_back(newColor);
        centersX.push_back(cx);
        centersY.push_back(cy);
    }
};

// Input thread: Captures raw input with pause control
atomic<char> input{0};
atomic<bool> inputActive{true}; // Global flag to control input thread
void inputThread() {
    while (true) {
        if (inputActive) {
            char tmp[3] = {0};
            ssize_t bytesRead = read(0, tmp, 3);
            if (bytesRead > 0) {
                if (tmp[0] == '\e' && tmp[1] == '[') {
                    input = tmp[2] - 48; // Arrow keys (17-20)
                } else {
                    input = tmp[0]; // Regular keys
                }
            }
        }
        this_thread::sleep_for(milliseconds(10));
    }
}

int main() {
    Game game;
    struct termios old, current;
    tcgetattr(0, &old);
    current = old;
    current.c_lflag &= ~ICANON;
    current.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &current);

    cout << "Enter your name: ";
    game.enableEcho(old);
    string name;
    cin >> name;
    game.setPlayerName(name);
    game.disableEcho(old);

    game.menu(old);

    tcsetattr(0, TCSANOW, &current);
    system("clear");

    thread(inputThread).detach();
    game.startFallThread();

    while (true) {
        while (game.isRunning()) {
            game.handleInput(input);
            input = 0;
            game.update();
            game.draw();
            this_thread::sleep_for(milliseconds(10));
        }
        if (!game.handleGameOver(old)) break;
        system("clear");
    }

    tcsetattr(0, TCSANOW, &old);
    system("clear");
    cout << "\e[?25h" << flush;
    return 0;
}
