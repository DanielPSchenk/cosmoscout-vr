﻿////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "../../../../src/cs-utils/filesystem.hpp"
#include "TransferFunction.hpp"

namespace csp::visualquery {

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::string TransferFunction::sName = "TransferFunction";

////////////////////////////////////////////////////////////////////////////////////////////////////

std::string TransferFunction::sSource() {
  return cs::utils::filesystem::loadToString(
      "../share/resources/nodes/csp-visual-query/TransferFunction.js");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<TransferFunction> TransferFunction::sCreate() {
  return std::make_unique<TransferFunction>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::string const& TransferFunction::getName() const {
  return sName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TransferFunction::process() {
  writeOutput("lut", mLut);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TransferFunction::onMessageFromJS(nlohmann::json const& message) {
  if (message.find("lut") == message.end()) {
    return;
  }

  cs::core::Settings::deserialize(message, "lut", mLut);

  process();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

nlohmann::json TransferFunction::getData() const {
  return {}; // TODO
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TransferFunction::setData(nlohmann::json const& json) {
  // TODO
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::visualquery
