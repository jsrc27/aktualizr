#ifndef UPTANE_SECONDARY_PROVIDER_BUILDER_H
#define UPTANE_SECONDARY_PROVIDER_BUILDER_H

#include <memory>

#include "libaktualizr/secondary_provider.h"

class SecondaryProviderBuilder {
 public:
  static std::shared_ptr<SecondaryProvider> Build(
      Config& config, const std::shared_ptr<const INvStorage>& storage,
      const std::shared_ptr<const PackageManagerInterface>& package_manager) {
    return std::make_shared<SecondaryProvider>(SecondaryProvider(config, storage, package_manager));
  }
  SecondaryProviderBuilder(SecondaryProviderBuilder&&) = delete;
  SecondaryProviderBuilder(const SecondaryProviderBuilder&) = delete;
  SecondaryProviderBuilder& operator=(const SecondaryProviderBuilder&) = delete;

 private:
  SecondaryProviderBuilder() {}
};
#endif  // UPTANE_SECONDARY_PROVIDER_BUILDER_H
