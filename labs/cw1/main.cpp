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
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
namespace fs = std::filesystem;

#define USE_SFML 0
#define USE_STB_IMAGE 0
#define USE_SINGLE_THREAD 1
#define SINGLE_THREAD_SPAWN_WORKERS 1
#define FAKE_DELAY 0
#define USE_OMP_HSV 0

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

const double getImageHSV_stb(const sf::Vector3f data)
{
    double h{ 0 };
    auto cmax = std::max(std::max(data.x, data.y), data.z);
    auto cmin = std::min(std::min(data.x, data.y), data.z);
    auto delta = cmax - cmin;

    if (delta != 0)
    {
        if (cmax == data.x)
        {
            h = ((data.y - data.z) / delta);
        }
        else if (cmax == data.y)
        {
            h = (((data.z - data.x) / delta) + 2);
        }
        else
        {
            h = (((data.x - data.y) / delta) + 4);
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
    return h;
}

const double RetrieveImageHSV(const uint8_t* imgdata, const int width, const int height, const int dim)
{
    std::vector<double> hueValues(width * height);
#if USE_OMP_HSV
    auto threadCount = std::thread::hardware_concurrency();
#pragma omp parallel for num_threads(threadCount - 3) schedule(dynamic)
#endif
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
            int o = x + y * width; // 1d index
            // is it RGB or RGBA?
            if (dim >= 3)
            {
                float r = imgdata[o * dim] / 255.0f;
                float g = imgdata[o * dim + 1] / 255.0f;
                float b = imgdata[o * dim + 2] / 255.0f;
                hueValues[o] = getImageHSV_stb(sf::Vector3f(r, g, b));
            }
            else if (dim == 1)
            {
                //values[o] = imgdata[o] / 255.0f;
                throw std::errc::no_such_process;
                hueValues[o] = -1;
            }
        }

    double median{ 0.0f };
    std::sort(hueValues.begin(), hueValues.end());
    if ((int)hueValues.size() % 2 == 0)
    {
        median = (hueValues[(int)hueValues.size() / 2] + hueValues[((int)hueValues.size() / 2) + 1]) / 2;
    }
    else
    {
        median = hueValues[((int)hueValues.size() + 1) / 2];
    }
    auto mid_drv{ median / 360.f };
    double integ{ 0.0 };
    double frac = modf(mid_drv, &integ);
    auto temperature = frac * (mid_drv + (1 / 6));
    return temperature;
}

const bool isHSVGreater(std::pair<sf::Texture, float> lhs, std::pair<sf::Texture, float> rhs)
{
    return lhs.second < rhs.second;
}

const bool isHSVGreater_ver2(std::pair<int, float> lhs, std::pair<int, float> rhs)
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
    auto threadCount = std::thread::hardware_concurrency();

    sf::Clock timer;
#if USE_SFML
    std::vector<std::string> imageFilenames(0);
    GetImageFilenames(image_folder, &imageFilenames);
    int fileCount{ (int)imageFilenames.size() }; // will be made irrelevant soon

    std::vector<std::pair<sf::Texture, double>> texs(fileCount);
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

    std::sort(texs.begin(), texs.end(), isHSVGreater);
#endif // USE_SFML

#if USE_STB_IMAGE
    std::vector<std::string> imageFilenames(0);
    GetImageFilenames(image_folder, &imageFilenames);
    int fileCount{ (int)imageFilenames.size() }; // will be made irrelevant soon

    std::vector<std::pair<int, float>> texs(fileCount);
#pragma omp parallel for num_threads(threadCount) schedule(dynamic)
    for (int i{ 0 }; i < fileCount; ++i)
    {
        int w{ 0 }, h{ 0 }, n{ 0 };
        auto imageData = stbi_load(imageFilenames[i].c_str(), &w, &h, &n, 0);
        texs[i].first = i;
        texs[i].second = RetrieveImageHSV(imageData, w, h, n);
        stbi_image_free(imageData);
    }
    std::sort(texs.begin(), texs.end(), isHSVGreater_ver2);
#endif // USE_STB_IMAGE

    auto time = timer.getElapsedTime().asMilliseconds();

    // Define some constants
    const float pi = 3.14159f;
    const int gameWidth = 800;
    const int gameHeight = 600;

    int imageIndex = 0;

#if USE_SINGLE_THREAD
    std::vector<std::string> imageFilenames(0);
    std::vector<std::pair<int, float>> texs(0);
#endif // USE_SINGLE_THREAD

    // Create the window of the application
    sf::RenderWindow window(sf::VideoMode(gameWidth, gameHeight, 32), "Image Fever",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setVerticalSyncEnabled(true);

    // Load an image to begin with
    sf::Texture texture;
    if (!texture.loadFromFile("G:/NapierWork/4th Year/Concurrent and Parallel Systems/image_fever_example/placeholder/placeholder.jpg"))
        return EXIT_FAILURE;
    sf::Sprite sprite (texture);
    // Make sure the texture fits the screen
    sprite.setScale(ScaleFromDimensions(texture.getSize(),gameWidth,gameHeight));
    std::unique_ptr<std::thread> loadThread{ nullptr };
    bool contentLoaded{ false };

    sf::Clock clock;

    while (window.isOpen())
    {
#if USE_SINGLE_THREAD   //  Setting to create load thread to handle overarching management and processing
        if (!loadThread && !contentLoaded)
        {
            clock.restart();
            loadThread = std::make_unique<std::thread>([&] {
#if FAKE_DELAY
                std::this_thread::sleep_for(std::chrono::seconds(20));
#endif // FAKE_DELAY
                GetImageFilenames(image_folder, &imageFilenames);
                int fileCount{ (int)imageFilenames.size() }; // will be made irrelevant soon
                texs.resize(fileCount);
#if SINGLE_THREAD_SPAWN_WORKERS //  Allows load thread to spawn other worker threads to manage the image processing workload
#pragma omp parallel for num_threads(threadCount - 1) schedule(dynamic)
#endif // SINGLE_THREAD_SPAWN_WORKERS
                for (int i{ 0 }; i < fileCount; ++i)
                {
                    int w{ 0 }, h{ 0 }, n{ 0 };
                    auto imageData = stbi_load(imageFilenames[i].c_str(), &w, &h, &n, 0);
                    texs[i].first = i;
                    texs[i].second = RetrieveImageHSV(imageData, w, h, n);
                    stbi_image_free(imageData);
                }
                std::sort(texs.begin(), texs.end(), isHSVGreater_ver2);
                contentLoaded = true;
            });
        }  
#endif // USE_SINGLE_THREAD

        if (sprite.getTexture() == &texture && contentLoaded)
        {
            const auto& imageFilename = imageFilenames[texs[0].first];
            // set it as the window title 
            window.setTitle(imageFilename);
            // ... and load the appropriate texture, and put it in the sprite
            if (texture.loadFromFile(imageFilename))
            {
                sprite = sf::Sprite(texture);
                sprite.setScale(ScaleFromDimensions(texture.getSize(), gameWidth, gameHeight));
            }
        }

        // Handle events
        sf::Event event;
        while (window.pollEvent(event))
        {
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
            if (event.type == sf::Event::KeyPressed && imageFilenames.size() != 0)
            {
                // adjust the image index
                if (event.key.code == sf::Keyboard::Key::Left)
                    imageIndex = (imageIndex + imageFilenames.size() - 1) % imageFilenames.size();
                else if (event.key.code == sf::Keyboard::Key::Right)
                    imageIndex = (imageIndex + 1) % imageFilenames.size();
                // get image filename
                const auto& imageFilename = imageFilenames[texs[imageIndex].first];
                // set it as the window title 
                window.setTitle(imageFilename);
                // ... and load the appropriate texture, and put it in the sprite
                if (texture.loadFromFile(imageFilename))
                {
                    sprite = sf::Sprite(texture);
                    sprite.setScale(ScaleFromDimensions(texture.getSize(), gameWidth, gameHeight));
                }
                std::cout << imageIndex << std::endl;
            }
        }

        if (loadThread && contentLoaded)
        {
            loadThread->join();
            loadThread.reset();
            auto time = clock.getElapsedTime().asMilliseconds();
            std::cout << time << "ms to load photos" << std::endl;
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
