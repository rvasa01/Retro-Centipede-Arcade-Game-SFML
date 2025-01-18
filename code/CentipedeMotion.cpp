/*
Author: Rahil Vasa
Class: ECE4122 
Last Date Modified: 09/29/2024

Description:
This file implements a game where the player controls a spaceship to shoot centipedes, spiders, and destroy mushrooms while avoiding collisions.
*/

#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <list>
#include <random>
#include <sstream>  

// Global constants for screen dimensions and settings
const int SCREEN_WIDTH = 1036;
const int SCREEN_HEIGHT = 569;
const int Y_DISPLACEMENT = 25;    // Vertical displacement when moving down or up
const float SHIP_SPEED = 300.f;   // Speed of the spaceship
const float LASER_SPEED = 600.f;  // Speed of the laser blast
const float SHOT_INTERVAL = 0.6f; // Interval between laser shots in seconds
const float CENTIPEDE_SPEED = 450.f; // Speed of the centipede
const float SPIDER_SPEED = 200.f;  // Speed of the spider
const int TOP_BUFFER = 50;        // Top buffer area without mushrooms
const int BOTTOM_BUFFER = 100;    // Bottom buffer area without mushrooms

// respawn clock for the spider
sf::Clock spiderRespawnClock;

// Declaration of the normalize function
sf::Vector2f normalize(sf::Vector2f v);

// Function to calculate the distance between two points
float distance(sf::Vector2f a, sf::Vector2f b) {
    return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

// Function to normalize a vector
sf::Vector2f normalize(sf::Vector2f v) {
    float mag = std::sqrt(v.x * v.x + v.y * v.y);
    if (mag == 0) return sf::Vector2f(0, 0);
    return sf::Vector2f(v.x / mag, v.y / mag);
}

/*
The Mushroom struct represents a mushroom in the game. 
It stores information like its sprite, position on the screen, 
and whether it is small or not. 
*/
struct Mushroom {
    sf::Sprite sprite;
    sf::Vector2f position;
    bool isSmall;

    Mushroom(sf::Texture& texture, float x, float y, bool small = false)
        : isSmall(small) {
        sprite.setTexture(texture);
        sprite.setPosition(x, y);
        position = {x, y};
    }
};

/*
The EceCentipede class represents the centipede enemy in the game. 
It can move across the screen, change direction when it hits the boundary or a mushroom, 
and can split into multiple centipedes when hit by a laser. This class handles all movement and collision logic for the centipede.
*/
class ECE_Centipede : public sf::Sprite {
public:
    /*
    This is the constructor for the centipede. It initializes the head of the centipede and then adds several body segments behind it. The
    inputs are the textures for the head and body of the centipede, the number of segments to create, and the starting position of the Centipede. 
    */
    ECE_Centipede(sf::Texture& headTexture, sf::Texture& bodyTexture, int numSegments, sf::Vector2f startPosition = {0.f, 0.f})
        : headTexture(headTexture), bodyTexture(bodyTexture), followDistance(30.f), direction(1.f, 0.f), moveDirectionDown(true),
          leftBound(0.f), rightBound(SCREEN_WIDTH), lowerBound(SCREEN_HEIGHT) {

        // Initialize the head
        sf::Sprite head(headTexture);
        head.setPosition(startPosition);
        segments.push_back(head);

        // Initialize and create the body segments behind the head. 
        for (int i = 1; i < numSegments; ++i) {
            sf::Sprite body(bodyTexture);
            body.setPosition(startPosition.x, startPosition.y + i * followDistance);
            segments.push_back(body);
        }
    }
    /*
    The update function is called every frame to move the centipede across the screen. 
    It handles moving the head of the centipede based on its direction and then makes sure each body segment follows the one before it.
    It also ensures the centipede changes direction when it reaches the boundary of the screen. This function takes deltaTime as input, 
    which controls the speed of movement based on the time since the last update.
    */
    void update(float deltaTime) {
        // Move the head based on the current direction
        segments[0].move(direction * CENTIPEDE_SPEED * deltaTime);

        // Check for collisions with screen boundaries
        checkBounds();

        // Update the rest of the segments to follow the head
        for (int i = 1; i < segments.size(); ++i) {
            sf::Vector2f dir = segments[i - 1].getPosition() - segments[i].getPosition();
            float dist = distance(segments[i - 1].getPosition(), segments[i].getPosition());

            if (dist > followDistance) {
                segments[i].move(normalize(dir) * (CENTIPEDE_SPEED * deltaTime));
            }
        }
    }

    /*
    The draw function draws each segment of the centipede on it. 
    This is called each frame to ensure the centipede appears on the screen.
    */
    void draw(sf::RenderWindow& window) {
        // Draw all segments
        for (auto& segment : segments) {
            window.draw(segment);
        }
    }

    /*
    This function checks if the head of the centipede has collided with a mushroom.
    If a collision is detected, the centipede changes direction to simulate it hitting an obstacle.
    This function takes a list of mushrooms to check for collisions.
    */
    void checkMushroomCollision(std::list<Mushroom>& mushrooms) {
        for (auto& mushroom : mushrooms) {
            if (segments[0].getGlobalBounds().intersects(mushroom.sprite.getGlobalBounds())) {
                // Change direction when the head collides with a mushroom
                direction = sf::Vector2f(-direction.x, direction.y);
                moveVertically(segments[0].getPosition());
                break;
            }
        }
    }


    /*
    This function checks if the centipede has been hit by a laser shot.
    If it gets hit, it either removes the head, splits the centipede into two, or simply removes the body segment that got hit.
    The score is updated accordingly. If the centipede is completely destroyed, it returns true. Inputs are laserBounds for checking 
    collision, list of centipedes, score for updating the player's points. Returns true if the centipede is fully destroyed, false otherwise.
    */
    bool checkLaserCollision(const sf::FloatRect& laserBounds, std::list<ECE_Centipede>& centipedes, int& score) {
        for (int i = 0; i < segments.size(); ++i) {
            if (segments[i].getGlobalBounds().intersects(laserBounds)) {
                if (i == 0) {
                    // If the head is hit, remove it and make the next segment the new head
                    segments.erase(segments.begin());
                    score += 100; // Increment score by 100 for hitting the head
                } else if (i == segments.size() - 1) {
                    // If the tail is hit, just remove it
                    segments.pop_back();
                    score += 10; // Increment score by 10 for hitting a body segment
                } else {
                    // If a body segment is hit, split the centipede into two
                    std::vector<sf::Sprite> newSegments1(segments.begin(), segments.begin() + i);
                    std::vector<sf::Sprite> newSegments2(segments.begin() + i + 1, segments.end());

                    if (!newSegments1.empty()) {
                        newSegments1[0].setTexture(headTexture);
                        centipedes.emplace_back(headTexture, bodyTexture, newSegments1.size());
                        centipedes.back().segments = newSegments1;
                        centipedes.back().direction = direction; // Keep same direction for first half
                    }

                    if (!newSegments2.empty()) {
                        newSegments2[0].setTexture(headTexture);
                        centipedes.emplace_back(headTexture, bodyTexture, newSegments2.size());
                        centipedes.back().segments = newSegments2;
                        centipedes.back().direction = -direction; // Reverse direction for the second half
                    }

                    score += 10; // Increment score by 10 for hitting a body segment

                    // Remove the original centipede as it has been split
                    return true;
                }
                return segments.empty(); // Return true if centipede is fully destroyed
            }
        }
        return false;
    }

    /*
    This function checks if the centipede collides with the spaceship.
    If a collision occurs, it returns true, which is used to handle the player's lives.
    - Input: spaceshipBounds, which defines the area occupied by the spaceship.
    - Output: Returns true if there is a collision, false otherwise.
    */
    bool checkSpaceshipCollision(const sf::FloatRect& spaceshipBounds) {
        for (auto& segment : segments) {
            if (segment.getGlobalBounds().intersects(spaceshipBounds)) {
                return true; // Collision detected
            }
        }
        return false;
    }

    std::vector<sf::Sprite> segments;

private:
    sf::Texture& headTexture;
    sf::Texture& bodyTexture;
    float followDistance;              // Distance each segment tries to maintain from the one in front
    sf::Vector2f direction;            // Current direction of movement
    bool moveDirectionDown;            // True if moving down, false if moving up
    const float leftBound;             // Left boundary for movement
    const float rightBound;            // Right boundary for movement
    const float lowerBound;            // Lower boundary for movement

    /*
    This function moves the head vertically, either up or down, based on the current direction.
    It is used when the centipede changes direction after hitting an obstacle or boundary.
    - Input: headPosition, which is the current position of the head segment.
    */
    void moveVertically(sf::Vector2f headPosition) {
        if (moveDirectionDown) {
            segments[0].setPosition(headPosition.x, headPosition.y + Y_DISPLACEMENT);
        } else {
            segments[0].setPosition(headPosition.x, headPosition.y - Y_DISPLACEMENT);
        }

        if (headPosition.y <= 0) {
            moveDirectionDown = false;
        }

        if ((headPosition.y + 27) >= lowerBound) {
            moveDirectionDown = true;
        }
    }

    /*
    This function checks if the centipede head has reached the boundaries of the screen.
    If it does, it changes the movement direction accordingly.
    */
    void checkBounds() {
        sf::Vector2f headPosition = segments[0].getPosition();
        if (headPosition.x <= leftBound) {
            direction = sf::Vector2f(1.f, 0.f);
            moveVertically(headPosition);
        } else if ((headPosition.x + 27) >= rightBound) {
            direction = sf::Vector2f(-1.f, 0.f);
            moveVertically(headPosition);
        }

        if (headPosition.y <= 0) {
            moveDirectionDown = true;
        } else if ((headPosition.y + Y_DISPLACEMENT + 27) > lowerBound) {
            moveDirectionDown = false;
        }
    }
};

// Laser class for firing laser shots from spaceship
class ECE_LaserBlast : public sf::Sprite {
public:
    // - Input: laserTexture for the visual representation, and x and y for the starting position.
    ECE_LaserBlast(sf::Texture& laserTexture, float x, float y) {
        this->setTexture(laserTexture);
        this->setPosition(x, y);
    }

    // - Input: deltaTime controls how much the laser moves based on elapsed time.
    void update(float deltaTime) {
        // Move the laser upwards
        this->move(0, -LASER_SPEED * deltaTime);
    }

    /*
    Checks if the laser has moved off the top of the screen.
    - Output: Returns true if the laser is off-screen, false otherwise.
    */
    bool isOffScreen() {
        // Check if the laser has moved off the top of the screen
        return this->getPosition().y < 0;
    }
};

// Spider class for randomly moving spider
class Spider : public sf::Sprite {
public:
    /*
    Represents a spider in the game. It moves in a random direction on the screen 
    and can collide with mushrooms, the player's spaceship, or get shot by a laser.
    - Input: spiderTexture: The texture for the spider.
    */
    Spider(sf::Texture& spiderTexture) {
        this->setTexture(spiderTexture);
        this->setPosition(static_cast<float>(rand() % SCREEN_WIDTH), static_cast<float>(rand() % SCREEN_HEIGHT / 2));
        direction = sf::Vector2f((rand() % 2 ? 1.f : -1.f), (rand() % 2 ? 1.f : -1.f));
        isAlive = true;
    }

    /*
    Update the spider's position based on the direction it is moving.
    Changes direction if it hits the boundaries of the screen.
    - Input: deltaTime controls how much the spider moves based on the time passed since the last update.
    */
    void update(float deltaTime) {
        if (isAlive) {
            this->move(direction * SPIDER_SPEED * deltaTime);

            // Change direction if hitting boundaries
            sf::Vector2f position = this->getPosition();
            if (position.x <= 0 || position.x + this->getGlobalBounds().width >= SCREEN_WIDTH) {
                direction.x = -direction.x;
            }
            if (position.y <= 0 || position.y + this->getGlobalBounds().height >= SCREEN_HEIGHT) {
                direction.y = -direction.y;
            }
        }
    }

    /*
    Checks if the spider has collided with a mushroom. 
    If there is a collision, the mushroom is destroyed.
    - Input: mushrooms is a list of mushrooms to check for collisions.
    - Output: Returns true if a mushroom is destroyed.
    */
    bool checkMushroomCollision(std::list<Mushroom>& mushrooms) {
        if (isAlive) {
            for (auto mushroomIt = mushrooms.begin(); mushroomIt != mushrooms.end();) {
                if (this->getGlobalBounds().intersects(mushroomIt->sprite.getGlobalBounds())) {
                    mushroomIt = mushrooms.erase(mushroomIt);
                    return true; // Destroy mushroom on collision
                } else {
                    ++mushroomIt;
                }
            }
        }
        return false;
    }

    /*
    Check if the spider collides with the player's spaceship.
    - Input: spaceshipBounds is the area covered by the spaceship.
    - Output: Returns true if the spider collides with the spaceship, false otherwise.
    */
    bool checkSpaceshipCollision(const sf::FloatRect& spaceshipBounds) {
        return isAlive && this->getGlobalBounds().intersects(spaceshipBounds);
    }

    /*
    Check if the spider is hit by a laser.
    If hit, the spider is set to not alive and the score is incremented.
    - Input: laserBounds is the area covered by the laser, score is the player's score to be updated.
    - Output: Returns true if the spider is hit by a laser.
    */
    bool checkLaserCollision(const sf::FloatRect& laserBounds, int& score) {
        if (isAlive && this->getGlobalBounds().intersects(laserBounds)) {
            isAlive = false;
            spiderRespawnClock.restart();
            score += 300; // Increment score by 300 when hitting the spider
            return true;
        }
        return false;
    }

    /*
    Set the alive state of the spider.
    - Input: alive, which is either true (alive) or false (not alive).
    */
    void setIsAlive(bool alive) {
        isAlive = alive;
    }
    
    /*
    Get the current alive state of the spider.
    - Output: Returns true if the spider is alive, false otherwise.
    */
    bool getIsAlive() const {
        return isAlive;
    }

private:
    sf::Vector2f direction; // Direction of movement
    bool isAlive;           // State of the spider
};

int main() {
    // Window setup
    sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Smooth Centipede Movement");

    // Load textures for centipede head, body, mushrooms, spaceship, laser, spider, and startup screen
    sf::Texture centipedeHeadTexture;
    sf::Texture centipedeBodyTexture;
    sf::Texture mushroomTexture;
    sf::Texture mushroomSmallTexture;
    sf::Texture starshipTexture;
    sf::Texture laserTexture;
    sf::Texture spiderTexture;
    sf::Texture startScreenTexture;

    if (!centipedeHeadTexture.loadFromFile("graphics/CentipedeHead.png") ||
        !centipedeBodyTexture.loadFromFile("graphics/CentipedeBody.png") ||
        !mushroomTexture.loadFromFile("graphics/Mushroom0.png") ||
        !mushroomSmallTexture.loadFromFile("graphics/Mushroom1.png") ||
        !starshipTexture.loadFromFile("graphics/StarShip.png") ||
        !laserTexture.loadFromFile("graphics/LaserClass.png") ||
        !spiderTexture.loadFromFile("graphics/spider.png") ||
        !startScreenTexture.loadFromFile("graphics/Startup Screen BackGround.png")) {
        return -1;
    }

    // Create start screen sprite
    sf::Sprite startScreenSprite;
    startScreenSprite.setTexture(startScreenTexture);

    // Wait for the user to press Enter to start the game
    bool startGame = false;
    while (window.isOpen() && !startGame) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                startGame = true;
            }
        }

        window.clear();
        window.draw(startScreenSprite);
        window.display();
    }

    // Create the centipede list
    std::list<ECE_Centipede> centipedes;
    centipedes.emplace_back(centipedeHeadTexture, centipedeBodyTexture, 12);

    // Create a list of mushrooms using a random generator
    std::list<Mushroom> mushrooms;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> xDist(0, SCREEN_WIDTH - 100);
    std::uniform_int_distribution<> yDist(TOP_BUFFER, SCREEN_HEIGHT - BOTTOM_BUFFER);

    for (int i = 0; i < 30; ++i) {
        float x = static_cast<float>(xDist(gen));
        float y = static_cast<float>(yDist(gen));
        mushrooms.emplace_back(mushroomTexture, x, y);
    }

    // Create the spaceship sprite
    sf::Sprite spaceship;
    spaceship.setTexture(starshipTexture);
    sf::Vector2f initialPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT - BOTTOM_BUFFER + 20.f);
    spaceship.setPosition(initialPosition); // Initial position for the spaceship

    // Create lives (depicted as spaceship sprites)
    int lives = 2;
    std::vector<sf::Sprite> livesSprites;
    for (int i = 0; i < lives; ++i) {
        sf::Sprite lifeSprite;
        lifeSprite.setTexture(starshipTexture);
        lifeSprite.setPosition(SCREEN_WIDTH - (i + 1) * 50.f - 10.f, 10.f);
        livesSprites.push_back(lifeSprite);
    }

    // Create the spider
    Spider spider(spiderTexture);

    // Create a list of laser shots
    std::list<ECE_LaserBlast> lasers;
    sf::Clock laserClock;

    sf::Clock clock;

    // Score variable
    int score = 0;

    // Font setup for score display
    sf::Font font;
    if (!font.loadFromFile("fonts/KOMIKAP.ttf")) {
        return -1; // Ensure that the font file is available in the specified directory
    }

    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(sf::Color::White);
    scoreText.setPosition(10.f, 10.f);

    // Game Over text
    sf::Text gameOverText;
    gameOverText.setFont(font);
    gameOverText.setCharacterSize(72);
    gameOverText.setFillColor(sf::Color::Red); // Red font for the Game Over text. 
    gameOverText.setString("GAME OVER");
    gameOverText.setPosition(SCREEN_WIDTH / 2.f - 200.f, SCREEN_HEIGHT / 2.f - 50.f);

    // You Win text
    sf::Text youWinText;
    youWinText.setFont(font);
    youWinText.setCharacterSize(72);
    youWinText.setFillColor(sf::Color::Green); // Green font for the You Win text. 
    youWinText.setString("YOU WIN");
    youWinText.setPosition(SCREEN_WIDTH / 2.f - 150.f, SCREEN_HEIGHT / 2.f - 50.f);

    bool gameOver = false;
    bool youWin = false;

    // Main loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Time delta for smooth movement
        float deltaTime = clock.restart().asSeconds();

        if (!gameOver && !youWin) {
            // Update centipedes
            for (auto it = centipedes.begin(); it != centipedes.end();) {
                it->update(deltaTime);
                if (it->segments.empty()) {
                    it = centipedes.erase(it);
                } else {
                    ++it;
                }
            }

            // Check if all centipedes are destroyed (player wins)
            if (centipedes.empty()) {
                youWin = true;
            }

            // Check for collisions between centipedes and mushrooms
            for (auto& centipede : centipedes) {
                centipede.checkMushroomCollision(mushrooms);
            }

            // Update the spider
            if (!spider.getIsAlive() && spiderRespawnClock.getElapsedTime().asSeconds() >= 5.f) { // We want the spider to respawn 5 seconds after its dead. 
                spider.setIsAlive(true);
            }
            spider.update(deltaTime);
            spider.checkMushroomCollision(mushrooms);

            // Move the spaceship with arrow keys
            sf::Vector2f spaceshipPosition = spaceship.getPosition();
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && spaceshipPosition.y > SCREEN_HEIGHT - BOTTOM_BUFFER) {
                spaceship.move(0.f, -SHIP_SPEED * deltaTime); // SHIP_SPEED give you control of how fast you want the ship. 
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && spaceshipPosition.y < SCREEN_HEIGHT - starshipTexture.getSize().y) {
                spaceship.move(0.f, SHIP_SPEED * deltaTime);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && spaceshipPosition.x > 0) {
                spaceship.move(-SHIP_SPEED * deltaTime, 0.f);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && spaceshipPosition.x < SCREEN_WIDTH - starshipTexture.getSize().x) {
                spaceship.move(SHIP_SPEED * deltaTime, 0.f);
            }

            // Shooting lasers with spacebar
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && laserClock.getElapsedTime().asSeconds() > SHOT_INTERVAL) {
                float laserX = spaceshipPosition.x + starshipTexture.getSize().x / 2.f - laserTexture.getSize().x / 2.f;
                float laserY = spaceshipPosition.y;
                lasers.emplace_back(laserTexture, laserX, laserY);
                laserClock.restart();
            }

            // Update lasers and remove those that go off screen
            for (auto it = lasers.begin(); it != lasers.end();) {
                it->update(deltaTime);
                if (it->isOffScreen()) {
                    it = lasers.erase(it);
                } else {
                    ++it;
                }
            }

            // Check for laser collisions with mushrooms
            for (auto laserIt = lasers.begin(); laserIt != lasers.end();) {
                bool laserHit = false;

                for (auto mushroomIt = mushrooms.begin(); mushroomIt != mushrooms.end(); ++mushroomIt) {
                    if (laserIt->getGlobalBounds().intersects(mushroomIt->sprite.getGlobalBounds())) {
                        laserHit = true;

                        if (!mushroomIt->isSmall) {
                            // Change mushroom to smaller one on first hit
                            mushroomIt->sprite.setTexture(mushroomSmallTexture);
                            mushroomIt->isSmall = true;
                        } else {
                            // Remove mushroom from list on second hit
                            mushroomIt = mushrooms.erase(mushroomIt);
                            score += 4;  // Increment score by 4 only when the mushroom is fully destroyed
                        }

                        break; // Laser can only hit one mushroom, so break after collision
                    }
                }

                if (laserHit) {
                    laserIt = lasers.erase(laserIt); // Remove laser after hit
                } else {
                    ++laserIt;
                }
            }

            // Check for laser collisions with centipedes
            for (auto laserIt = lasers.begin(); laserIt != lasers.end();) {
                bool laserHit = false;

                for (auto centipedeIt = centipedes.begin(); centipedeIt != centipedes.end();) {
                    if (centipedeIt->checkLaserCollision(laserIt->getGlobalBounds(), centipedes, score)) {
                        centipedeIt = centipedes.erase(centipedeIt);
                        laserHit = true;
                        break;
                    } else {
                        ++centipedeIt;
                    }
                }

                if (laserHit) {
                    laserIt = lasers.erase(laserIt);
                } else {
                    ++laserIt;
                }
            }

            // Check for laser collisions with spider
            for (auto laserIt = lasers.begin(); laserIt != lasers.end();) {
                if (spider.checkLaserCollision(laserIt->getGlobalBounds(), score)) {
                    laserIt = lasers.erase(laserIt);
                } else {
                    ++laserIt;
                }
            }

            // Check for collisions between centipedes and spaceship
            for (auto& centipede : centipedes) {
                if (centipede.checkSpaceshipCollision(spaceship.getGlobalBounds())) {
                    lives--;
                    livesSprites.pop_back();
                    spaceship.setPosition(initialPosition); // Respawn spaceship to initial position
                    if (lives == 0) {
                        gameOver = true;
                    }
                    break; // Stop checking after a collision
                }
            }

            // Check for collisions between spider and spaceship
            if (spider.getIsAlive() && spider.checkSpaceshipCollision(spaceship.getGlobalBounds())) {
                if (lives > 0) {
                    lives--; // Decrease life count by 1
                    livesSprites.pop_back(); // Remove one life icon from the screen
                    spaceship.setPosition(initialPosition); // Respawn spaceship to initial position

                    if (lives == 0) {
                        gameOver = true; // Set game over only when all lives are lost
                    }
                }
            }

            // Update score display text
            std::stringstream ss;
            ss << "Score: " << score;
            scoreText.setString(ss.str());
        }

        // Rendering
        window.clear();

        if (youWin) {
            // Draw "YOU WIN" screen
            window.draw(youWinText);
        } else if (gameOver) {
            // Draw game over screen
            window.draw(gameOverText);
        } else {
            // Draw mushrooms
            for (auto& mushroom : mushrooms) {
                window.draw(mushroom.sprite);
            }

            // Draw centipedes
            for (auto& centipede : centipedes) {
                centipede.draw(window);
            }

            // Draw spaceship if not game over
            window.draw(spaceship);

            // Draw spider if alive
            if (spider.getIsAlive()) {
                window.draw(spider);
            }

            // Draw lasers
            for (auto& laser : lasers) {
                window.draw(laser);
            }

            // Draw score
            window.draw(scoreText);

            // Draw lives
            for (auto& lifeSprite : livesSprites) {
                window.draw(lifeSprite);
            }
        }

        window.display();
    }

    return 0;
}
