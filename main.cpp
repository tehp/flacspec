#define DR_FLAC_IMPLEMENTATION

#include <complex>
#include <iostream>
#include <valarray>
#include <unistd.h>
#include <vector>

#include "/usr/local/include/FLAC++/decoder.h"
#include "/usr/local/include/FLAC++/encoder.h"
#include "/usr/local/include/FLAC++/export.h"
#include "/usr/local/include/FLAC++/metadata.h"

#include "lib/dr_flac.h"

#define SAMPLE_RATE 44100

#define NUM_CHUNKS 1200
#define NUM_LEVELS 600

const double PI = 3.141592653589793238460;

typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;

// Cooley–Tukey FFT (in-place, divide-and-conquer)
// Higher memory requirements and redundancy although more intuitive
void fft(CArray &x)
{
    const size_t N = x.size();

    if (N <= 1)
    {
        return;
    }

    // divide
    CArray even = x[std::slice(0, N / 2, 2)];
    CArray odd = x[std::slice(1, N / 2, 2)];

    // conquer
    fft(even);
    fft(odd);

    // combine
    for (size_t k = 0; k < N / 2; ++k)
    {
        Complex t = std::polar(1.0, -2 * PI * k / N) * odd[k];
        x[k] = even[k] + t;
        x[k + N / 2] = even[k] - t;
    }
}

int main()
{
    // libflac++
    FLAC__StreamDecoder *fileDecoder;

    // dr_flac
    drflac *pFlac = drflac_open_file("music/zeldas_theme.flac", NULL);
    if (pFlac == NULL)
    {
        std::cout << "read err: filename not found" << std::endl;
        return -1;
    }

    int32_t spectrogram[NUM_CHUNKS][NUM_LEVELS];

    for (int i = 0; i < NUM_CHUNKS; i++)
    {
        for (int j = 0; j < NUM_LEVELS; j++)
        {
            spectrogram[i][j] = 0;
        }
    }

    int32_t *pSampleData = (int32_t *)malloc((size_t)pFlac->totalPCMFrameCount * pFlac->channels * sizeof(int32_t));
    drflac_read_pcm_frames_s32(pFlac, pFlac->totalPCMFrameCount, pSampleData);

    int32_t CHUNK_SIZE = pFlac->totalPCMFrameCount / NUM_CHUNKS;

    std::cout << "num blocks: " << pFlac->totalPCMFrameCount / CHUNK_SIZE << std::endl;
    std::cout << "num levels: " << NUM_LEVELS << std::endl;
    std::cout << "num samples per chunk: " << CHUNK_SIZE << std::endl;

    std::vector<int32_t> samples;

    int32_t max_sample = 0;
    int32_t min_sample = 0;

    double max_sample_double = 0.0;
    double min_sample_double = 0.0;

    double max_found = 0;

    for (int i = 0; i < NUM_CHUNKS; i++)
    {
        std::vector<int32_t> samples_chunk;
        std::vector<double> samples_chunk_double;

        Complex samples_complex[CHUNK_SIZE];

        for (int j = 0; j < CHUNK_SIZE; j++)
        {
            int32_t val = pSampleData[(i * CHUNK_SIZE) + j];
            samples.push_back(val);
            samples_chunk.push_back(val);
            samples_chunk_double.push_back(val / (double)2147483647);

            double val_double = val / (double)2147483647;

            samples_complex[j] = Complex(val_double);

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

        CArray data(samples_complex, CHUNK_SIZE);

        fft(data);

        std::cout << "fft iteration: " << i << "/" << NUM_CHUNKS << std::endl;

        for (int i = 0; i < CHUNK_SIZE / 2; ++i)
        {
            double a = abs(data[i].real());
            if (a > max_found)
            {
                max_found = a;
            }
        }

        // plug into spectrogram
        for (int m = 0; m < NUM_LEVELS; m++)
        {
            spectrogram[i][m] = abs(data[m * (CHUNK_SIZE / NUM_LEVELS)].real()); // TODO: aggregate levels instead of skipping to fit screen
        }
    }

    double frequency_resolution = SAMPLE_RATE / double(CHUNK_SIZE * 2); // this is how much Hz each step is on spectrogram

    std::cout << "freq res: " << frequency_resolution << std::endl;
    std::cout << "chunk size: " << CHUNK_SIZE << std::endl;

    std::cout << "num samples: " << samples.size() << std::endl;

    std::cout << "min sample: " << min_sample << ", max sample: " << max_sample << std::endl;
    std::cout << "min sample double: " << min_sample_double << ", max sample double: " << max_sample_double << std::endl;

    drflac_close(pFlac);

    std::cout << "file closed" << std::endl;

    return 0;
}