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
    EnvironmentAccessTable(): library_counter_(0) {
    }

    ~EnvironmentAccessTable() {
        for (auto iter = table_.begin(); iter != table_.end(); ++iter) {
            delete *iter;
        }
        table_.clear();
    }

    EnvironmentAccess* insert(EnvironmentAccess* env_access) {
        table_.push_back(env_access);
        return env_access;
    }

    void push_library() {
        ++library_counter_;
    }

    void pop_library() {
        --library_counter_;
    }

    bool inside_library() const {
        return library_counter_ > 0;
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
        SEXP r_fun_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_symbol = PROTECT(allocVector(STRSXP, size));
        SEXP r_bindings = PROTECT(allocVector(LGLSXP, size));
        SEXP r_env_id = PROTECT(allocVector(INTSXP, size));

        int index = 0;
        for (auto iter = table_.begin(); iter != table_.end();
             ++iter, ++index) {
            EnvironmentAccess* env_access = *iter;

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
                                r_x_char,
                                r_fun_id,
                                r_symbol,
                                r_bindings,
                                r_env_id);
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
                                   r_x_char,
                                   r_fun_id,
                                   r_symbol,
                                   r_bindings,
                                   r_env_id});

        std::vector<std::string> names({"call_id",
                                        "depth",
                                        "fun_name",
                                        "source_fun_id",
                                        "source_call_id",
                                        "arg_type",
                                        "n",
                                        "which",
                                        "x_int",
                                        "x_char",
                                        "fun_id",
                                        "symbol",
                                        "bindings",
                                        "env_id"});

        SEXP df = create_data_frame(names, columns);

        UNPROTECT(14);

        return df;
    }

  private:
    int library_counter_;
    std::vector<EnvironmentAccess*> table_;
};

#endif /* ENVTRACER_ENVIRONMENT_ACCESS_TABLE_H */
