////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#ifndef CS_AUDIO_BUFFER_MANAGER_HPP
#define CS_AUDIO_BUFFER_MANAGER_HPP

#include "cs_audio_export.hpp"

#include <string>
#include <vector>
#include <AL/al.h>

namespace cs::audio {

struct Buffer {
  std::string mFile;
  int         mUsageNumber;
  ALuint      mOpenAlId;

  Buffer(std::string file, ALuint openAlId) 
    : mFile(std::move(file))
    , mOpenAlId(std::move(openAlId)) {
    mUsageNumber = 1;  
  }
};

class CS_AUDIO_EXPORT BufferManager {
 public:
  ~BufferManager();

  // returns an OpenAL id to a buffer for this file; The BufferManager will
  // check if a buffer for this file already exists and if so reuse the existing one
  ALuint getBuffer(std::string file);
  // signals to the bufferManager that a Source is not using a buffer to this file anymore
  void removeBuffer(std::string file);
  
 private:
  std::vector<std::shared_ptr<Buffer>> mBufferList;
  
  // creates a new buffer
  ALuint createBuffer(std::string file);
  // deletes a buffer if it is not used in any source
  void deleteBuffer(std::shared_ptr<Buffer> buffer);
};

} // namespace cs::audio

#endif // CS_AUDIO_BUFFER_MANAGER_HPP