#define DR_FLAC_IMPLEMENTATION

#include <complex>
#include <iostream>
#include <valarray>
#include <string>
#include <unistd.h>
#include <math.h>
#include <vector>
#include <fstream>

#include "lib/dr_flac.h"

#define SAMPLE_RATE 44100

#define NUM_CHUNKS 1400
#define NUM_LEVELS 800

typedef std::complex<double> complex_val;
typedef std::valarray<complex_val> complex_arr;

void transform(complex_arr &input)
{
    double PI = 3.14159265358979323846;

    const size_t size = input.size();

    if (size <= 1)
    {
        return;
    }

    complex_arr even = input[std::slice(0, size / 2, 2)];
    transform(even);

    complex_arr odd = input[std::slice(1, size / 2, 2)];
    transform(odd);

    for (size_t i = 0; i < size / 2; ++i)
    {
        complex_val t = std::polar(1.0, -2 * PI * i / size) * odd[i];
        input[i] = even[i] + t;
        input[i + size / 2] = even[i] - t;
    }
}

int main(int argc, char *argv[])
{
    drflac *pFlac = drflac_open_file(argv[1], NULL);

    if (pFlac == NULL)
    {
        std::cout << "read err: filename not found" << std::endl;
        return -1;
    }

    // final image is stored here
    int32_t spectrogram[NUM_CHUNKS][NUM_LEVELS];

    int32_t *pSampleData = (int32_t *)malloc((size_t)pFlac->totalPCMFrameCount * pFlac->channels * sizeof(int32_t));
    drflac_read_pcm_frames_s32(pFlac, pFlac->totalPCMFrameCount, pSampleData);

    int32_t CHUNK_SIZE = pFlac->totalPCMFrameCount / NUM_CHUNKS;

    int32_t max_sample = 0;
    int32_t min_sample = 0;

    double max_sample_double = 0.0;
    double min_sample_double = 0.0;

    for (int i = 0; i < NUM_CHUNKS * 2; i++)
    {

        complex_val samples_complex[CHUNK_SIZE];

        for (int j = 0; j < CHUNK_SIZE; j++)
        {
            int32_t val = pSampleData[(i * CHUNK_SIZE) + j];

            double val_double = val / (double)2147483647;

            samples_complex[j] = complex_val(val_double);

            {
                if (val > max_sample)
                {
                    max_sample = val;
                }

                if (val < min_sample)
                {
                    min_sample = val;
                }

                if (val_double > max_sample_double)
                {
                    max_sample_double = val_double;
                }

                if (val_double < min_sample_double)
                {
                    min_sample_double = val_double;
                }
            }
        }

        complex_arr data(samples_complex, CHUNK_SIZE);

        transform(data);

        std::cout << "fft iteration: " << i << "/" << NUM_CHUNKS * 2 << std::endl;

        // plug into spectrogram
        for (int m = 0; m < NUM_LEVELS; m++)
        {
            int32_t level_size = ((CHUNK_SIZE / NUM_LEVELS) / 4); // 4 is the magic number here, removes the second half ( / 2) and negative frequencies ( / 2) -> ( / 4)

            // level_size can not be < 1, will result in full black image
            if (level_size < 1)
            {
                level_size = 1;
            }

            // TODO: special coefficient for scaling number of samples per level on dramatically different length songs (can't use level_size)

            double magnitude = 0;
            for (int l = 0; l < level_size; l++)
            {
                magnitude += (abs(data[(m * (level_size)) + l].real()));
                magnitude += (abs(data[(m * (level_size)) - l].real()));
            }
            spectrogram[i / 2][m] = round(magnitude);
        }
    }

    double frequency_resolution = SAMPLE_RATE / double(CHUNK_SIZE * 2); // this is how much Hz each step is on spectrogram

    std::cout << "freq res: " << frequency_resolution << std::endl;
    std::cout << "chunk size: " << CHUNK_SIZE << std::endl;

    std::cout << "min sample: " << min_sample << ", max sample: " << max_sample << std::endl;
    std::cout << "min sample double: " << min_sample_double << ", max sample double: " << max_sample_double << std::endl;

    std::cout << "num blocks: " << pFlac->totalPCMFrameCount / CHUNK_SIZE << std::endl;
    std::cout << "num levels: " << NUM_LEVELS << std::endl;
    std::cout << "num samples per chunk: " << CHUNK_SIZE << std::endl;

    drflac_close(pFlac);

    std::cout << "file closed" << std::endl;

    // make image
    std::ofstream output;
    output.open("output.ppm");
    output << "P3\n";
    output << NUM_CHUNKS << " " << NUM_LEVELS << "\n";
    output << "255\n";

    for (int i = 0; i < NUM_LEVELS; i++)
    {
        for (int j = 0; j < NUM_CHUNKS; j++)
        {
            int32_t color = spectrogram[j][NUM_LEVELS - i];
            int32_t r = 0;
            int32_t g = 0;
            int32_t b = 0;
            {
                if (color > 0)
                {
                    r = 180;
                    g = 62;
                    b = 58;
                }
                if (color > 1)
                {
                    r = 233;
                    g = 62;
                    b = 58;
                }
                if (color > 2)
                {
                    r = 237;
                    g = 104;
                    b = 60;
                }
                if (color > 5)
                {
                    r = 243;
                    g = 144;
                    b = 63;
                }
                if (color > 10)
                {
                    r = 253;
                    g = 199;
                    b = 12;
                }
                if (color > 20)
                {
                    r = 255;
                    g = 243;
                    b = 59;
                }
                if (color > 50)
                {
                    r = 255;
                    g = 251;
                    b = 195;
                }
                if (color > 100)
                {
                    r = 255;
                    g = 255;
                    b = 255;
                }
            }
            output << r << " " << g << " " << b << " ";
        }
        output << "\n";
    }

    output.close();

    return 0;
}