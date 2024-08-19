#include "types.h"


namespace Kexec {

Kexeption::Kexeption(const StringType& info)
    : info(info)
{}

const char* Kexeption::what() const throw() {
    return info.c_str();
}

}