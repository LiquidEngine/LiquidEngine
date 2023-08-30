#pragma once

#include "quoll/core/Uuid.h"
#include "AssetData.h"

namespace quoll {

/**
 * @brief Asset map
 *
 * Store all the assets of a specific type
 *
 * @tparam THandle Asset handle type
 * @tparam TData Asset data type
 */
template <class THandle, class TData> class AssetMap {
public:
  /**
   * Asset handle
   */
  using Handle = THandle;

public:
  /**
   * @brief Add asset
   *
   * @param data Asset data
   * @return New asset handle
   */
  THandle addAsset(const AssetData<TData> &data) {
    auto handle = getNewHandle();
    mAssets.insert_or_assign(handle, data);
    return handle;
  }

  /**
   * @brief Update asset
   *
   * @param handle Asset handle
   * @param data Asset data
   */
  void updateAsset(THandle handle, const AssetData<TData> &data) {
    QuollAssert(mAssets.find(handle) != mAssets.end(), "Asset does not exist");
    mAssets.at(handle) = data;
  }

  /**
   * @brief Get asset
   *
   * @param handle Asset handle
   * @return Asset
   */
  const AssetData<TData> &getAsset(THandle handle) const {
    return mAssets.at(handle);
  }

  /**
   * @brief Get asset
   *
   * @param handle Asset handle
   * @return Asset
   */
  AssetData<TData> &getAsset(THandle handle) { return mAssets.at(handle); }

  /**
   * @brief Get all assets
   *
   * @return List of all assets
   */
  inline const std::unordered_map<THandle, AssetData<TData>> &
  getAssets() const {
    return mAssets;
  }

  /**
   * @brief Find asset handle by uuid
   *
   * @param uuid Asset uuid
   * @return Handle
   */
  inline THandle findHandleByUuid(const Uuid &uuid) const {
    for (auto &[handle, data] : mAssets) {
      if (data.uuid == uuid) {
        return handle;
      }
    }

    return THandle::Null;
  }

  /**
   * @brief Get all assets
   *
   * @return List of all assets
   */
  inline std::unordered_map<THandle, AssetData<TData>> &getAssets() {
    return mAssets;
  }

  /**
   * @brief Check if asset exists
   *
   * @param handle Asset handle
   * @retval true Asset exists
   * @retval false Asset does not exist
   */
  inline bool hasAsset(THandle handle) const {
    return mAssets.find(handle) != mAssets.end();
  }

  /**
   * @brief Delete asset
   *
   * @param handle Asset handle
   */
  void deleteAsset(THandle handle) { mAssets.erase(handle); }

private:
  THandle getNewHandle() {
    THandle handle = mLastHandle;
    mLastHandle = THandle{static_cast<uint32_t>(mLastHandle) + 1};
    return handle;
  }

private:
  std::unordered_map<THandle, AssetData<TData>> mAssets;
  THandle mLastHandle{1};
};

} // namespace quoll
