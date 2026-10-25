// Single place that answers "what deadline applies to this student?" (extension rule, FR-09/FR-10).
#pragma once
#include <algorithm>

#include "data/Repositories.h"

namespace lms {

class DeadlineResolver {
public:
    explicit DeadlineResolver(IExtensionRepository& extensions) : extensions_(extensions) {}
    DateTime effectiveDeadline(const Assignment& a, Id studentId) const {
        if (const auto ext = extensions_.find(a.id, studentId))
            return std::max(a.deadline, ext->revisedDeadline);
        return a.deadline;
    }

private:
    IExtensionRepository& extensions_;
};

}  // namespace lms
