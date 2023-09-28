////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "logger.hpp"

#include "../cs-utils/logger.hpp"

namespace cs::audio {

////////////////////////////////////////////////////////////////////////////////////////////////////

spdlog::logger& logger() {
  static auto logger = utils::createLogger("cs-audio");
  return *logger;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace cs::audio
