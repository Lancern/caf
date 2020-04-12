#include "Infrastructure/Memory.h"
#include "Basic/CAFStore.h"

#include <utility>

namespace caf {

bool CAFStore::Entry::HasChild(const std::string& name) const {
  return _children.find(name) != _children.end();
}

void CAFStore::Entry::AddChild(const std::string& name, std::unique_ptr<Entry> entry) {
  _children.emplace(name, std::move(entry));
}

CAFStore::Entry* CAFStore::Entry::GetChild(const std::string& name) const {
  auto i = _children.find(name);
  if (i == _children.end()) {
    return nullptr;
  }
  return i->second.get();
}

void CAFStore::Entry::AddDescendent(Entry* entry) {
  _descendents.push_back(entry);
}

CAFStore::CAFStore()
  : _root(caf::make_unique<Entry>()),
    _entries { _root.get() },
    _funcIdToEntry()
{ }

CAFStore::CAFStore(CAFStore &&) noexcept = default;

CAFStore& CAFStore::operator=(CAFStore &&) = default;

CAFStore::~CAFStore() = default;

void CAFStore::Load(const nlohmann::json &json) {
  FunctionIdType funcId = 0;
  for (const auto& funcJson : json) {
    auto funcName = funcJson.get<std::string>();
    AddFunction(Function { funcId++, std::move(funcName) });
  }
}

size_t CAFStore::GetFunctionsCount() const {
  size_t count = 0;
  for (auto entry : _entries) {
    if (entry->HasFunction()) {
      ++count;
    }
  }
  return count;
}

void CAFStore::AddFunction(Function func) {
  auto id = func.id();
  const auto& name = func.name();
  auto node = _root.get();

  std::vector<Entry *> path { node };
  size_t start = 0;
  while (start < name.length()) {
    size_t sep = name.find('.', start);
    if (sep == std::string::npos) {
      sep = name.length();
    }

    auto component = name.substr(start, sep - start);

    if (!node->HasChild(component)) {
      auto entry = CreateEntry();
      node->AddChild(component, std::move(entry));
    }
    node = node->GetChild(component);
    path.push_back(node);

    start = sep + 1;
  }

  node->SetFunction(std::move(func));
  for (auto p : path) {
    p->AddDescendent(node);
  }

  _funcIdToEntry.emplace(id, node);
}

CAFStore::Statistics CAFStore::GetStatistics() const {
  Statistics stat;
  stat.ApiFunctionsCount = GetFunctionsCount();
  return stat;
}

nlohmann::json CAFStore::ToJson() const {
  auto json = nlohmann::json::array();
  for (auto entry : _entries) {
    if (!entry->HasFunction()) {
      continue;
    }
    json.push_back(entry->GetFunction().name());
  }
  return json;
}

std::unique_ptr<CAFStore::Entry> CAFStore::CreateEntry() {
  auto entry = caf::make_unique<Entry>();
  _entries.push_back(entry.get());
  return entry;
}

} // namespace caf
