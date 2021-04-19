#include <cstdlib>
#include <immintrin.h>

#include <SFML/Graphics.hpp>

static const char *WindowTitle = "mandelbrot";

static const std::size_t WindowWidth = 1024;
static const std::size_t WindowHeight = 1024;

static const unsigned FPSCounterCharacterSize = 20;
static const unsigned FPSCounterX = FPSCounterCharacterSize;
static const unsigned FPSCounterY = FPSCounterCharacterSize / 2;

struct {
    float left;
    float top;
    float right;
    float bottom;
} static const RegionOfInterest{.left = -2, .top = 1, .right = 1, .bottom = -1};

static const float XC0 = 0;
static const float YC0 = 0;

static const float OXMovementFactor = 10;
static const float OYMovementFactor = 10;

static const float DeltaX = (RegionOfInterest.right - RegionOfInterest.left) / WindowWidth;
static const float DeltaY = (RegionOfInterest.top - RegionOfInterest.bottom) / WindowHeight;
static const float DeltaScale = 0.25;

static const float SqrRMax = 2 * 2;
static const std::size_t MaxIterNo = 255;

struct RGBQuad {
    sf::Uint8 r;
    sf::Uint8 g;
    sf::Uint8 b;
    sf::Uint8 a;
};

typedef RGBQuad (&TextureBuf)[WindowHeight][WindowWidth];

void renderTexture(TextureBuf textureBuf, float xC, float yC, float scale);
void setPixel(RGBQuad *pixel, unsigned char iterNo);

signed main()
{
    static_assert(MaxIterNo < 256);

    sf::RenderWindow Window{sf::VideoMode{WindowWidth, WindowHeight}, WindowTitle};

    sf::Texture texture;
    if (!texture.create(WindowWidth, WindowHeight)) return EXIT_FAILURE;
    sf::Sprite sprite(texture);

    sf::Font textFont;
    if (!textFont.loadFromFile("arial.ttf")) return EXIT_FAILURE;

    sf::Text fpsCounter{"", textFont, FPSCounterCharacterSize};
    sf::FloatRect rect = fpsCounter.getLocalBounds();
    fpsCounter.setOrigin(rect.left + rect.width / 2, rect.top + rect.height / 2);
    fpsCounter.setPosition(FPSCounterX, FPSCounterY);
    fpsCounter.setFillColor(sf::Color::Green);
    fpsCounter.setOutlineColor(sf::Color::Green);

    sf::Clock clock;

    TextureBuf textureBuf = (TextureBuf) *(sf::Uint8 *) calloc(WindowWidth * WindowHeight * 4, sizeof(sf::Uint8));

    float xC = XC0;
    float yC = YC0;
    float scale = 1;

    sf::Event event{};
    while (Window.isOpen()) {
        clock.restart();

        while (Window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed:
                    Window.close();
                    break;
                case sf::Event::KeyPressed:
                    switch (event.key.code) {
                        case sf::Keyboard::Left:
                            xC -= DeltaX * OXMovementFactor;
                            break;
                        case sf::Keyboard::Right:
                            xC += DeltaX * OXMovementFactor;
                            break;
                        case sf::Keyboard::Up:
                            yC += DeltaY * OYMovementFactor;
                            break;
                        case sf::Keyboard::Down:
                            yC -= DeltaY * OYMovementFactor;
                            break;
                        case sf::Keyboard::Z:
                            scale -= DeltaScale;
                            break;
                        case sf::Keyboard::X:
                            scale += DeltaScale;
                            break;
                        default:
                            break;
                    }
                default:
                    break;
            }
        }

        renderTexture(textureBuf, xC, yC, scale);

        texture.update((sf::Uint8 *) textureBuf);

        Window.clear();
        Window.draw(sprite);
        Window.draw(fpsCounter);
        Window.display();

        sf::Time frameRenderingTime = clock.getElapsedTime();
        clock.restart();
        fpsCounter.setString(std::to_string(1 / frameRenderingTime.asSeconds()));
    }

    free(textureBuf);

    return EXIT_SUCCESS;
}

void renderTexture(TextureBuf textureBuf, float xC, float yC, float scale = 1)
{
    const __m256 idxSeq = _mm256_setr_ps(0, 1, 2, 3, 4, 5, 6, 7);
    const __m256 sqrRMax = _mm256_set1_ps(SqrRMax);
    const __m256i bitPosSeq = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
    const __m256i bitMask = _mm256_set1_epi32(1);
    const __m256 twoFactor = _mm256_set1_ps(2);

    for (std::size_t iY = 0; iY < WindowHeight; ++iY) {
        __m256 deltaXSeq = _mm256_mul_ps(idxSeq, _mm256_set1_ps(DeltaX));
        __m256 x0 = _mm256_add_ps(_mm256_set1_ps((RegionOfInterest.left + xC) * scale), deltaXSeq);
        __m256 y0 = _mm256_set1_ps((RegionOfInterest.top - iY * DeltaY + yC) * scale);

        for (std::size_t iX = 0; iX < WindowWidth; iX += 8) {
            __m256 x = x0;
            __m256 y = y0;

            __m256i iterNos = _mm256_setzero_si256();

            std::size_t iterNo = 0;
            for (; iterNo < MaxIterNo; ++iterNo) {
                __m256 sqrX = _mm256_mul_ps(x, x);
                __m256 sqrY = _mm256_mul_ps(y, y);
                __m256 crossProd = _mm256_mul_ps(x, y);

                __m256 sqrR = _mm256_add_ps(sqrX, sqrY);

                auto cmp = (unsigned char) _mm256_movemask_ps(_mm256_cmp_ps(sqrR, sqrRMax, _CMP_LT_OS));
                if (cmp == 0) break;

                iterNos = _mm256_add_epi32(iterNos, _mm256_and_si256(_mm256_srlv_epi32(_mm256_set1_epi32(cmp), bitPosSeq), bitMask));

                x = _mm256_add_ps(_mm256_sub_ps(sqrX, sqrY), x0);
                y = _mm256_fmadd_ps(twoFactor, crossProd, y0);
            }

            for (size_t i = 0; i < 8; ++i) {
                setPixel(&textureBuf[iY][iX + i], ((int *) &iterNos)[i]);
            }

            x0 = _mm256_add_ps(x0, _mm256_set1_ps(DeltaX * 8 * scale));
        }
    }
}

void setPixel(RGBQuad *pixel, unsigned char iterNo)
{
    pixel->r = ((iterNo + 1) % 2) * 255;
    pixel->g = ((iterNo + 1) % 2) * 255;
    pixel->b = ((iterNo + 1) % 2) * 255;
    pixel->a = sqrt(sqrt((float) (iterNo + 1) / (MaxIterNo + 1))) * 255;
}
