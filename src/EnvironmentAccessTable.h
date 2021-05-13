#ifndef ENVTRACER_ENVIRONMENT_ACCESS_TABLE_H
#define ENVTRACER_ENVIRONMENT_ACCESS_TABLE_H

#include "Call.h"
#include <unordered_map>
#include "Function.h"
#include "Environment.h"
#include "EnvironmentAccess.h"
#include <instrumentr/instrumentr.h>

class EnvironmentAccessTable {
  public:
    EnvironmentAccessTable() {
    }

    ~EnvironmentAccessTable() {
        for (auto iter = table_.begin(); iter != table_.end(); ++iter) {
            delete iter->second;
        }
        table_.clear();
    }

    EnvironmentAccess* insert(EnvironmentAccess* env_access) {
        auto result = table_.insert({env_access->get_call_id(), env_access});
        return result.first->second;
    }

    EnvironmentAccess* lookup(int call_id) {
        auto result = table_.find(call_id);
        if (result == table_.end()) {
            Rf_error("cannot find call with id %d", call_id);
        }
        return result->second;
    }

    SEXP to_sexp() {
        int size = table_.size();

        SEXP r_call_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_depth = PROTECT(allocVector(INTSXP, size));
        SEXP r_fun_name = PROTECT(allocVector(STRSXP, size));
        SEXP r_source_fun_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_arg_type = PROTECT(allocVector(STRSXP, size));
        SEXP r_n = PROTECT(allocVector(INTSXP, size));
        SEXP r_which = PROTECT(allocVector(INTSXP, size));
        SEXP r_x_int = PROTECT(allocVector(INTSXP, size));
        SEXP r_x_char = PROTECT(allocVector(STRSXP, size));

        int index = 0;
        for (auto iter = table_.begin(); iter != table_.end();
             ++iter, ++index) {
            EnvironmentAccess* env_access = iter->second;

            env_access->to_sexp(index,
                                r_call_id,
                                r_depth,
                                r_fun_name,
                                r_source_fun_id,
                                r_source_call_id,
                                r_arg_type,
                                r_n,
                                r_which,
                                r_x_int,
                                r_x_char);
        }

        std::vector<SEXP> columns({r_call_id,
                                   r_depth,
                                   r_fun_name,
                                   r_source_fun_id,
                                   r_source_call_id,
                                   r_arg_type,
                                   r_n,
                                   r_which,
                                   r_x_int,
                                   r_x_char});

        std::vector<std::string> names({"call_id",
                                        "depth",
                                        "fun_name",
                                        "source_fun_id",
                                        "source_call_id",
                                        "arg_type",
                                        "n",
                                        "which",
                                        "x_int",
                                        "x_char"});

        SEXP df = create_data_frame(names, columns);

        UNPROTECT(10);

        return df;
    }

  private:
    std::unordered_map<int, EnvironmentAccess*> table_;
};

#endif /* ENVTRACER_ENVIRONMENT_ACCESS_TABLE_H */
