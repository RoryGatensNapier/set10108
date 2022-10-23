// This is a chopped Pong example from SFML examples

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <omp.h>
#include <thread>
#include <iostream>

namespace fs = std::filesystem;

sf::Vector2f ScaleFromDimensions(const sf::Vector2u& textureSize, int screenWidth, int screenHeight)
{
    float scaleX = screenWidth / float(textureSize.x);
    float scaleY = screenHeight / float(textureSize.y);
    float scale = std::min(scaleX, scaleY);
    return { scale, scale };
}

const double getImageHSV(const sf::Texture tex)
{
    double h{ 0.0 }, s{ 0.0 }, v{ 0.0 };
    int acc{ 0 }, px_count{ 0 };
    auto size = tex.getSize();
    px_count = size.x * size.y;
    std::vector<double> hueValues(px_count);
    auto img = tex.copyToImage();

    if (img.getPixelsPtr())
    {
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                auto col = img.getPixel(x, y);
                auto r_derived = col.r / 255.0;
                auto g_derived = col.g / 255.0;
                auto b_derived = col.b / 255.0;
                auto cmax = std::max(std::max(r_derived, g_derived), b_derived);
                auto cmin = std::min(std::min(r_derived, g_derived), b_derived);
                auto delta = cmax - cmin;

                if (delta != 0)
                {
                    if (cmax == r_derived)
                    {
                        h = ((g_derived - b_derived) / delta);
                    }
                    else if (cmax == g_derived)
                    {
                        h = (((b_derived - r_derived) / delta) + 2);
                    }
                    else
                    {
                        h = (((r_derived - g_derived) / delta) + 4);
                    }
                    h *= 60;
                }
                else
                {
                    h = 0;
                }
                if (h < 0)
                {
                    //h += 360;
                }
                hueValues[acc] = h;
                ++acc;
            }
        }
        double median{ 0.0f };
        std::sort(hueValues.begin(), hueValues.end());
        if (px_count % 2 == 0)
        {
            median = (hueValues[px_count / 2] + hueValues[(px_count / 2) + 1]) / 2;
        }
        else
        {
            median = hueValues[(px_count + 1) / 2];
        }
        auto mid_drv{ median / 360.f };
        double integ{ 0.0 };
        double frac = modf(mid_drv, &integ);
        auto hueMapped = frac * (mid_drv + (1 / 6));
        img.~Image();
        return hueMapped;
    }
}

const bool isHSVGreater(std::pair<sf::Texture, float> lhs, std::pair<sf::Texture, float> rhs)
{
    return lhs.second < rhs.second;
}

void GetImageFilenames(const char* folder, std::vector<std::string> *output)
{
    for (auto& p : fs::directory_iterator(folder))
    {
        output->resize(output->size() + 1);
        output->back() = p.path().u8string();
    }
}

int main()
{
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    constexpr char* image_folder = "G:/NapierWork/4th Year/Concurrent and Parallel Systems/image_fever_example/unsorted";
    std::vector<std::string> imageFilenames(0);
    //std::vector<std::pair<sf::Texture, double>> texs(0); //-- implement this with same resize method as used in the filenames function
    // 
    //#TODO: push this into a threaded structure w/ main load thread that spawns other workers once all the file names have been gathered
    //While texs != imageFilenames.size() then do all this stuff, then once complete join and allow place holder to be removed
    GetImageFilenames(image_folder, &imageFilenames);

    int fileCount{ (int)imageFilenames.size() }; // will be made irrelevant soon
    std::vector<std::pair<sf::Texture, double>> texs(fileCount);
    auto threadCount = std::thread::hardware_concurrency();
    sf::Clock timer;
    timer.restart();
#pragma omp parallel for num_threads(threadCount) schedule(static) // #TODO: test performance here
    for (int i = 0; i < fileCount; i++)
    {
        if (texs[i].first.loadFromFile(imageFilenames[i]))
        {
            texs[i].second = getImageHSV(texs[i].first);
        }
        else
        {
            texs[i].first = sf::Texture();
            texs[i].second = -1;
        }
    }
    
    auto time = timer.getElapsedTime().asMilliseconds();
    std::sort(texs.begin(), texs.end(), isHSVGreater);

    // Define some constants
    const float pi = 3.14159f;
    const int gameWidth = 800;
    const int gameHeight = 600;

    int imageIndex = 0;

    // Create the window of the application
    sf::RenderWindow window(sf::VideoMode(gameWidth, gameHeight, 32), "Image Fever",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setVerticalSyncEnabled(true);

    // Load an image to begin with
    sf::Texture texture;
    if (!texture.loadFromFile("G:/NapierWork/4th Year/Concurrent and Parallel Systems/image_fever_example/placeholder/placeholder.jpg"))
        return EXIT_FAILURE;
    sf::Sprite sprite (texs[0].first);
    // Make sure the texture fits the screen
    sprite.setScale(ScaleFromDimensions(texture.getSize(),gameWidth,gameHeight));

    sf::Clock clock;
    while (window.isOpen())
    {
        // Handle events
        sf::Event event;
        while (window.pollEvent(event))
        {
            // This currently works as is with just GetImageFilenames
            //std::thread loadThread([&] {
            //    GetImageFilenames(image_folder, &imageFilenames);
            //    // ...
            //    });

            // Window closed or escape key pressed: exit
            if ((event.type == sf::Event::Closed) ||
               ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape)))
            {
                window.close();
                break;
            }
            
            // Window size changed, adjust view appropriately
            if (event.type == sf::Event::Resized)
            {
                sf::View view;
                view.setSize(gameWidth, gameHeight);
                view.setCenter(gameWidth/2.f, gameHeight/2.f);
                window.setView(view);
            }

            // Arrow key handling!
            if (event.type == sf::Event::KeyPressed)
            {
                // adjust the image index
                if (event.key.code == sf::Keyboard::Key::Left)
                    imageIndex = (imageIndex + imageFilenames.size() - 1) % imageFilenames.size();
                else if (event.key.code == sf::Keyboard::Key::Right)
                    imageIndex = (imageIndex + 1) % imageFilenames.size();
                // get image filename
                const auto& imageFilename = imageFilenames[imageIndex];
                // set it as the window title 
                window.setTitle(imageFilename);
                // ... and load the appropriate texture, and put it in the sprite
                if (texture.loadFromFile(imageFilename))
                {
                    sprite = sf::Sprite(texs[imageIndex].first);
                    sprite.setScale(ScaleFromDimensions(texture.getSize(), gameWidth, gameHeight));
                }
                std::cout << imageIndex << std::endl;
            }
        }

        // Clear the window
        window.clear(sf::Color(0, 0, 0));
        // draw the sprite
        window.draw(sprite);
        // Display things on screen
        window.display();
    }

    return EXIT_SUCCESS;
}
