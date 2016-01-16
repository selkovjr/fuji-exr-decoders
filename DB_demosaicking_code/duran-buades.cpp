/*
 * Copyright 2009-2015 IPOL Image Processing On Line http://www.ipol.im/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @mainpage A Demosaicking Algorithm with Adaptive Inter-channel Correlation
 *           (version 1)
 *
 * README.txt:
 * @verbinclude README.txt
 */

/**
 * @file   duran-buades.cpp
 * @brief  Main executable file for the Duran-Buades (2015) decoder
 *
 * @author Gene Selkov <selkovjr@gmail.com>
 */

#include "libdemosaic.h"
#include "libAuxiliary.h"
#include "io_tiff.h"
#include <string.h>

// Usage: duran-buades bayer.tiff decoded.tiff beta

int main(int argc, char **argv) {
  if (argc < 4) {
    printf("usage: duran-buades bayer.tiff decoded.tiff beta\n\n");
    printf("bayer.tiff   :: input Bayer-encoded image (gray scale).\n");
    printf("decoded.tiff :: demosaicked image.\n");
    printf("beta         :: fixed channel-correlation parameter.\n");
    printf("\n");
    printf("The following parameters are fixed in main():\n");
    printf("epsilon   :: thresholding parameter avoiding numerical\n"
        "             intrincacies when computing local variation of\n"
        "             chromatic components.\n");
    printf("M         :: bounding parameter above which a discontinuity\n"
        "             of the luminance gradient is considered.\n");
    printf("halfL     :: half-size of the support zone where the variance\n"
        "             of the chromatic components is computed.\n");
    printf("reswind   :: half-size of research window.\n");
    printf("compwind  :: half-size of comparison window.\n");
    printf("N         :: number of most similar pixels for filtering.\n");
    printf("redx redy :: coordinates of the first red value in CFA.\n");

    return EXIT_FAILURE;
  }

  // Read Bayer-encoded image
  size_t nx, ny;
  float *mosaicked = NULL;
  char *description;

  /* TIFF 16-bit grayscale -> float input */
  if (NULL == (mosaicked = read_tiff_gray16_f32(argv[1], &nx, &ny, &description))) {
    fprintf(stderr, "error while reading from %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  if (!mosaicked) {
    fprintf(stderr, "Error - %s not found or not a correct TIFF image.\n", argv[1]);
    return EXIT_FAILURE;
  }

  // Input image parameters
  int width = (int) nx;
  int height = (int) ny;
  int dim = width * height;

  // Input parameters
  float beta = atof(argv[3]);

  if ((beta < 0.0f) || (beta > 1.0f)) {
    fprintf(stderr, "Error - beta must be in range (0,1].\n");
    return EXIT_FAILURE;
  }

  // Compute h in terms of beta if not automatically determined
  float h = 0.0f;

  if (beta != 0.0f) {
    h = (310.0f * beta - 214.0f) / 3.0f;
  }

  // Fixed parameters
  float epsilon = 0.00000001f;
  float M = 13.0f;
  int halfL = 1;
  int reswind = 10;
  int compwind = 1;
  int N = 10;
  int redx = 1;
  int redy = 0;

  int num_channels = 3;

  // Demosaicking process
  float **demosaicked = new float*[num_channels];
  for (int c = 0; c < num_channels; c++)
    demosaicked[c] = new float[dim];

  if (algorithm_chain(mosaicked, mosaicked, mosaicked, demosaicked[0],
        demosaicked[1], demosaicked[2], beta, h, epsilon, M,
        halfL, reswind, compwind, N, redx, redy, width,
        height) != 1)
    return EXIT_FAILURE;

  // Save demosaicked image
  float *output_image = new float[dim * 3];
  int k = 0;
  for (int c = 0; c < num_channels; c++)
    for (int i = 0; i < dim; i++) {
      output_image[k] = demosaicked[c][i];
      k++;
    }

  if (write_tiff_rgb_f32(argv[2], output_image, (size_t) width, (size_t) height)) {
    fprintf(stderr, "Error - Failed to save TIFF image %s.\n", argv[2]);
    return EXIT_FAILURE;
  }

  // Delete allocated memory
  delete[] output_image;
  free(mosaicked);

  for (int c = 0; c < num_channels; c++) {
    delete[] demosaicked[c];
  }

  delete[] demosaicked;

  return EXIT_SUCCESS;
}
