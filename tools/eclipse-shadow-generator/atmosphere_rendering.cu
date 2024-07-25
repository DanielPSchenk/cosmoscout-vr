////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "atmosphere_rendering.cuh"

#include "tiff_utils.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

////////////////////////////////////////////////////////////////////////////////////////////////////

double __device__ getSunLuminance(double sunRadius) {
  const double sunLuminousPower = 3.75e28;
  const double sunLuminousExitance =
      sunLuminousPower / (sunRadius * sunRadius * 4.0 * glm::pi<double>());
  return sunLuminousExitance / glm::pi<double>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

cudaTextureObject_t createCudaTexture(tiff_utils::RGBATexture const& texture) {
  cudaArray* cuArray;
  auto       channelDesc = cudaCreateChannelDesc<float4>();

  cudaMallocArray(&cuArray, &channelDesc, texture.width, texture.height);
  cudaMemcpy2DToArray(cuArray, 0, 0, texture.data.data(), texture.width * sizeof(float) * 4,
      texture.width * sizeof(float) * 4, texture.height, cudaMemcpyHostToDevice);

  // Specify texture object parameters
  cudaResourceDesc resDesc = {};
  resDesc.resType          = cudaResourceTypeArray;
  resDesc.res.array.array  = cuArray;

  cudaTextureDesc texDesc  = {};
  texDesc.addressMode[0]   = cudaAddressModeClamp;
  texDesc.addressMode[1]   = cudaAddressModeClamp;
  texDesc.filterMode       = cudaFilterModeLinear;
  texDesc.readMode         = cudaReadModeElementType;
  texDesc.normalizedCoords = 1;

  // Create texture object
  cudaTextureObject_t textureObject = 0;
  cudaCreateTextureObject(&textureObject, &resDesc, &texDesc, nullptr);

  gpuErrchk(cudaGetLastError());

  return textureObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ glm::vec4 texture2D(cudaTextureObject_t tex, glm::vec2 uv) {
  auto data = tex2D<float4>(tex, uv.x, uv.y);
  return glm::vec4(data.x, data.y, data.z, data.w);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ float safeSqrt(float a) {
  return glm::sqrt(glm::max(a, 0.0f));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ float clampDistance(float d) {
  return glm::max(d, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ float getTextureCoordFromUnitRange(float x, int textureSize) {
  return 0.5 / float(textureSize) + x * (1.0 - 1.0 / float(textureSize));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ float distanceToTopAtmosphereBoundary(
    common::Geometry const& geometry, float r, float mu) {
  float discriminant = r * r * (mu * mu - 1.0) + geometry.mRadiusAtmo * geometry.mRadiusAtmo;
  return clampDistance(-r * mu + safeSqrt(discriminant));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// As we are always in outer space, this function does not need the r parameter.
__device__ glm::vec2 getTransmittanceTextureUvFromRMu(
    advanced::Textures const& textures, common::Geometry const& geometry, double mu) {
  // Distance to top atmosphere boundary for a horizontal ray at ground level.
  double H =
      sqrt(geometry.mRadiusAtmo * geometry.mRadiusAtmo - geometry.mRadiusOcc * geometry.mRadiusOcc);
  // Distance to the top atmosphere boundary for the ray (r,mu), and its minimum
  // and maximum values over all mu - obtained for (r,1) and (r,mu_horizon).
  double d    = distanceToTopAtmosphereBoundary(geometry, geometry.mRadiusAtmo, mu);
  double dMax = 2.0 * H;
  double xMu  = d / dMax;
  return glm::vec2(getTextureCoordFromUnitRange(xMu, textures.mTransmittanceTextureWidth),
      getTextureCoordFromUnitRange(1.0, textures.mTransmittanceTextureHeight));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// As we are always in outer space, this function does not need the r parameter.
__device__ glm::vec3 getScatteringTextureUvwFromRMuMuSNu(advanced::Textures const& textures,
    common::Geometry const& geometry, double mu, double muS, double nu,
    bool rayRMuIntersectsGround) {

  // Distance to top atmosphere boundary for a horizontal ray at ground level.
  double H =
      sqrt(geometry.mRadiusAtmo * geometry.mRadiusAtmo - geometry.mRadiusOcc * geometry.mRadiusOcc);

  // Discriminant of the quadratic equation for the intersections of the ray (r,mu) with the ground
  // (see rayIntersectsGround).
  double rMu          = geometry.mRadiusAtmo * mu;
  double discriminant = rMu * rMu - geometry.mRadiusAtmo * geometry.mRadiusAtmo +
                        geometry.mRadiusOcc * geometry.mRadiusOcc;
  double uMu;
  if (rayRMuIntersectsGround) {
    // Distance to the ground for the ray (r,mu), and its minimum and maximum values over all mu -
    // obtained for (r,-1) and (r,mu_horizon).
    double d    = -rMu - safeSqrt(discriminant);
    double dMin = geometry.mRadiusAtmo - geometry.mRadiusOcc;
    double dMax = H;
    uMu = 0.5 - 0.5 * getTextureCoordFromUnitRange(dMax == dMin ? 0.0 : (d - dMin) / (dMax - dMin),
                          textures.mScatteringTextureMuSize / 2);
  } else {
    // Distance to the top atmosphere boundary for the ray (r,mu), and its minimum and maximum
    // values over all mu - obtained for (r,1) and (r,mu_horizon).
    double d    = -rMu + safeSqrt(discriminant + H * H);
    double dMax = 2.0 * H;
    uMu = 0.5 + 0.5 * getTextureCoordFromUnitRange(d / dMax, textures.mScatteringTextureMuSize / 2);
  }

  double d    = distanceToTopAtmosphereBoundary(geometry, geometry.mRadiusOcc, muS);
  double dMin = geometry.mRadiusAtmo - geometry.mRadiusOcc;
  double dMax = H;
  double a    = (d - dMin) / (dMax - dMin);
  double D    = distanceToTopAtmosphereBoundary(geometry, geometry.mRadiusOcc, textures.mMuSMin);
  double A    = (D - dMin) / (dMax - dMin);
  // An ad-hoc function equal to 0 for muS = MU_S_MIN (because then d = D and thus a = A), equal to
  // 1 for muS = 1 (because then d = dMin and thus a = 0), and with a large slope around muS = 0,
  // to get more texture samples near the horizon.
  float uMuS = getTextureCoordFromUnitRange(
      glm::max(1.0 - a / A, 0.0) / (1.0 + a), textures.mScatteringTextureMuSSize);

  float uNu = (nu + 1.0) / 2.0;
  return glm::vec3(uNu, uMuS, uMu);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ void getRefractedViewRays(advanced::Textures const& textures,
    common::Geometry const& geometry, glm::dvec3 camera, glm::dvec3 viewRay, glm::dvec3& viewRayR,
    glm::dvec3& viewRayG, glm::dvec3& viewRayB) {

  double    mu = dot(camera, viewRay) / geometry.mRadiusAtmo;
  glm::vec2 uv = getTransmittanceTextureUvFromRMu(textures, geometry, mu);

  // Cosine of the angular deviation of the ray due to refraction.
  glm::dvec3 muRGB = glm::cos(glm::dvec3(texture2D(textures.mThetaDeviation, uv)));
  glm::dvec3 axis  = glm::normalize(glm::cross(camera, viewRay));

  viewRayR = normalize(math::rotateVector(viewRay, axis, muRGB.r));
  viewRayG = normalize(math::rotateVector(viewRay, axis, muRGB.g));
  viewRayB = normalize(math::rotateVector(viewRay, axis, muRGB.b));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ glm::vec3 getTransmittanceToTopAtmosphereBoundary(
    advanced::Textures const& textures, common::Geometry const& geometry, double mu) {
  glm::vec2 uv = getTransmittanceTextureUvFromRMu(textures, geometry, mu);
  return glm::vec3(texture2D(textures.mTransmittance, uv));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ bool rayIntersectsGround(common::Geometry const& geometry, double mu) {
  return mu < 0.0 && geometry.mRadiusAtmo * geometry.mRadiusAtmo * (mu * mu - 1.0) +
                             geometry.mRadiusOcc * geometry.mRadiusOcc >=
                         0.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ glm::vec3 moleculePhaseFunction(cudaTextureObject_t phaseTexture, float nu) {
  float theta = glm::acos(nu) / M_PI; // 0<->1
  return glm::vec3(texture2D(phaseTexture, glm::vec2(theta, 0.0)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ glm::vec3 aerosolPhaseFunction(cudaTextureObject_t phaseTexture, float nu) {
  float theta = glm::acos(nu) / M_PI; // 0<->1
  return glm::vec3(texture2D(phaseTexture, glm::vec2(theta, 1.0)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ void getCombinedScattering(advanced::Textures const& textures,
    common::Geometry const& geometry, float mu, float muS, float nu, bool rayRMuIntersectsGround,
    glm::vec3& multipleScattering, glm::vec3& singleAerosolsScattering) {
  glm::vec3 uvw =
      getScatteringTextureUvwFromRMuMuSNu(textures, geometry, mu, muS, nu, rayRMuIntersectsGround);
  float     texCoordX = uvw.x * float(textures.mScatteringTextureNuSize - 1);
  float     texX      = floor(texCoordX);
  float     lerp      = texCoordX - texX;
  glm::vec2 uv0       = glm::vec2((texX + uvw.y) / float(textures.mScatteringTextureNuSize), uvw.z);
  glm::vec2 uv1 = glm::vec2((texX + 1.0 + uvw.y) / float(textures.mScatteringTextureNuSize), uvw.z);

  multipleScattering = glm::vec3(texture2D(textures.mMultipleScattering, uv0) * (1.0f - lerp) +
                                 texture2D(textures.mMultipleScattering, uv1) * lerp);
  singleAerosolsScattering =
      glm::vec3(texture2D(textures.mSingleAerosolsScattering, uv0) * (1.0f - lerp) +
                texture2D(textures.mSingleAerosolsScattering, uv1) * lerp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace advanced {

////////////////////////////////////////////////////////////////////////////////////////////////////

__host__ Textures loadTextures(std::string const& path) {
  tiff_utils::RGBATexture multiscattering =
      tiff_utils::read2DTexture(path + "/multiple_scattering.tif", 31);
  tiff_utils::RGBATexture singleScattering =
      tiff_utils::read2DTexture(path + "/single_aerosols_scattering.tif", 31);
  tiff_utils::RGBATexture theta_deviation =
      tiff_utils::read2DTexture(path + "/theta_deviation.tif");
  tiff_utils::RGBATexture phase         = tiff_utils::read2DTexture(path + "/phase.tif");
  tiff_utils::RGBATexture transmittance = tiff_utils::read2DTexture(path + "/transmittance.tif");

  std::cout << "Computing shadow map with atmosphere..." << std::endl;
  std::cout << "  - Mutli-scattering texture dimensions: " << multiscattering.width << "x"
            << multiscattering.height << std::endl;
  std::cout << "  - Single-scattering texture dimensions: " << singleScattering.width << "x"
            << singleScattering.height << std::endl;
  std::cout << "  - Theta deviation texture dimensions: " << theta_deviation.width << "x"
            << theta_deviation.height << std::endl;
  std::cout << "  - Phase texture dimensions: " << phase.width << "x" << phase.height << std::endl;
  std::cout << "  - Transmittance texture dimensions: " << transmittance.width << "x"
            << transmittance.height << std::endl;

  Textures textures;
  textures.mMultipleScattering       = createCudaTexture(multiscattering);
  textures.mSingleAerosolsScattering = createCudaTexture(singleScattering);
  textures.mThetaDeviation           = createCudaTexture(theta_deviation);
  textures.mPhase                    = createCudaTexture(phase);
  textures.mTransmittance            = createCudaTexture(transmittance);

  textures.mTransmittanceTextureWidth  = 256;
  textures.mTransmittanceTextureHeight = 64;
  textures.mScatteringTextureMuSize    = 128;
  textures.mScatteringTextureMuSSize   = 256 / 8;
  textures.mScatteringTextureNuSize    = 8;
  textures.mMuSMin                     = std::cos(2.094395160675049);

  return textures;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

__device__ glm::vec3 getLuminance(glm::dvec3 camera, glm::dvec3 viewRay, glm::dvec3 sunDirection,
    common::Geometry const& geometry, common::LimbDarkening const& limbDarkening,
    Textures const& textures, double phiSun) {

  // Compute the distance to the top atmosphere boundary along the view ray, assuming the viewer is
  // in space (or NaN if the view ray does not intersect the atmosphere).
  double r   = length(camera);
  double rmu = dot(camera, viewRay);
  double distanceToTopAtmosphereBoundary =
      -rmu - sqrt(rmu * rmu - r * r + geometry.mRadiusAtmo * geometry.mRadiusAtmo);

  glm::vec3  skyLuminance  = glm::vec3(0.0);
  glm::vec3  transmittance = glm::vec3(1.0);
  glm::dvec3 viewRayR, viewRayG, viewRayB;
  viewRayR = viewRayG = viewRayB = viewRay;

  // We only need to compute the luminance if the view ray intersects the atmosphere.
  if (distanceToTopAtmosphereBoundary > 0.0) {

    camera += viewRay * distanceToTopAtmosphereBoundary;

    // Compute the mu, muS and nu parameters needed for the texture lookups.
    double mu                     = (rmu + distanceToTopAtmosphereBoundary) / geometry.mRadiusAtmo;
    double muS                    = dot(camera, sunDirection) / geometry.mRadiusAtmo;
    double nu                     = dot(viewRay, sunDirection);
    bool   rayRMuIntersectsGround = rayIntersectsGround(geometry, mu);

    glm::vec3 multipleScattering;
    glm::vec3 singleAerosolsScattering;
    getCombinedScattering(textures, geometry, mu, muS, nu, rayRMuIntersectsGround,
        multipleScattering, singleAerosolsScattering);

    skyLuminance = multipleScattering * moleculePhaseFunction(textures.mPhase, nu) +
                   singleAerosolsScattering * aerosolPhaseFunction(textures.mPhase, nu);

    getRefractedViewRays(textures, geometry, camera, viewRay, viewRayR, viewRayG, viewRayB);

    transmittance = rayRMuIntersectsGround
                        ? glm::vec3(0.0)
                        : getTransmittanceToTopAtmosphereBoundary(textures, geometry, mu);
  }

  float sunR = limbDarkening.get(math::angleBetweenVectors(viewRayR, sunDirection) / phiSun);
  float sunG = limbDarkening.get(math::angleBetweenVectors(viewRayG, sunDirection) / phiSun);
  float sunB = limbDarkening.get(math::angleBetweenVectors(viewRayB, sunDirection) / phiSun);

  glm::vec3 sunLuminance =
      transmittance * (float)getSunLuminance(geometry.mRadiusSun) * glm::vec3(sunR, sunG, sunB);

  return skyLuminance + sunLuminance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace advanced

////////////////////////////////////////////////////////////////////////////////////////////////////
