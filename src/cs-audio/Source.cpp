////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "Source.hpp"
#include "../cs-core/AudioEngine.hpp"
#include "internal/BufferManager.hpp"

#include <AL/al.h>

namespace cs::audio {

Source::Source(std::shared_ptr<BufferManager> bufferManager, std::string file) 
  : mFile(std::move(file)) 
  , mBufferManager(std::move(bufferManager)) {

  // generate new source  
  alGenSources((ALuint)1, &mOpenAlId);

  // TODO: check if file actually exists

  // bind buffer to source
  alSourcei(mOpenAlId, AL_BUFFER, mBufferManager->getBuffer(file));
  // TODO: Error handling
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Source::~Source() {
  
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool Source::play() {
  alSourcePlay(mOpenAlId);
  // TODO: Error handling
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool Source::stop() {
  alSourceStop(mOpenAlId);
  // TODO: Error handling
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Source::update() {
    
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool Source::setFile(std::string file) {
  mFile = file;
  // TODO: check if file exists
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::string Source::getFile() const {
  return mFile;   
}

} // namespace cs::audio