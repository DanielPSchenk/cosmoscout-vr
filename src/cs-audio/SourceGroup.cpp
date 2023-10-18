////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "SourceGroup.hpp"
#include "Source.hpp"
#include "internal/SettingsMixer.hpp"
#include "internal/UpdateBuilder.hpp"
#include "internal/SourceSettings.hpp"

namespace cs::audio {

SourceGroup::SourceGroup(std::shared_ptr<UpdateBuilder> updateBuilder) 
  : SourceSettings(updateBuilder)
  , mMemberSources(std::set<std::shared_ptr<Source>>()) {
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SourceGroup::~SourceGroup() {
  reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SourceGroup::add(std::shared_ptr<Source> source) {
  if (source->mGroup != nullptr) {
    // TODO: automatic reassignment
    logger().warn("Audio Group Warning: Remove Source form previous group before assigning a new one!");
    return;
  }
  mMemberSources.insert(source);
  source->mGroup = std::shared_ptr<SourceGroup>(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SourceGroup::remove(std::shared_ptr<Source> sourceToRemove) {
  // if removal was successful
  if (mMemberSources.erase(sourceToRemove) == 1) {
    sourceToRemove->mGroup = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SourceGroup::reset() {
  for (auto source : mMemberSources) {
    source->mGroup = nullptr;
  }
  mMemberSources.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::set<std::shared_ptr<Source>> SourceGroup::getMembers() const {
  return mMemberSources;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SourceGroup::addToUpdateList() {
  mUpdateBuilder->update(this);
}

} // namespace cs::audio