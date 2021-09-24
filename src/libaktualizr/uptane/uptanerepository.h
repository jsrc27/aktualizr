#ifndef UPTANE_REPOSITORY_H_
#define UPTANE_REPOSITORY_H_

#include "fetcher.h"

class INvStorage;

namespace Uptane {

class RepositoryCommon {
 public:
  RepositoryCommon(RepositoryType type_in) : type{type_in} {}
  virtual ~RepositoryCommon() = default;
  void initRoot(RepositoryType repo_type, const std::string &root_raw);
  void verifyRoot(const std::string &root_raw);
  int rootVersion() { return root.version(); }
  bool rootExpired() { return root.isExpired(TimeStamp::Now()); }
  virtual void updateMeta(INvStorage &storage, const IMetadataFetcher &fetcher, bool offline) = 0;

 protected:
  void resetRoot();
  void updateRoot(INvStorage &storage, const IMetadataFetcher &fetcher, RepositoryType repo_type);

  static const int64_t kMaxRotations = 1000;

  Root root;
  RepositoryType type;
};
}  // namespace Uptane

#endif
