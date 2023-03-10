#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "anyopt/irbuilder.h"

#include<thorin/def.h>

namespace anyopt {

void print_scope_analysis(IRBuilder& irbuilder, std::string entry_name);

}

#endif
