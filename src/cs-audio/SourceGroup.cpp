////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "SourceGroup.hpp"
#include "internal/SourceBase.hpp"
#include "internal/SettingsMixer.hpp"
#include "internal/UpdateInstructor.hpp"
#include "internal/SourceSettings.hpp"

namespace cs::audio {

SourceGroup::SourceGroup(std::shared_ptr<UpdateInstructor> UpdateInstructor, 
  std::shared_ptr<UpdateConstructor> updateConstructor,
  std::shared_ptr<AudioController> audioController) 
  : SourceSettings(std::move(UpdateInstructor))
  , std::enable_shared_from_this<SourceGroup>()
  , mMembers(std::set<std::shared_ptr<SourceBase>>())
  , mUpdateConstructor(std::move(updateConstructor))
  , mAudioController(std::move(audioController)) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SourceGroup::~SourceGroup() {
  reset();
  removeFromUpdateList();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SourceGroup::join(std::shared_ptr<SourceBase> source) {
  if (mAudioController.expired()) {
    logger().warn("Audio Group Warning: Failed to add source to group. Audio controller is expired!");
    return;
  }
  if (!source->mGroup.expired()) {
    logger().warn("Audio Group Warning: Remove source form previous group before assigning a new one!");
    return;
  }
  mMembers.insert(source);
  source->mGroup = shared_from_this();

  // apply group settings to newly added source
  if (!mCurrentSettings->empty()) {
    mUpdateConstructor->applyCurrentGroupSettings(source, mAudioController.lock(), mCurrentSettings);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SourceGroup::remove(std::shared_ptr<SourceBase> sourceToRemove) {
  if (mMembers.erase(sourceToRemove) == 1) {
    sourceToRemove->mGroup = nullptr;

    // TODO: Remove group setting from sources
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SourceGroup::reset() {
  for (auto sourcePtr : mMembers) {
    sourcePtr->mGroup = nullptr;

    // TODO: Remove group setting from sources
  }
  mMembers.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const std::set<std::shared_ptr<SourceBase>> SourceGroup::getMembers() const {
  return mMembers;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SourceGroup::addToUpdateList() {
  mUpdateInstructor->update(shared_from_this());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SourceGroup::removeFromUpdateList() {
  mUpdateInstructor->removeUpdate(shared_from_this());
}

} // namespace cs::audio