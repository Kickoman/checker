#include "types.h"


namespace Kexec {

KException::KException(const StringType& info)
    : info(info)
{}

const char* KException::what() const throw() {
    return info.c_str();
}

}