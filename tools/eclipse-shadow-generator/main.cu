////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "../../src/cs-utils/CommandLine.hpp"

#include "LimbDarkening.cuh"
#include "gpuErrCheck.hpp"
#include "math.cuh"
#include "types.hpp"
#include "with_atmosphere.cuh"
#include "without_atmosphere.cuh"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// This tool can be used to create the eclipse shadow maps used by CosmoScout VR. See the         //
// README.md file in this directory for usage instructions!                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

  stbi_flip_vertically_on_write(1);

  ShadowSettings settings;

  std::string cOutput = "shadow.hdr";
  std::string cMode   = "limb-darkening";
  std::string cAtmosphereSettings;
  bool        cPrintHelp = false;

  // First configure all possible command line options.
  cs::utils::CommandLine args(
      "Welcome to the shadow map generator! Here are the available options:");
  args.addArgument({"-o", "--output"}, &cOutput,
      "The image will be written to this file (default: \"" + cOutput + "\").");
  args.addArgument({"--size"}, &settings.size,
      "The output texture size (default: " + std::to_string(settings.size) + ").");
  args.addArgument({"--mode"}, &cMode,
      "This should be either 'with-atmosphere', 'limb-darkening', 'circles', 'linear', or "
      "'smoothstep' (default: " +
          cMode + ").");
  args.addArgument({"--with-umbra"}, &settings.includeUmbra,
      "Add the umbra region to the shadow map (default: " + std::to_string(settings.includeUmbra) +
          ").");
  args.addArgument({"--mapping-exponent"}, &settings.mappingExponent,
      "Adjusts the distribution of sampling positions. A value of 1.0 will position the "
      "umbra's end in the middle of the texture, larger values will shift this to the "
      "right. (default: " +
          std::to_string(settings.mappingExponent) + ").");
  args.addArgument({"--atmosphere-settings"}, &cAtmosphereSettings,
      "The path to the atmosphere settings directory. Must be given when using the "
      "'with-atmosphere' mode.");
  args.addArgument({"-h", "--help"}, &cPrintHelp, "Show this help message.");

  // Then do the actual parsing.
  try {
    std::vector<std::string> arguments(argv + 1, argv + argc);
    args.parse(arguments);
  } catch (std::runtime_error const& e) {
    std::cerr << "Failed to parse command line arguments: " << e.what() << std::endl;
    return 1;
  }

  // When cPrintHelp was set to true, we print a help message and exit.
  if (cPrintHelp) {
    args.printHelp();
    return 0;
  }

  // Check whether a valid mode was given.
  if (cMode != "limb-darkening" && cMode != "circles" && cMode != "linear" &&
      cMode != "smoothstep" && cMode != "with-atmosphere") {
    std::cerr << "Invalid value given for --mode!" << std::endl;
    return 1;
  }

  // If we are in atmosphere mode, we need also the atmosphere settings.
  if (cMode == "with-atmosphere" && cAtmosphereSettings.empty()) {
    std::cerr << "When using the 'with-atmosphere' mode, you must provide the path to the "
                 "atmosphere settings directory using --atmosphere-settings!"
              << std::endl;
    return 1;
  }

  // Initialize the limb darkening model.
  LimbDarkening limbDarkening;
  limbDarkening.init();

  // Compute the 2D kernel size.
  dim3     blockSize(16, 16);
  uint32_t numBlocksX = (settings.size + blockSize.x - 1) / blockSize.x;
  uint32_t numBlocksY = (settings.size + blockSize.y - 1) / blockSize.y;
  dim3     gridSize   = dim3(numBlocksX, numBlocksY);

  // Allocate the shared memory for the shadow map.
  float* shadow = nullptr;
  gpuErrchk(cudaMallocManaged(
      &shadow, static_cast<size_t>(settings.size * settings.size) * 3 * sizeof(float)));

  // Compute the shadow map based on the given mode.
  if (cMode == "with-atmosphere") {
    computeAtmosphereShadow(shadow, settings, cAtmosphereSettings, limbDarkening);
  } else if (cMode == "limb-darkening") {
    computeLimbDarkeningShadow<<<gridSize, blockSize>>>(shadow, settings, limbDarkening);
  } else if (cMode == "circles") {
    computeCircleIntersectionShadow<<<gridSize, blockSize>>>(shadow, settings);
  } else if (cMode == "linear") {
    computeLinearShadow<<<gridSize, blockSize>>>(shadow, settings);
  } else if (cMode == "smoothstep") {
    computeSmoothstepShadow<<<gridSize, blockSize>>>(shadow, settings);
  }

  gpuErrchk(cudaPeekAtLastError());
  gpuErrchk(cudaDeviceSynchronize());

  // Finally write the output texture!
  stbi_write_hdr(
      cOutput.c_str(), static_cast<int>(settings.size), static_cast<int>(settings.size), 3, shadow);

  return 0;
}
