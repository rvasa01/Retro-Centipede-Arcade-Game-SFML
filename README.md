# Retro-Centipede-Arcade-Game-SFML

# Retro Centipede Arcade Game

This project is a simplified recreation of the classic arcade game **Centipede**, built as part of a coursework assignment for ECE4122/6122. It uses **SFML (Simple and Fast Multimedia Library)** for 2D graphics and includes class-based design principles in C++.

## Features
- **Game Objective**: Destroy all the centipede segments or survive until lives run out.
- **Gameplay Elements**:
  - A centipede with 11 body segments and one head moves through a mushroom-filled play area.
  - A spaceship controlled by the player at the bottom of the screen fires laser blasts to destroy the centipede and mushrooms.
  - Randomly placed mushrooms in the game area act as obstacles and are progressively destroyed by laser blasts or spiders.
  - A spider moves randomly, destroying mushrooms and attacking the spaceship.
- **Score Display**: The game shows the score and remaining lives at the top of the screen.
- **Keyboard Controls**:
  - Arrow keys for movement.
  - Spacebar for firing laser blasts.
- **Game Reset**: Once the game ends, it returns to the start screen.

## Technical Details
- **Classes**:
  - `ECE_Centipede`: Manages the centipede's position, segmentation upon being hit, and collision detection.
  - `ECE_LaserBlast`: Tracks the laser blasts' position and collision interactions with objects.
- **Randomization**: Mushrooms are randomly placed in the game area using `std::uniform_int_distribution`.
- **Collision Handling**: Detects and responds to collisions between the centipede, mushrooms, laser blasts, and the spider.
- **Game States**: Tracks score, remaining lives, and transitions between start and gameplay screens.
- **Graphics Management**: Separate folder for graphics assets required for the game.

## How to Play
1. Clone the repository and ensure you have SFML installed.
2. Compile the project using the provided `CMakeLists.txt`.
3. Run the game, and use the arrow keys to move the spaceship and the spacebar to shoot.
4. Aim to destroy all the centipede segments while avoiding the spider and managing your lives.

## Folder Structure
```plaintext
Lab1/
├── CMakeLists.txt
├── code/
│   ├── main.cpp
│   ├── ECE_Centipede.h
│   ├── ECE_Centipede.cpp
│   ├── ECE_LaserBlast.h
│   ├── ECE_LaserBlast.cpp
│   └── ...
├── graphics/
│   ├── centipede.png
│   ├── mushroom.png
│   ├── spaceship.png
│   └── ...
