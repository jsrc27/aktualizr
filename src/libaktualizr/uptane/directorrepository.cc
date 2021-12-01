#include "directorrepository.h"

#include "storage/invstorage.h"

namespace Uptane {

void DirectorRepository::resetMeta() {
  resetRoot();
  targets = Targets();
  latest_targets = Targets();
  snapshot = Snapshot();
}

void DirectorRepository::verifyOfflineSnapshot(const std::string& snapshot_raw_new, const std::string& snapshot_raw_old) {
  try {
    // Verify the signature:
    snapshot = Snapshot(RepositoryType::Image(), Uptane::Role::OfflineSnapshot(), Utils::parseJSON(snapshot_raw_new), std::make_shared<MetaWithKeys>(root));
  } catch (const Exception& e) {
    LOG_ERROR << "Signature verification for Offline Snapshot metadata failed";
    throw;
  }

  Json::Value target_list_new = Utils::parseJSON(snapshot_raw_new)["signed"]["meta"];
  Json::Value target_list_old = Utils::parseJSON(snapshot_raw_old)["signed"]["meta"];
  for (auto next = target_list_new.begin(); next != target_list_new.end(); ++next) {
    Json::Value target_new = next.key();
    for (auto old = target_list_old.begin(); old != target_list_new.end(); ++old) {
      Json::Value target_old = old.key();
      if (target_new.asString() == target_old.asString()) {
        if (target_old["version"].asInt() > target_new["version"].asInt()) {
          throw Uptane::SecurityException(RepositoryType::DIRECTOR, "Rollback attempt");
        }
      }
    }
  }
}

void DirectorRepository::checkOfflineSnapshotExpired() {
  if (snapshot.isExpired(TimeStamp::Now())) {
    throw Uptane::ExpiredMetadata(type.ToString(), Role::SNAPSHOT);
  }
}

void DirectorRepository::checkTargetsExpired() {
  if (latest_targets.isExpired(TimeStamp::Now())) {
    throw Uptane::ExpiredMetadata(type.ToString(), Role::TARGETS);
  }
}

void DirectorRepository::targetsSanityCheck() {
  //  5.4.4.6.6. If checking Targets metadata from the Director repository,
  //  verify that there are no delegations.
  if (!latest_targets.delegated_role_names_.empty()) {
    throw Uptane::InvalidMetadata(type.ToString(), Role::TARGETS, "Found unexpected delegation.");
  }
  //  5.4.4.6.7. If checking Targets metadata from the Director repository,
  //  check that no ECU identifier is represented more than once.
  std::set<Uptane::EcuSerial> ecu_ids;
  for (const auto& target : targets.targets) {
    for (const auto& ecu : target.ecus()) {
      if (ecu_ids.find(ecu.first) == ecu_ids.end()) {
        ecu_ids.insert(ecu.first);
      } else {
        LOG_ERROR << "ECU " << ecu.first << " appears twice in Director's Targets";
        throw Uptane::InvalidMetadata(type.ToString(), Role::TARGETS, "Found repeated ECU ID.");
      }
    }
  }
}

bool DirectorRepository::usePreviousTargets() const {
  // Don't store the new targets if they are empty and we've previously received
  // a non-empty list.
  return !targets.targets.empty() && latest_targets.targets.empty();
}

void DirectorRepository::verifyTargets(const std::string& targets_raw, bool offline) {
  try {
    // Verify the signature:
    std::shared_ptr<Uptane::Targets> targets_offline_ptr;
    if (offline) {
      latest_targets = Targets(RepositoryType::Director(), Role::OfflineTargets(), Utils::parseJSON(targets_raw),
                               std::make_shared<MetaWithKeys>(root));
      targets_offline_ptr = std::make_shared<Uptane::Targets>(latest_targets);
    } else {
      latest_targets = Targets(RepositoryType::Director(), Role::Targets(), Utils::parseJSON(targets_raw),
                               std::make_shared<MetaWithKeys>(root));
    }
    if (!usePreviousTargets()) {
      targets = latest_targets;
    }
    if (targets_offline_ptr->version() != snapshot.role_version(Uptane::Role::OfflineTargets()) && offline) {
      throw Uptane::VersionMismatch(RepositoryType::DIRECTOR, Uptane::Role::OFFLINETARGETS);
    }
  } catch (const Uptane::Exception& e) {
    LOG_ERROR << "Signature verification for Director Targets metadata failed";
    throw;
  }
}

void DirectorRepository::checkMetaOffline(INvStorage& storage) {
  resetMeta();
  // Load Director Root Metadata
  {
    std::string director_root;
    if (!storage.loadLatestRoot(&director_root, RepositoryType::Director())) {
      throw Uptane::SecurityException(RepositoryType::DIRECTOR, "Could not load latest root");
    }

    initRoot(RepositoryType(RepositoryType::DIRECTOR), director_root);

    if (rootExpired()) {
      throw Uptane::ExpiredMetadata(RepositoryType::DIRECTOR, Role::ROOT);
    }
  }

  // Load Director Targets Metadata
  {
    std::string director_targets;

    if (!storage.loadNonRoot(&director_targets, RepositoryType::Director(), Role::Targets())) {
      throw Uptane::SecurityException(RepositoryType::DIRECTOR, "Could not load Targets role");
    }

    verifyTargets(director_targets, false);

    checkTargetsExpired();

    targetsSanityCheck();
  }
}

void DirectorRepository::updateMeta(INvStorage& storage, const IMetadataFetcher& fetcher, bool offline) {
  // Uptane step 2 (download time) is not implemented yet.
  // Uptane step 3 (download metadata)

  // reset Director repo to initial state before starting Uptane iteration
  resetMeta();

  updateRoot(storage, fetcher, RepositoryType::Director(), offline, director_offline_metadata);

  // Not supported: 3. Download and check the Timestamp metadata file from the Director repository, following the
  // procedure in Section 5.4.4.4. Not supported: 4. Download and check the Snapshot metadata file from the Director
  // repository, following the procedure in Section 5.4.4.5.

  // Update Director Offline Snapshot Metadata
  if (offline) {
    // Load Offline Snapshot metadata from well-known location
    // Then compare with existing offline snapshot version
    std::string director_offline_snapshot;
    fetcher.fetchLatestRoleOffline(&director_offline_snapshot, director_offline_metadata, RepositoryType::Director(), Role::OfflineSnapshot());

    const int fetched_version = extractVersionUntrusted(director_offline_snapshot);
    int local_version;
    std::string director_snapshot_stored;
    if (storage.loadNonRoot(&director_snapshot_stored, RepositoryType::Director(), Role::OfflineSnapshot())) {
      local_version = extractVersionUntrusted(director_snapshot_stored);
    } else {
      local_version = -1;
    }

    if (local_version < fetched_version) {
      // If new Snapshot is more recent then verify and load it into storage
      verifyOfflineSnapshot(director_offline_snapshot, director_snapshot_stored);
      storage.storeNonRoot(director_offline_snapshot, RepositoryType::Director(), Role::OfflineSnapshot());
    } else {
      verifyOfflineSnapshot(director_snapshot_stored, director_snapshot_stored);
    }

    checkOfflineSnapshotExpired();
  }

  // Update Director Targets/Offline Targets Metadata
  if (offline) {
    // Get list of potential offline targets metadata filenames from offline snapshot
    std::string offline_snapshot;
    storage.loadNonRoot(&offline_snapshot, RepositoryType::Director(), Role::OfflineSnapshot());
    Json::Value targets_list = Utils::parseJSON(offline_snapshot)["signed"]["meta"];
    std::string target_file;
    for (auto target = targets_list.begin(); target != targets_list.end(); ++target) {
      std::string filename = target.key().asString();
      if (access(filename.c_str(), R_OK) == 0) {
        target_file = director_offline_metadata + filename;
      }
    }

    if (target_file.size() == 0) {
      throw Uptane::SecurityException(RepositoryType::DIRECTOR, "Could not find any valid offline targets metadata file");
    }

    std::string offline_targets;
    fetcher.fetchRoleFilename(&offline_targets, target_file, RepositoryType::Director());
    verifyTargets(offline_targets, offline);
    storage.storeNonRoot(offline_targets, RepositoryType::Director(), Role::OfflineTargets());

    checkTargetsExpired();

    targetsSanityCheck();
  } else {
    std::string director_targets;

    fetcher.fetchLatestRole(&director_targets, kMaxDirectorTargetsSize, RepositoryType::Director(), Role::Targets());
    int remote_version = extractVersionUntrusted(director_targets);

    int local_version;
    std::string director_targets_stored;
    if (storage.loadNonRoot(&director_targets_stored, RepositoryType::Director(), Role::Targets())) {
      local_version = extractVersionUntrusted(director_targets_stored);
      try {
        verifyTargets(director_targets_stored, offline);
      } catch (const std::exception& e) {
        LOG_WARNING << "Unable to verify stored Director Targets metadata.";
      }
    } else {
      local_version = -1;
    }

    verifyTargets(director_targets, offline);

    // TODO(OTA-4940): check if versions are equal but content is different. In
    // that case, the member variable targets is updated, but it isn't stored in
    // the database, which can cause some minor confusion.
    if (local_version > remote_version) {
      throw Uptane::SecurityException(RepositoryType::DIRECTOR, "Rollback attempt");
    } else if (local_version < remote_version && !usePreviousTargets()) {
      storage.storeNonRoot(director_targets, RepositoryType::Director(), Role::Targets());
    }

    checkTargetsExpired();

    targetsSanityCheck();
  }
}

void DirectorRepository::dropTargets(INvStorage& storage) {
  try {
    storage.clearNonRootMeta(RepositoryType::Director());
    resetMeta();
  } catch (const Uptane::Exception& ex) {
    LOG_ERROR << "Failed to reset Director Targets metadata: " << ex.what();
  }
}

bool DirectorRepository::matchTargetsWithImageTargets(
    const std::shared_ptr<const Uptane::Targets>& image_targets) const {
  // step 10 of https://uptane.github.io/papers/ieee-isto-6100.1.0.0.uptane-standard.html#rfc.section.5.4.4.2
  // TODO(OTA-4800): support delegations. Consider reusing findTargetInDelegationTree(),
  // but it would need to be moved into a common place to be resued by Primary and Secondary.
  // Currently this is only used by aktualizr-secondary, but according to the
  // Standard, "A Secondary ECU MAY elect to perform this check only on the
  // metadata for the image it will install".
  if (image_targets == nullptr) {
    return false;
  }
  const auto& image_target_array = image_targets->targets;
  const auto& director_target_array = targets.targets;

  for (const auto& director_target : director_target_array) {
    auto found_it = std::find_if(
        image_target_array.begin(), image_target_array.end(),
        [&director_target](const Target& image_target) { return director_target.MatchTarget(image_target); });

    if (found_it == image_target_array.end()) {
      return false;
    }
  }

  return true;
}

}  // namespace Uptane
