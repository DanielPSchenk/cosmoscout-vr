////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-FileCopyrightText: 2017 Eric Bruneton
// SPDX-License-Identifier: MIT

#include "Model.hpp"

#include "../../logger.hpp"

#include <glm/gtc/constants.hpp>

namespace csp::atmospheres::models::schneegans {

////////////////////////////////////////////////////////////////////////////////////////////////////

// The parameterization and comments below are based on the demo application by Eric Bruneton. The
// original source code can be found here:
// https://github.com/ebruneton/precomputed_atmospheric_scattering/blob/master/atmosphere/demo/demo.cc

enum Luminance {
  // Render the spectral radiance at kLambdaR, kLambdaG, kLambdaB.
  NONE,
  // Render the sRGB luminance, using an approximate (on the fly) conversion
  // from 3 spectral radiance values only (see section 14.3 in <a href=
  // "https://arxiv.org/pdf/1612.04336.pdf">A Qualitative and Quantitative
  //  Evaluation of 8 Clear Sky Models</a>).
  APPROXIMATE,
  // Render the sRGB luminance, precomputed from 15 spectral radiance values
  // (see section 4.4 in <a href=
  // "http://www.oskee.wz.cz/stranka/uploads/SCCG10ElekKmoch.pdf">Real-time
  //  Spectral Scattering in Large-scale Natural Participating Media</a>).
  PRECOMPUTED
};

constexpr bool      HALF_PRECISION    = false;
constexpr bool      COMBINED_TEXTURES = true;
constexpr Luminance LUMINANCE_MODE    = Luminance::PRECOMPUTED;

// Values from "Reference Solar Spectral Irradiance: ASTM G-173", ETR column
// (see http://rredc.nrel.gov/solar/spectra/am1.5/ASTMG173/ASTMG173.html),
// summed and averaged in each bin (e.g. the value for 360nm is the average
// of the ASTM G-173 values for all wavelengths between 360 and 370nm).
// Values in W.m^-2.
constexpr int                    LAMBDA_MIN       = 360;
constexpr int                    LAMBDA_MAX       = 830;
constexpr std::array<double, 48> SOLAR_IRRADIANCE = {1.11776, 1.14259, 1.01249, 1.14716, 1.72765,
    1.73054, 1.6887, 1.61253, 1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
    1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533, 1.6965, 1.68194,
    1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482, 1.41018, 1.36775, 1.34188, 1.31429,
    1.28303, 1.26758, 1.2367, 1.2082, 1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992};

// Values from http://www.iup.uni-bremen.de/gruppen/molspec/databases/
// referencespectra/o3spectra2011/index.html for 233K, summed and averaged in
// each bin (e.g. the value for 360nm is the average of the original values
// for all wavelengths between 360 and 370nm). Values in m^2.
constexpr std::array<double, 48> OZONE_CROSS_SECTION = {1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28,
    1.527e-27, 2.763e-27, 5.52e-27, 8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26,
    7.752e-26, 9.016e-26, 1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
    4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25, 2.662e-25,
    2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26, 6.566e-26, 5.105e-26,
    4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26, 2.534e-26, 1.624e-26, 1.465e-26,
    2.078e-26, 1.383e-26, 7.105e-27};

// From https://en.wikipedia.org/wiki/Dobson_unit, in molecules.m^-2.
constexpr double DOBSON_UNIT = 2.687e20;

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(nlohmann::json const& j, Model::Settings::Layer& o) {
  cs::core::Settings::deserialize(j, "width", o.mWidth);
  cs::core::Settings::deserialize(j, "expTerm", o.mExpTerm);
  cs::core::Settings::deserialize(j, "expScale", o.mExpScale);
  cs::core::Settings::deserialize(j, "linearTerm", o.mLinearTerm);
  cs::core::Settings::deserialize(j, "constantTerm", o.mConstantTerm);
}

void to_json(nlohmann::json& j, Model::Settings::Layer const& o) {
  cs::core::Settings::serialize(j, "width", o.mWidth);
  cs::core::Settings::serialize(j, "expTerm", o.mExpTerm);
  cs::core::Settings::serialize(j, "expScale", o.mExpScale);
  cs::core::Settings::serialize(j, "linearTerm", o.mLinearTerm);
  cs::core::Settings::serialize(j, "constantTerm", o.mConstantTerm);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(nlohmann::json const& j, Model::Settings::Component& o) {
  cs::core::Settings::deserialize(j, "extinctionSpectrum", o.mExtinctionSpectrum);
  cs::core::Settings::deserialize(j, "phaseFunctionSpectrum", o.mPhaseFunctionSpectrum);
  cs::core::Settings::deserialize(j, "layers", o.mLayers);
}

void to_json(nlohmann::json& j, Model::Settings::Component const& o) {
  cs::core::Settings::serialize(j, "extinctionSpectrum", o.mExtinctionSpectrum);
  cs::core::Settings::serialize(j, "phaseFunctionSpectrum", o.mPhaseFunctionSpectrum);
  cs::core::Settings::serialize(j, "layers", o.mLayers);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void from_json(nlohmann::json const& j, Model::Settings& o) {
  cs::core::Settings::deserialize(j, "sunAngularRadius", o.mSunAngularRadius);
  cs::core::Settings::deserialize(j, "components", o.mComponents);
  cs::core::Settings::deserialize(j, "groundAlbedo", o.mGroundAlbedo);
}

void to_json(nlohmann::json& j, Model::Settings const& o) {
  cs::core::Settings::serialize(j, "sunAngularRadius", o.mSunAngularRadius);
  cs::core::Settings::serialize(j, "components", o.mComponents);
  cs::core::Settings::serialize(j, "groundAlbedo", o.mGroundAlbedo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool Model::init(
    nlohmann::json const& modelSettings, double planetRadius, double atmosphereRadius) {

  Settings settings;

  try {
    settings = modelSettings;
  } catch (std::exception const& e) {
    logger().error("Failed to parse atmosphere parameters: {}", e.what());
  }

  auto                          layer = settings.mComponents[0].mLayers[0];
  internal::DensityProfileLayer rayleighLayer(
      layer.mWidth, layer.mExpTerm, layer.mExpScale, layer.mLinearTerm, layer.mConstantTerm);
  layer = settings.mComponents[1].mLayers[0];
  internal::DensityProfileLayer mieLayer(
      layer.mWidth, layer.mExpTerm, layer.mExpScale, layer.mLinearTerm, layer.mConstantTerm);

  // Density profile increasing linearly from 0 to 1 between 10 and 25km, and
  // decreasing linearly from 1 to 0 between 25 and 40km. This is an approximate
  // profile from http://www.kln.ac.lk/science/Chemistry/Teaching_Resources/
  // Documents/Introduction%20to%20atmospheric%20chemistry.pdf (page 10).
  std::vector<internal::DensityProfileLayer> ozoneDensity;
  layer = settings.mComponents[2].mLayers[0];
  ozoneDensity.emplace_back(
      layer.mWidth, layer.mExpTerm, layer.mExpScale, layer.mLinearTerm, layer.mConstantTerm);
  layer = settings.mComponents[2].mLayers[1];
  ozoneDensity.emplace_back(
      layer.mWidth, layer.mExpTerm, layer.mExpScale, layer.mLinearTerm, layer.mConstantTerm);

  std::vector<double> wavelengths;
  std::vector<double> solarIrradiance;
  std::vector<double> rayleighScattering;
  std::vector<double> mieScattering;
  std::vector<double> mieExtinction;
  std::vector<double> absorptionExtinction;
  std::vector<double> groundAlbedo;

  // Maximum number density of ozone molecules, in m^-3 (computed so at to get
  // 300 Dobson units of ozone - for this we divide 300 DU by the integral of
  // the ozone density profile defined below, which is equal to 15km).
  double maxOzoneNumberDensity      = 300.0 * DOBSON_UNIT / 15000.0;
  double mRayleigh                  = 1.24062e-6;
  double mMieScaleHeight            = 1200.0;
  double mMieAngstromAlpha          = 0.0;
  double mMieAngstromBeta           = 5.328e-3;
  double mMieSingleScatteringAlbedo = 0.9;
  double mMiePhaseFunctionG         = 0.8;

  for (int l = LAMBDA_MIN; l <= LAMBDA_MAX; l += 10) {
    double lambda = static_cast<double>(l) * 1e-3; // micro-meters
    double mie    = mMieAngstromBeta / mMieScaleHeight * pow(lambda, -mMieAngstromAlpha);
    wavelengths.push_back(l);
    solarIrradiance.push_back(SOLAR_IRRADIANCE[(l - LAMBDA_MIN) / 10]);
    rayleighScattering.push_back(mRayleigh * pow(lambda, -4));
    mieScattering.push_back(mie * mMieSingleScatteringAlbedo);
    mieExtinction.push_back(mie);
    absorptionExtinction.push_back(
        maxOzoneNumberDensity * OZONE_CROSS_SECTION[(l - LAMBDA_MIN) / 10]);
    groundAlbedo.push_back(settings.mGroundAlbedo.get());
  }

  double maxSunZenithAngle = (HALF_PRECISION ? 102.0 : 120.0) / 180.0 * glm::pi<double>();

  mModel.reset(new internal::Model(wavelengths, solarIrradiance, settings.mSunAngularRadius,
      planetRadius, atmosphereRadius, {rayleighLayer}, rayleighScattering, {mieLayer},
      mieScattering, mieExtinction, mMiePhaseFunctionG, ozoneDensity, absorptionExtinction,
      groundAlbedo, maxSunZenithAngle, 1.0, LUMINANCE_MODE == PRECOMPUTED ? 15 : 3,
      COMBINED_TEXTURES, HALF_PRECISION));

  glDisable(GL_CULL_FACE);
  mModel->Init();
  glEnable(GL_CULL_FACE);

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

GLuint Model::getShader() const {
  return mModel->shader();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

GLuint Model::setUniforms(GLuint program, GLuint startTextureUnit) const {
  mModel->SetProgramUniforms(program, startTextureUnit, startTextureUnit + 1, startTextureUnit + 2);
  return startTextureUnit + 3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::atmospheres::models::schneegans
