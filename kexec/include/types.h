#pragma once

#include <string>
#include <functional>
#include <optional>

namespace Kexec {

using StringType = std::string;

class KException : public std::exception {
public:
    explicit KException(const StringType& info);
    const char* what() const throw() override;
private:
    const StringType info;
};

}