#include <graphics.h>
#include <fonts.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <stdlib.h>
#include <time.h>

extern image_header space_image;
extern image_header space2_image;
extern image_header ship_image;
extern image_header asteroid_image;

struct asteroid 
{
	int position_x;
	int position_y;
    int speed_variations;
};

struct collision 
{
	int position_x_left;
	int position_x_right;
};
// Create Starting Variables
static const int ship_width = 27;
static const int ship_height = 18;
static const int asteroid_width = 10;
static const int asteroid_height = 24;

void addAsteroid(struct asteroid asteroids[], int position) 
{
    asteroids[position].position_x = rand() % (display_width - (asteroid_width / 2)) + (asteroid_width / 2); // Randomize x position
	asteroids[position].position_y = 0 - (asteroid_height / 2);
    asteroids[position].speed_variations = rand() % 20 + 10; // Randomize speed
}

void drawAsteroids(struct asteroid asteroids[], int count) 
{
    for (int i = 0; i < count; i++) 
    {
        draw_image(&asteroid_image, asteroids[i].position_x, asteroids[i].position_y); // Draw asteroid image
    }
}

bool moveAsteroids(struct asteroid asteroids[], int count, float movement, float dt) 
{
    bool output = false;
    for (int i = 0; i < count; i++) 
    {
        asteroids[i].position_y += (movement + asteroids[i].speed_variations) * dt;
        if (asteroids[i].position_y > display_height + (asteroid_height / 2))
        {
            output = true;  // Return If Asteroid has left the screen
        }
    }
    return output;
}

void removeAsteroids(struct asteroid asteroids[], int count)
{
    for (int i = 1; i < count + 1; i++) 
    {
        asteroids[i-1] = asteroids[i];
    }
    asteroids[count].position_x = 0;
    asteroids[count].position_y = 0;
}

bool checkcollision(struct asteroid asteroids[], int count, float x, float y)
{
    // Ass Collision Grid so ship bonds reflect spright and are not a rextangle
    struct collision ship_collision[18] = {{11, 17}, {10, 18}, {10, 18}, {7, 20}, {6, 21}, {5, 22}, {5, 23}, {4, 24}, {3, 24}, {2, 25}, {1, 26}, {2, 25}, {3, 24}, {4, 23}, {4, 23}, {6, 21}, {7, 20}, {8, 19}};
    struct collision asteroid_collision[22] = {{6, 8}, {5, 8}, {5, 8}, {4, 8}, {4, 8}, {4, 9}, {3, 9}, {3, 9}, {3, 9}, {2, 9}, {2, 10}, {2, 10}, {2, 10}, {2, 9}, {2, 9}, {1, 8}, {1, 7}, {2, 7}, {3, 7}, {4, 7}};
    bool output = false;
    for (int i = 0; i < count; i++) // For Each Asteroid
    {
        for (int j = 0; j < 22; j++) // For Each Layer of the Asteroid (y)
        {
            for (int k = 0; k < 18; k++) // For Each Layer of the Ship (y)
            {
                int ship_collision_x = x + ship_collision[k].position_x_left - 1 - (ship_width / 2);
                int asteroid_collision_x = asteroids[i].position_x + asteroid_collision[j].position_x_left - 1 - (asteroid_width / 2);
                int ship_collision_y = y + k - (ship_height / 2);
                int asteroid_collision_y = asteroids[i].position_y + j - (asteroid_height / 2);

                if ((ship_collision_x + ship_collision[k].position_x_right > asteroid_collision_x) && 
                    (ship_collision_x < asteroid_collision_x + asteroid_collision[j].position_x_right) && 
                    (ship_collision_y + 1 > asteroid_collision_y) && 
                    (ship_collision_y < asteroid_collision_y + 1))
                {
                    output = true; // Return If Asteroid has colided with ship
                    goto fullbrakeout;
                }
            }
        }
    }
    fullbrakeout:
    return output;
}

void app_main() //135:240
{
    // Seeding Random Number
    srand(time(NULL));

    // Graphics Start
    graphics_init();
    set_orientation(PORTRAIT);
    setFontColour(255, 255, 255);

    // Button Construction
    gpio_set_direction(35, GPIO_MODE_INPUT);
    gpio_set_direction(0, GPIO_MODE_INPUT);

    // Start High Score Board
    int topscores[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    while (1) // Start Screen
    {
        cls(-1); // Clear screen
        draw_image(&space_image, display_width / 2, display_height / 2); // Draw background image
        
        setFont(FONT_DEJAVU18);
        print_xy("Red Dwarf", CENTER, 30);
        print_xy("-------", CENTER, LASTY + 15);

        
        setFont(FONT_SMALL);
        draw_image(&ship_image, display_width / 2, (display_height / 2) - ship_height); // Draw ship image
        print_xy("Move the  Ship", CENTER, (display_height / 2) - ship_height - 20);
        draw_image(&asteroid_image, display_width / 2, (display_height / 2) - asteroid_height + 40); // Draw ship image
        print_xy("to avoid the Asteroid", CENTER, (display_height / 2) - asteroid_height +20);

        print_xy("<- : move ship left using", 10, LASTY + 40);
        print_xy("left button", 10, LASTY + 10);
        print_xy("-> : move ship right using", 10, LASTY + 15);
        print_xy("right button", 10, LASTY + 10);
        
        print_xy("Play Game", 0, display_height - 10);

        flip_frame();
        
        if (!gpio_get_level(0)) 
            break;
    }
    
    while (1)
    {
        setFont(FONT_SMALL);

        // Create Starting Variables
        float x = display_width / 2;
        float y = display_height - ship_height;
        float shipSpeed = 100;

        float asteroidSpeed = 100;
        float asteroidSpace = 1;
        float currentSpace = 0;
        float step = 1e-2f;

        int asteroidsNo = -1;
        int maxAsteroids = 20;
        struct asteroid asteroids[maxAsteroids];

        int score = 0;

        // Create Frame Timer
        uint64_t last_time = esp_timer_get_time();
        uint64_t start_time = esp_timer_get_time();

        bool runGame = true;
        while(runGame) 
        {
            uint64_t time = esp_timer_get_time();
            float dt = (time-last_time) * 1e-6f;
            last_time = time;

            cls(-1); // Clear screen
            draw_image(&space2_image, display_width / 2, ((int)((time-start_time) * 1e-6f * (asteroidSpeed / 2))) % 831); // Draw background image
            draw_image(&space2_image, display_width / 2, (((int)((time-start_time) * 1e-6f * (asteroidSpeed / 2))) % 831) - 831); // Draw background image
            draw_image(&ship_image, x, y); // Draw ship image

            char charVal[50];
            sprintf(charVal, "Score: %d", score);
            print_xy(charVal, CENTER, 0);

            drawAsteroids(asteroids, asteroidsNo);
            if (10 - (int)((((time-start_time) * 1e-6f) / 3) * 10) > 0)
            {
                sprintf(charVal, "Start in: %i", 10 - (int)((((time-start_time) * 1e-6f) / 3) * 10));
                print_xy(charVal, CENTER, LASTY + 10);
            }
            
            // Move ship left or right based on button press
            if (!gpio_get_level(0)) 
            {
                x -= shipSpeed * dt;
                if (x < ship_width / 2) 
                    x= ship_width / 2;
            }
            if (!gpio_get_level(35)) 
            {
                x += shipSpeed * dt;
                if (x > display_width - (ship_width / 2)) 
                    x = display_width - (ship_width / 2);
            }

            //Move asteroids down 
            if (asteroidsNo > 0 && moveAsteroids(asteroids, asteroidsNo, asteroidSpeed, dt))
            {
                // Remove asteroids that are below page
                removeAsteroids(asteroids, asteroidsNo);
                asteroidsNo --; 
                score += 100;
            }

            //Spawn more if necessary
            currentSpace += step;
            if (currentSpace >= asteroidSpace)
            {
                currentSpace = 0;
                if (asteroidsNo < maxAsteroids)
                    asteroidsNo ++;
                addAsteroid(asteroids, asteroidsNo);
            }

            runGame = !checkcollision(asteroids, asteroidsNo, x, y);

            flip_frame();
            asteroidSpeed += step;
            if (asteroidsNo < maxAsteroids / 2)
                asteroidSpace -= (step / 100);
        }

        int Position = 11;
        for (int i = 0; i < 10; i++)
        {
            if (topscores[i] < score)
            {
                Position = i;
                break;
            }
        }

        for (int i = 10 - 1; i >= Position; i--)
        {
            topscores[i] = topscores[i - 1];
        }
        topscores[Position] = score;

        bool endscreen = true;
        uint64_t end_time = esp_timer_get_time();
        while (endscreen)
        {
            uint64_t time = esp_timer_get_time();
            
            cls(-1); // Clear screen
            draw_image(&space_image, display_width / 2, display_height / 2); // Draw background image
            setFont(FONT_DEJAVU18);

            char charVal[50];
            sprintf(charVal, "Score: %d", score);
            print_xy(charVal, CENTER, 40);
            print_xy("-------", CENTER, LASTY + 15);

            setFont(FONT_UBUNTU16);
            for (int i = 0; i < 10; i++)
            {
                sprintf(charVal, "%i: %i", i + 1, topscores[i]);
                print_xy(charVal, 50, LASTY + 15);
            }

            setFont(FONT_SMALL);
            print_xy("Play Again", 0, display_height - 10);

            flip_frame();
            
            if (!(2 - (int)((((time-end_time) * 1e-6f) / 3) * 10) > 0) && !gpio_get_level(0)) 
                endscreen = false;
        }
    }
}