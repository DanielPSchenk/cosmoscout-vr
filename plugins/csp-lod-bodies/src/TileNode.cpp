////////////////////////////////////////////////////////////////////////////////////////////////////
//                               This file is part of CosmoScout VR                               //
////////////////////////////////////////////////////////////////////////////////////////////////////

// SPDX-FileCopyrightText: German Aerospace Center (DLR) <cosmoscout@dlr.de>
// SPDX-License-Identifier: MIT

#include "TileNode.hpp"

#include "HEALPix.hpp"

namespace csp::lodbodies {

////////////////////////////////////////////////////////////////////////////////////////////////////

TileNode::TileNode(TileId const& tileId)
    : mTileId(tileId) {

  auto baseXY      = HEALPix::getBaseXY(mTileId);
  mTileOffsetScale = glm::ivec3(baseXY.y, baseXY.z, HEALPix::getNSide(mTileId));
  mTileF1F2        = glm::ivec2(HEALPix::getF1(mTileId), HEALPix::getF2(mTileId));
  mCornersLngLat   = HEALPix::getCornersLngLat(mTileId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<TileDataBase> const& TileNode::getTileData(TileDataType type) const {
  return mTileData.get(type);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

PerDataType<std::shared_ptr<TileDataBase>> const& TileNode::getTileData() const {
  return mTileData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileNode::setTileData(std::shared_ptr<TileDataBase> tile) {
  mTileData.set(tile->getDataType(), std::move(tile));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TileNode* TileNode::getChild(int childIdx) const {
  return mChildren.at(childIdx).get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileNode::setChild(int childIdx, TileNode* child) {
  // unset OLD parent
  if (mChildren.at(childIdx)) {
    mChildren.at(childIdx)->setParent(nullptr);
  }

  mChildren.at(childIdx).reset(child);

  // set NEW parent
  if (mChildren.at(childIdx)) {
    mChildren.at(childIdx)->setParent(this);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TileNode* TileNode::getParent() const {
  return mParent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileNode::setParent(TileNode* parent) {
  mParent = parent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool TileNode::childrenAvailable() const {
  for (int i = 0; i < 4; ++i) {

    // child is not loaded -> can not refine
    if (!mChildren[i]) {
      return false;
    }

    auto dem = mChildren[i]->getTileData(TileDataType::eElevation);
    auto img = mChildren[i]->getTileData(TileDataType::eColor);

    // child is not on GPU -> can not refine
    if (dem->getTexLayer() < 0 || (img && img->getTexLayer() < 0)) {
      return false;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int TileNode::getLevel() const {
  return mTileId.level();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

glm::int64 TileNode::getPatchIdx() const {
  return mTileId.patchIdx();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int TileNode::getLastFrame() const {
  return mLastFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileNode::setLastFrame(int frame) {
  mLastFrame = frame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int TileNode::getAge(int frame) const {
  return frame - mLastFrame;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

BoundingBox<double> const& TileNode::getBounds() const {
  return mTb;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileNode::setBounds(BoundingBox<double> const& tb) {
  mTb        = tb;
  mHasBounds = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileNode::removeBounds() {
  mTb        = BoundingBox<double>();
  mHasBounds = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool TileNode::hasBounds() const {
  return mHasBounds;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TileId const& TileNode::getTileId() const {
  return mTileId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

MinMaxPyramid* TileNode::getMinMaxPyramid() const {
  return mMinMaxPyramid.get();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void TileNode::setMinMaxPyramid(std::unique_ptr<MinMaxPyramid> pyramid) {
  mMinMaxPyramid = std::move(pyramid);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

glm::ivec3 const& TileNode::getTileOffsetScale() const {
  return mTileOffsetScale;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

glm::ivec2 const& TileNode::getTileF1F2() const {
  return mTileF1F2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

std::array<glm::dvec2, 4> const& TileNode::getCornersLngLat() const {
  return mCornersLngLat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace csp::lodbodies
