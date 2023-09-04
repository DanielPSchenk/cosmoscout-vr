////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: 2017 Eric Bruneton
// SPDX-License-Identifier: BSD-3-Clause

// This file has been directly copied from here:
// https://github.com/ebruneton/precomputed_atmospheric_scattering/blob/master/atmosphere/definitions.glsl
// The only differences should be related to formatting. The documentation below can also be read
// online at:
// https://ebruneton.github.io/precomputed_atmospheric_scattering/atmosphere/definitions.glsl.html

/*<h2>atmosphere/definitions.glsl</h2>

<p>This GLSL file defines the physical types and constants which are used in the
main <a href="functions.glsl.html">functions</a> of our atmosphere model, in
such a way that they can be compiled by a GLSL compiler (a
<a href="reference/definitions.h.html">C++ equivalent</a> of this file
provides the same types and constants in C++, to allow the same functions to be
compiled by a C++ compiler - see the <a href="../index.html">Introduction</a>).

<h3>Physical quantities</h3>

<p>The physical quantities we need for our atmosphere model are
<a href="https://en.wikipedia.org/wiki/Radiometry">radiometric</a> and
<a href="https://en.wikipedia.org/wiki/Photometry_(optics)">photometric</a>
quantities. In GLSL we can't define custom numeric types to enforce the
homogeneity of expressions at compile time, so we define all the physical
quantities as <code>float</code>, with preprocessor macros (there is no
<code>typedef</code> in GLSL).

<p>We start with six base quantities: length, wavelength, angle, solid angle,
power and luminous power (wavelength is also a length, but we distinguish the
two for increased clarity).
*/

#define Length float
#define Wavelength float
#define Angle float
#define SolidAngle float
#define Power float
#define LuminousPower float

/*
<p>From this we "derive" the irradiance, radiance, spectral irradiance,
spectral radiance, luminance, etc, as well pure numbers, area, volume, etc (the
actual derivation is done in the <a href="reference/definitions.h.html">C++
equivalent</a> of this file).
*/

#define Number float
#define InverseLength float
#define Area float
#define Volume float
#define NumberDensity float
#define Irradiance float
#define Radiance float
#define SpectralPower float
#define SpectralIrradiance float
#define SpectralRadiance float
#define SpectralRadianceDensity float
#define ScatteringCoefficient float
#define InverseSolidAngle float
#define LuminousIntensity float
#define Luminance float
#define Illuminance float

/*
<p>We  also need vectors of physical quantities, mostly to represent functions
depending on the wavelength. In this case the vector elements correspond to
values of a function at some predefined wavelengths. Again, in GLSL we can't
define custom vector types to enforce the homogeneity of expressions at compile
time, so we define these vector types as <code>vec3</code>, with preprocessor
macros. The full definitions are given in the
<a href="reference/definitions.h.html">C++ equivalent</a> of this file).
*/

// A generic function from Wavelength to some other type.
#define AbstractSpectrum vec3
// A function from Wavelength to Number.
#define DimensionlessSpectrum vec3
// A function from Wavelength to SpectralPower.
#define PowerSpectrum vec3
// A function from Wavelength to SpectralIrradiance.
#define IrradianceSpectrum vec3
// A function from Wavelength to SpectralRadiance.
#define RadianceSpectrum vec3
// A function from Wavelength to SpectralRadianceDensity.
#define RadianceDensitySpectrum vec3
// A function from Wavelength to ScaterringCoefficient.
#define ScatteringSpectrum vec3

// A position in 3D (3 length values).
#define Position vec3
// A unit direction vector in 3D (3 unitless values).
#define Direction vec3
// A vector of 3 luminance values.
#define Luminance3 vec3
// A vector of 3 illuminance values.
#define Illuminance3 vec3

/*
<p>Finally, we also need precomputed textures containing physical quantities in
each texel. Since we can't define custom sampler types to enforce the
homogeneity of expressions at compile time in GLSL, we define these texture
types as <code>sampler2D</code> and <code>sampler3D</code>, with preprocessor
macros. The full definitions are given in the
<a href="reference/definitions.h.html">C++ equivalent</a> of this file).
*/

#define TransmittanceTexture sampler2D
#define AbstractScatteringTexture sampler3D
#define ReducedScatteringTexture sampler3D
#define ScatteringTexture sampler3D
#define ScatteringDensityTexture sampler3D
#define IrradianceTexture sampler2D

/*
<h3>Physical units</h3>

<p>We can then define the units for our six base physical quantities:
meter (m), nanometer (nm), radian (rad), steradian (sr), watt (watt) and lumen
(lm):
*/

const Length        m    = 1.0;
const Wavelength    nm   = 1.0;
const Angle         rad  = 1.0;
const SolidAngle    sr   = 1.0;
const Power         watt = 1.0;
const LuminousPower lm   = 1.0;

/*
<p>From which we can derive the units for some derived physical quantities,
as well as some derived units (kilometer km, kilocandela kcd, degree deg):
*/

const float PI = 3.14159265358979323846;

const Length                  km                                  = 1000.0 * m;
const Area                    m2                                  = m * m;
const Volume                  m3                                  = m * m * m;
const Angle                   pi                                  = PI * rad;
const Angle                   deg                                 = pi / 180.0;
const Irradiance              watt_per_square_meter               = watt / m2;
const Radiance                watt_per_square_meter_per_sr        = watt / (m2 * sr);
const SpectralIrradiance      watt_per_square_meter_per_nm        = watt / (m2 * nm);
const SpectralRadiance        watt_per_square_meter_per_sr_per_nm = watt / (m2 * sr * nm);
const SpectralRadianceDensity watt_per_cubic_meter_per_sr_per_nm  = watt / (m3 * sr * nm);
const LuminousIntensity       cd                                  = lm / sr;
const LuminousIntensity       kcd                                 = 1000.0 * cd;
const Luminance               cd_per_square_meter                 = cd / m2;
const Luminance               kcd_per_square_meter                = kcd / m2;

/*
<h3>Atmosphere parameters</h3>

<p>Using the above types, we can now define the parameters of our atmosphere
model. We start with the definition of density profiles, which are needed for
parameters that depend on the altitude:
*/

// An atmosphere layer of width 'width', and whose density is defined as
//   'exp_term' * exp('scale_height' * h) + 'linear_term' * h + 'constant_term',
// clamped to [0,1], and where h is the altitude.
struct DensityProfileLayer {
  Length        width;
  Number        exp_term;
  Length        scale_height;
  InverseLength linear_term;
  Number        constant_term;
};

// An atmosphere density profile made of several layers on top of each other
// (from bottom to top). The width of the last layer is ignored, i.e. it always
// extend to the top atmosphere boundary. The profile values vary between 0
// (null density) to 1 (maximum density).
struct DensityProfile {
  DensityProfileLayer layers[2];
};

// This texture contains all the phase functions for the scattering components of the atmosphere.
// The u coordinate maps to the scattering angle. Forward scattering is on the left (u == 0, theta
// == 0) and back scattering on the right (u == 1, theta == 180).
// The v coordinate maps to the various components. The phase function of the first scattering
// component is stored in the top row of pixels, the second in the next and so on.
uniform sampler2D phase_texture;

struct ScatteringComponent {
  Number             phaseTextureV;
  ScatteringSpectrum extinction;
  ScatteringSpectrum scattering;
  DensityProfile     density;
};

struct AbsorbingComponent {
  ScatteringSpectrum extinction;
  DensityProfile     density;
};

struct AtmosphereComponents {
  ScatteringComponent rayleigh;
  ScatteringComponent mie;
  AbsorbingComponent  ozone;
};
