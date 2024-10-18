#pragma once

#include "types.h"

namespace Kexec {

StringType execute(
    const StringType& command,
    const StringType& arguments = {},
    const StringType& input = {}
);

}