////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#ifndef CS_AUDIO_STREAMING_SOURCE_HPP
#define CS_AUDIO_STREAMING_SOURCE_HPP

#include "cs_audio_export.hpp"
#include "internal/SourceBase.hpp"
#include "internal/BufferManager.hpp"
#include "internal/UpdateInstructor.hpp"
#include "internal/FileReader.hpp"

#include <AL/al.h>
#include <map>
#include <any>

namespace cs::audio {

// forward declaration
class SourceGroup;

class CS_AUDIO_EXPORT StreamingSource : public SourceBase {
    
 public:
  ~StreamingSource();

  /// @brief Sets a new file to be played by the source.
  /// @return true if successful
  bool setFile(std::string file) override;

  bool updateStream();  

  StreamingSource(std::string file, int bufferLength, int queueSize,
    std::shared_ptr<UpdateInstructor> UpdateInstructor);

 private:
  void fillBuffer(ALuint buffer);
  bool startStream();

  std::vector<ALuint>     mBuffers;
  AudioContainerStreaming mAudioContainer;
  int                     mBufferLength;
  
  /// Specifies whether buffers should still be filled in a stream update.
  /// Is false if no new buffer is required to play the remaining content.
  bool                    mRefillBuffer;
  /// Specifies whether the source was playing in the last frame
  bool                    mNotPlaying;
};

} // namespace cs::audio

#endif // CS_AUDIO_STREAMING_SOURCE_HPP