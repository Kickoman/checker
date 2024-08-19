#pragma once

#include <string>
#include <functional>
#include <optional>

namespace Kexec {

using StringType = std::string;

class Kexeption : public std::exception {
public:
    explicit Kexeption(const StringType& info);
    const char* what() const throw() override;
private:
    const StringType info;
};

}