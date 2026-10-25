// Exception hierarchy used across all layers. The UI layer catches LmsError and
// shows e.what() to the user; DatabaseError is shown generically (details logged).
#pragma once
#include <stdexcept>
#include <string>

namespace lms {

class LmsError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
class ValidationError : public LmsError {
public:
    using LmsError::LmsError;
};
class AuthenticationError : public LmsError {
public:
    using LmsError::LmsError;
};
class AuthorizationError : public LmsError {
public:
    using LmsError::LmsError;
};
class NotFoundError : public LmsError {
public:
    using LmsError::LmsError;
};
class BusinessRuleError : public LmsError {
public:
    using LmsError::LmsError;
};
class DatabaseError : public LmsError {
public:
    using LmsError::LmsError;
};

}  // namespace lms
