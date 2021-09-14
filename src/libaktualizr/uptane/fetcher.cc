#include "fetcher.h"

#include "uptane/exceptions.h"

namespace Uptane {

void Fetcher::fetchRole(std::string* result, int64_t maxsize, RepositoryType repo, const Uptane::Role& role,
                        Version version) const {
  std::string url = (repo == RepositoryType::Director()) ? director_server : repo_server;
  if (role.IsDelegation()) {
    url += "/delegations";
  }
  url += "/" + version.RoleFileName(role);
  HttpResponse response = http->get(url, maxsize);
  if (!response.isOk()) {
    throw Uptane::MetadataFetchFailure(repo.toString(), role.ToString());
  }
  *result = response.body;
}

void IMetadataFetcher::fetchRoleOffline(std::string* result, std::string path, RepositoryType repo, const Uptane::Role& role,
                               Version version) const {
  std::string file = path + "/" + version.RoleFileName(role);
  fetchFile(file, repo, result);
}

void IMetadataFetcher::fetchRoleFilename(std::string* result, std::string file_path, RepositoryType repo) const {
  fetchFile(file_path, repo, result);
}

void IMetadataFetcher::fetchFile(std::string file, RepositoryType repo, std::string* result) const {
  std::ifstream file_input(file);
  if (!file_input.is_open()) {
    throw Uptane::MetadataFetchFailure(repo.toString(), file);
  }

  auto ss = std::ostringstream{};
  ss << file_input.rdbuf();
  *result = ss.str();
}

}  // namespace Uptane
