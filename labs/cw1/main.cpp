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
#define USE_PROCESSOR_THREAD 1
#define SINGLE_THREAD_SPAWN_WORKERS 1
#define FAKE_DELAY 0
#define USE_OMP_HSV 0
#define USE_TEXTURE_THREAD 1
#define THREAD_OVRHD (USE_PROCESSOR_THREAD + USE_TEXTURE_THREAD)
#define USE_STRUCT 1

struct ProcessedImage {
    int ID{ 0 };
    float HSV_Value{ 0.0f };
    std::string fileLocation;
    sf::Texture Texture;
    bool txrReady{ false };
};

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
        double median{ 0.0 };
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
    return 0;
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
#pragma omp parallel for num_threads(threadCount - THREAD_OVRHD) schedule(dynamic)
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

const bool isHSVGreater_ver3(ProcessedImage lhs, ProcessedImage rhs)
{
    return lhs.HSV_Value < rhs.HSV_Value;
}

void GetImageFilenames(const char* folder, std::vector<std::string> *output)
{
    for (auto& p : fs::directory_iterator(folder))
    {
        output->resize(output->size() + 1);
        output->back() = p.path().u8string();
    }
}

void GetImageFilenamesForStruct(const char* folder, std::vector<ProcessedImage>* output)
{
    int IDs{ 0 };
    for (auto& p : fs::directory_iterator(folder))
    {
        output->resize(output->size() + 1);
        output->back().fileLocation = p.path().u8string();
        output->back().ID = IDs;
        ++IDs;
    }
}

void LoadImagesToVector(const std::vector<std::string> fileNames, std::vector<sf::Texture> *output)
{
    for (auto n : fileNames)
    {
        output->resize(output->size() + 1);
        sf::Texture tempTex;
        tempTex.loadFromFile(n);
        output->back() = tempTex;
    }
}

void LoadImagesToStructuredVector(std::vector<ProcessedImage>* output)
{
    for (auto& n : *output)
    {
        n.Texture.loadFromFile(n.fileLocation);
        n.txrReady = true;
    }
}

int main()
{
    std::srand(static_cast<unsigned int>(std::time(NULL)));

    sf::Clock toMainTimer;

    const char* image_folder{ "./unsorted" };

    auto threadCount{ std::thread::hardware_concurrency() };

#if USE_SFML
    sf::Clock timer;
    std::vector<std::string> imageFilenames(0);
    GetImageFilenames(image_folder, &imageFilenames);
    int fileCount{ (int)imageFilenames.size() }; // will be made irrelevant soon

    std::vector<std::pair<sf::Texture, double>> texs(fileCount);
#pragma omp parallel for num_threads(threadCount - THREAD_OVRHD) schedule(static) // #TODO: test performance here
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
    auto time{ timer.getElapsedTime().asMilliseconds() };
#endif // USE_SFML

#if USE_STB_IMAGE
    sf::Clock timer;
    std::vector<std::string> imageFilenames(0);
    GetImageFilenames(image_folder, &imageFilenames);
    int fileCount{ (int)imageFilenames.size() }; // will be made irrelevant soon

    std::vector<std::pair<int, float>> texs(fileCount);
#pragma omp parallel for num_threads(threadCount - THREAD_OVRHD) schedule(dynamic)
    for (int i{ 0 }; i < fileCount; ++i)
    {
        int w{ 0 }, h{ 0 }, n{ 0 };
        auto imageData = stbi_load(imageFilenames[i].c_str(), &w, &h, &n, 0);
        texs[i].first = i;
        texs[i].second = RetrieveImageHSV(imageData, w, h, n);
        stbi_image_free(imageData);
    }
    std::sort(texs.begin(), texs.end(), isHSVGreater_ver2);
    auto time{ timer.getElapsedTime().asMilliseconds() };
#endif // USE_STB_IMAGE


    // Define some constants
    const float pi{ 3.14159f };
    const int gameWidth{ 800 };
    const int gameHeight{ 600 };

    int imageIndex{ 0 };

#if USE_PROCESSOR_THREAD
    std::vector<std::string> imageFilenames(0);
#if USE_STRUCT
    std::vector<ProcessedImage> Images(0);
#elif !USE_STB_IMAGE && !USE_SFML
    std::vector<std::pair<int, float>> imgData(0);
    std::vector<sf::Texture> imgSprites(0);
#endif // USE_STRUCT
#endif // USE_PROCESSOR_THREAD

    // Create the window of the application
    sf::RenderWindow window(sf::VideoMode(gameWidth, gameHeight, 32), "Image Fever",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setVerticalSyncEnabled(true);

    // Load an image to begin with
#if !USE_TEXTURE_THREAD
    sf::Texture texture;
#endif
    sf::Texture placeholderTexture;
    if (!placeholderTexture.loadFromFile("./placeholder/placeholder.jpg"))
        return EXIT_FAILURE;
    sf::Sprite placeholderSprite(placeholderTexture);
    sf::Sprite sprite(placeholderSprite);
    // Make sure the texture fits the screen
    sprite.setScale(ScaleFromDimensions(placeholderTexture.getSize(),gameWidth,gameHeight));

    //  Planned external threads that will manage 
    std::unique_ptr<std::thread> hsvThread{ nullptr };
    std::unique_ptr<std::thread> textureThread{ nullptr };

    // Main thread indicators for external thread usage
    bool dataRetrieved{ false };
    bool imagesSprited{ false };
    bool rmvdPlaceholder{ false };

    auto timeToMain{ toMainTimer.getElapsedTime().asMilliseconds() };
    std::cout << timeToMain << "ms to main loop" << std::endl;
    sf::Clock clock1, clock2;

    while (window.isOpen())
    {
#if USE_PROCESSOR_THREAD   //  Setting to create load thread to handle overarching management and processing
        if (!hsvThread && !dataRetrieved)
        {
            clock1.restart();
            hsvThread = std::make_unique<std::thread>([&] {
#if FAKE_DELAY
                std::this_thread::sleep_for(std::chrono::seconds(20));
#endif // FAKE_DELAY
#if USE_STRUCT
                GetImageFilenamesForStruct(image_folder, &Images);
#else
                GetImageFilenames(image_folder, &imageFilenames);
#endif // USE_STRUCT
#if USE_TEXTURE_THREAD
#if USE_STRUCT
                textureThread = std::make_unique<std::thread>([&] { clock2.restart();  LoadImagesToStructuredVector(&Images); imagesSprited = true; });
#else
                textureThread = std::make_unique<std::thread>([&] { clock2.restart();  LoadImagesToVector(imageFilenames, &imgSprites); imagesSprited = true; });
                imgData.resize((int)imageFilenames.size());
#endif // USE_STRUCT
#else
                imgData.resize((int)imageFilenames.size());
#endif // USE_TEXTURE_THREAD
#if SINGLE_THREAD_SPAWN_WORKERS //  Allows load thread to spawn other worker threads to manage the image processing workload
#pragma omp parallel for num_threads(threadCount - THREAD_OVRHD) schedule(dynamic)
#endif // SINGLE_THREAD_SPAWN_WORKERS
#if USE_STRUCT
                for (int i{ 0 }; i < (int)Images.size(); i++)
                {
                    int w{ 0 }, h{ 0 }, n{ 0 };
                    auto rawData = stbi_load(Images[i].fileLocation.c_str(), &w, &h, &n, 0);
                    Images[i].HSV_Value = RetrieveImageHSV(rawData, w, h, n);
                    stbi_image_free(rawData);
                }
                std::sort(Images.begin(), Images.end(), isHSVGreater_ver3);
                dataRetrieved = true;
#else
                for (int i{ 0 }; i < (int)imgData.size(); ++i)
                {
                    int w{ 0 }, h{ 0 }, n{ 0 };
                    auto rawData = stbi_load(imageFilenames[i].c_str(), &w, &h, &n, 0);
                    imgData[i].first = i;
                    imgData[i].second = RetrieveImageHSV(rawData, w, h, n);
                    stbi_image_free(rawData);
                }
                std::sort(imgData.begin(), imgData.end(), isHSVGreater_ver2);
                dataRetrieved = true;
#endif // USE_STRUCT

            });
        }  
#endif // USE_PROCESSOR_THREAD

#if !USE_TEXTURE_THREAD && !USE_STB_IMAGE && !USE_SFML
        if (sprite.getTexture() == &placeholderTexture && dataRetrieved && !rmvdPlaceholder)
        {
            const auto& imageFilename = imageFilenames[imgData[0].first];
            // set it as the window title 
            window.setTitle(imageFilename);
            // ... and load the appropriate texture, and put it in the sprite
            if (placeholderTexture.loadFromFile(imageFilename))
            {
                sprite = sf::Sprite(placeholderTexture);
                sprite.setScale(ScaleFromDimensions(placeholderTexture.getSize(), gameWidth, gameHeight));
            }
            rmvdPlaceholder = true;
        }
#else
#if USE_STRUCT
        if (Images.size() != 0)
        {
            if (sprite.getTexture() == placeholderSprite.getTexture() && Images[imageIndex].txrReady)
            {
                sprite = sf::Sprite(Images[imageIndex].Texture);
            }
        }
#else
#if !USE_STB_IMAGE && !USE_SFML
        if (sprite.getTexture() == &placeholderTexture && dataRetrieved && imagesSprited && !rmvdPlaceholder)
        {
            // set it as the window title 
            window.setTitle(imageFilenames[imgData[0].first]);
            // ... and load the appropriate texture, and put it in the sprite
            sprite = sf::Sprite(imgSprites[imgData[0].first]);
            sprite.setScale(ScaleFromDimensions(placeholderTexture.getSize(), gameWidth, gameHeight));
            rmvdPlaceholder = true;
        }
#endif // !USE_STB_IMAGE && !USE_SFML
#endif // USE_STRUCT
#endif // !USE_TEXTURE_THREAD


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
#if !USE_TEXTURE_THREAD
            if (event.type == sf::Event::KeyPressed && imageFilenames.size() != 0)
            {
                // adjust the image index
                if (event.key.code == sf::Keyboard::Key::Left)
                    imageIndex = (imageIndex + imageFilenames.size() - 1) % imageFilenames.size();
                else if (event.key.code == sf::Keyboard::Key::Right)
                    imageIndex = (imageIndex + 1) % imageFilenames.size();
                // get image filename
#if USE_STB_IMAGE
                const auto& imageFilename = imageFilenames[texs[imageIndex].first];
#elif !USE_STB_IMAGE && !USE_SFML
                const auto& imageFilename = imageFilenames[imgData[imageIndex].first];
#endif
#if !USE_SFML
                // set it as the window title 
                window.setTitle(imageFilename);
                // ... and load the appropriate texture, and put it in the sprite
                if (texture.loadFromFile(imageFilename))
                {
                    sprite = sf::Sprite(texture);
                    sprite.setScale(ScaleFromDimensions(texture.getSize(), gameWidth, gameHeight));
                }
                std::cout << "Image index = " << imageIndex << std::endl;
#else
                // set it as the window title 
                window.setTitle("image");
                // ... and load the appropriate texture, and put it in the sprite
                sprite = sf::Sprite(texs[imageIndex].first);
                sprite.setScale(ScaleFromDimensions(texs[imageIndex].first.getSize(), gameWidth, gameHeight));
                std::cout << "Image index = " << imageIndex << std::endl;
#endif // !USE_SFML

            }
#else
#if USE_STRUCT
            if (event.type == sf::Event::KeyPressed && dataRetrieved && imagesSprited)
            {
                // adjust the image index
                if (event.key.code == sf::Keyboard::Key::Left)
                    imageIndex = (imageIndex + Images.size() - 1) % Images.size();
                else if (event.key.code == sf::Keyboard::Key::Right)
                    imageIndex = (imageIndex + 1) % Images.size();

                window.setTitle(Images[imageIndex].fileLocation);
                if (Images[imageIndex].txrReady)
                {
                    sprite = sf::Sprite(Images[imageIndex].Texture);
                }
                else
                {
                    sprite = placeholderSprite;
                }
                sprite.setScale(ScaleFromDimensions(placeholderTexture.getSize(), gameWidth, gameHeight));
            }
#else
            if (event.type == sf::Event::KeyPressed && dataRetrieved && imagesSprited)
            {
                // adjust the image index
                if (event.key.code == sf::Keyboard::Key::Left)
                    imageIndex = (imageIndex + imageFilenames.size() - 1) % imageFilenames.size();
                else if (event.key.code == sf::Keyboard::Key::Right)
                    imageIndex = (imageIndex + 1) % imageFilenames.size();
                
                window.setTitle(imageFilenames[imgData[imageIndex].first]);
                sprite = sf::Sprite(imgSprites[imgData[imageIndex].first]);
                sprite.setScale(ScaleFromDimensions(imgSprites[imgData[imageIndex].first].getSize(), gameWidth, gameHeight));
            }
#endif
#endif // !USE_TEXTURE_THREAD
        }

        if (hsvThread && dataRetrieved)
        {
            hsvThread->join();
            hsvThread.reset();
            auto time = clock1.getElapsedTime().asMilliseconds();
            std::cout << time << "ms to process photos" << std::endl;
        }

#if USE_TEXTURE_THREAD
        if (textureThread && imagesSprited)
        {
            textureThread->join();
            textureThread.reset();
            auto time = clock2.getElapsedTime().asMilliseconds();
            std::cout << time << "ms to make sprites" << std::endl;
        }
#endif // USE_TEXTURE_THREAD


        // Clear the window
        window.clear(sf::Color(0, 0, 0));
        // draw the sprite
        window.draw(sprite);
        // Display things on screen
        window.display();
    }

    return EXIT_SUCCESS;
}
