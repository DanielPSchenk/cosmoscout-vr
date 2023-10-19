////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "OpenAlManager.hpp"
#include "../../cs-core/Settings.hpp"
#include <memory>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

namespace cs::audio {

OpenAlManager::~OpenAlManager() {
  alcMakeContextCurrent(nullptr);
	alcDestroyContext(mContext);
	alcCloseDevice(mDevice);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenAlManager::initOpenAl(core::Settings::mAudio settings) {
  // create settings for context
  ALCint attrlist[] = {
    ALC_FREQUENCY, settings.pMixerFrequency.get(),
	  ALC_MONO_SOURCES, settings.pNumberMonoSources.get(),
	  ALC_STEREO_SOURCES, settings.pNumberStereoSources.get(),
	  ALC_REFRESH, settings.pRefreshRate.get(),
	  ALC_SYNC, settings.pContextSync.get(),
	  ALC_HRTF_SOFT, settings.pEnableHRTF.get()
  };

  // open default device
  mDevice = alcOpenDevice(nullptr);
  if (!mDevice) {
    logger().warn("Failed to open default device!");
    return false;
  }

  // create context
  mContext = alcCreateContext(mDevice, attrlist);
  if (contextErrorOccurd()) {
    logger().warn("Failed to create context!");
    return false;
  }

  // select context
  ALCboolean contextSelected = alcMakeContextCurrent(mContext);
  if (contextErrorOccurd() || !contextSelected) {
    logger().warn("Faild to select current context!");
    return false;
  }

  return true;
}

bool OpenAlManager::contextErrorOccurd() {
  ALCenum error;
  if ((error = alcGetError(mDevice)) != ALC_NO_ERROR) {

    std::string errorCode;
    switch(error) {
      case ALC_INVALID_DEVICE:
        errorCode = "Invalid device handle";
        break;
      case ALC_INVALID_CONTEXT:
        errorCode = "Invalid context handle";
        break;
      case ALC_INVALID_ENUM:
        errorCode = "Invalid enumeration passed to an ALC call";
        break;
      case ALC_INVALID_VALUE:
        errorCode = "Invalid value passed to an ALC call";
        break;
      case ALC_OUT_OF_MEMORY:
        errorCode = "Not enough memory to execute the ALC call";
        break;
      default:
        errorCode = "Unkown error code";
    }
    logger().warn("OpenAL-Soft Context Error occured! Reason: {}...", errorCode);
    return true;
  }
  return false;
}
} // namespace cs::audio
