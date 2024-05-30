////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "WCSCoverageImage.hpp"

#include "../../../../csl-ogc/src/wcs/WebCoverageService.hpp"
#include "../../../../csl-ogc/src/wcs/WebCoverageTextureLoader.hpp"
#include "../../../../src/cs-utils/filesystem.hpp"
#include "../../logger.hpp"
#include "../../types/CoverageContainer.hpp"
#include "../../types/types.hpp"

namespace csp::visualquery {

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string WCSCoverageImage::sName = "WCSCoverageImage";

////////////////////////////////////////////////////////////////////////////////////////////////////

std::string WCSCoverageImage::sSource() {
  return cs::utils::filesystem::loadToString(
      "../share/resources/nodes/csp-visual-query/WCSCoverageImage.js");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<WCSCoverageImage> WCSCoverageImage::sCreate() {
  return std::make_unique<WCSCoverageImage>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

WCSCoverageImage::WCSCoverageImage() {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

WCSCoverageImage::~WCSCoverageImage() {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::string const& WCSCoverageImage::getName() const {
  return sName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WCSCoverageImage::onMessageFromJS(nlohmann::json const& message) {

  logger().debug("WCSCoverageImage: Message form JS: {}", message.dump());

  // process();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

nlohmann::json WCSCoverageImage::getData() const {
  return nlohmann::json();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WCSCoverageImage::setData(nlohmann::json const& json) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void WCSCoverageImage::process() {
  auto coverage = readInput<std::shared_ptr<CoverageContainer>>("coverageIn", nullptr);
  if (coverage == nullptr) {
    return;
  }

  // create request for texture loading
  csl::ogc::WebCoverageTextureLoader::Request request = getRequest();

  // load texture
  auto texLoader  = csl::ogc::WebCoverageTextureLoader();
  auto textureOpt = texLoader.loadTexture(
      *coverage->mServer, *coverage->mImageChannel, request, "wcs-cache", true);

  if (textureOpt.has_value()) {
    auto texture     = textureOpt.value();
    auto textureSize = texture.x * texture.y;

    Image2D image;
    image.mNumScalars = 1;
    image.mDimension  = {texture.x, texture.y};

    // convert radians to degree
    image.mBounds = {texture.lnglatBounds[0] * (180 / M_PI), texture.lnglatBounds[2] * (180 / M_PI),
        texture.lnglatBounds[3] * (180 / M_PI), texture.lnglatBounds[1] * (180 / M_PI)};

    switch (texture.type) {
    case 1: // UInt8
    {
      std::vector<uint8_t> textureData(static_cast<uint8_t*>(texture.buffer),
          static_cast<uint8_t*>(texture.buffer) + textureSize);

      U8ValueVector pointData{};

      for (uint8_t scalar : textureData) {
        pointData.emplace_back(std::vector{scalar});
      }

      image.mPoints = pointData;
      break;
    }

    case 2: // UInt16
    {
      std::vector<uint16_t> textureData(static_cast<uint16_t*>(texture.buffer),
          static_cast<uint16_t*>(texture.buffer) + textureSize);

      U16ValueVector pointData{};

      for (uint16_t scalar : textureData) {
        pointData.emplace_back(std::vector{scalar});
      }

      image.mPoints = pointData;
      break;
    }

    case 3: // Int16
    {
      std::vector<int16_t> textureData(static_cast<int16_t*>(texture.buffer),
          static_cast<int16_t*>(texture.buffer) + textureSize);

      I16ValueVector pointData{};

      for (int16_t scalar : textureData) {
        pointData.emplace_back(std::vector{scalar});
      }

      image.mPoints = pointData;
      break;
    }

    case 4: // UInt32
    {
      std::vector<uint32_t> textureData(static_cast<uint32_t*>(texture.buffer),
          static_cast<uint32_t*>(texture.buffer) + textureSize);

      U32ValueVector pointData{};

      for (uint32_t scalar : textureData) {
        pointData.emplace_back(std::vector{scalar});
      }

      image.mPoints = pointData;
      break;
    }

    case 5: // Int32
    {
      std::vector<int32_t> textureData(static_cast<int32_t*>(texture.buffer),
          static_cast<int32_t*>(texture.buffer) + textureSize);

      I32ValueVector pointData{};

      for (int32_t scalar : textureData) {
        pointData.emplace_back(std::vector{scalar});
      }

      image.mPoints = pointData;
      break;
    }

    case 6: // Float32
    case 7: {
      std::vector<float> textureData(
          static_cast<float*>(texture.buffer), static_cast<float*>(texture.buffer) + textureSize);

      F32ValueVector pointData{};

      for (float scalar : textureData) {
        pointData.emplace_back(std::vector{scalar});
      }

      image.mPoints = pointData;
      break;
    }

    default:
      logger().error("Texture has no known data type.");
    }

    writeOutput("imageOut", std::make_shared<Image2D>(image));
  }
}

csl::ogc::WebCoverageTextureLoader::Request WCSCoverageImage::getRequest() {
  csl::ogc::WebCoverageTextureLoader::Request request;

  request.mTime = readInput<std::string>("wcsTimeIn", "");
  if (request.mTime.value() == "") {
    request.mTime.reset();
  }

  auto bounds = readInput<std::array<double, 4>>("boundsIn", {-180., 180., -90., 90.});

  csl::ogc::Bounds2D bound;
  request.mBounds.mMinLon = bounds[0];
  request.mBounds.mMaxLon = bounds[1];
  request.mBounds.mMinLat = bounds[2];
  request.mBounds.mMaxLat = bounds[3];

  request.mMaxSize    = readInput<int>("resolutionIn", 1024);
  request.mLayerRange = std::make_pair(readInput<int>("layerIn", 1), readInput<int>("layerIn", 1));

  request.mFormat = "image/tiff";

  return request;
}

} // namespace csp::visualquery