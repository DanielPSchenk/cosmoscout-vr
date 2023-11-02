////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "../logger.hpp"
#include "UpdateInstructor.hpp"

namespace cs::audio {

UpdateInstructor::UpdateInstructor() 
  : mSourceUpdateList(std::set<std::shared_ptr<Source>>())
  , mGroupUpdateList(std::set<std::shared_ptr<SourceGroup>>())
  , mAudioControllerUpdate(false) { 
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void UpdateInstructor::update(std::shared_ptr<Source> source) {
  mSourceUpdateList.insert(source);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void UpdateInstructor::update(std::shared_ptr<SourceGroup> sourceGroup) {
  mGroupUpdateList.insert(sourceGroup);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void UpdateInstructor::update(std::shared_ptr<AudioController> audioController) {
  mAudioControllerUpdate = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

UpdateInstructor::UpdateInstruction UpdateInstructor::createUpdateInstruction() {
  UpdateInstruction result;

  if (mAudioControllerUpdate) {
    // update every source and group
    result.updateAll = true;
    return result;
  }

  result.updateWithGroup = std::make_shared<std::vector<std::shared_ptr<Source>>>();
  result.updateSourceOnly = std::make_shared<std::vector<std::shared_ptr<Source>>>();

  // add group members to updateList
  for (auto groupPtr : mGroupUpdateList) {
    auto groupMembers = groupPtr->getMembers();   
    result.updateWithGroup->insert(std::end(*(result.updateWithGroup)), std::begin(groupMembers), std::end(groupMembers));
  }

  // Compute mSourceUpdateList without result.updateWithGroup in order to later only process sources 
  // that are not already in the group update.
  for (auto sourcePtr : mSourceUpdateList) {
    if (std::find(result.updateWithGroup->begin(), result.updateWithGroup->end(), sourcePtr) == result.updateWithGroup->end()) {
      result.updateSourceOnly->push_back(sourcePtr);
    }
  }

  // reset update state
  mSourceUpdateList.clear();
  mGroupUpdateList.clear();
  mAudioControllerUpdate = false;

  return result;
}

} // namespace cs::audio