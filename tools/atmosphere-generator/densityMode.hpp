////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#ifndef DENSITY_MODE_HPP
#define DENSITY_MODE_HPP

#include <string>
#include <vector>

////////////////////////////////////////////////////////////////////////////////////////////////////
// This method is used to pre-compute the relative number density of atmospheric particles as a   //
// function of altitude. See README.md for usage details.                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////

int densityMode(std::vector<std::string> const& arguments);

#endif // DENSITY_MODE_HPP