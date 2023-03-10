#include "anyopt/analysis.h"

#include <thorin/analyses/scope.h>

namespace anyopt {

void print_scope_analysis(IRBuilder& irbuilder, std::string entry_name) {
    thorin::Continuation* entry = const_cast<thorin::Continuation*>(irbuilder.get_def(entry_name)->as<thorin::Continuation>());
    std::cerr << "Scope analysis for " << entry_name << "\n";

    thorin::Scope scope(entry);

    for (auto it : irbuilder) {
        if (scope.contains(it.second))
            std::cout << it.first << "\n";
    }
}

}
