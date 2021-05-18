#ifndef ENVTRACER_ENVIRONMENT_CONSTRUCTOR_TABLE_H
#define ENVTRACER_ENVIRONMENT_CONSTRUCTOR_TABLE_H

#include "Call.h"
#include <unordered_map>
#include "Function.h"
#include "Environment.h"
#include "EnvironmentConstructor.h"
#include <instrumentr/instrumentr.h>

class EnvironmentConstructorTable {
  public:
    EnvironmentConstructorTable() {
    }

    ~EnvironmentConstructorTable() {
        for (auto iter = table_.begin(); iter != table_.end(); ++iter) {
            delete iter->second;
        }
        table_.clear();
    }

    EnvironmentConstructor* insert(EnvironmentConstructor* env_constructor) {
        auto result =
            table_.insert({env_constructor->get_call_id(), env_constructor});
        return result.first->second;
    }

    EnvironmentConstructor* lookup(int call_id) {
        auto result = table_.find(call_id);
        if (result == table_.end()) {
            Rf_error("cannot find call with id %d", call_id);
        }
        return result->second;
    }

    SEXP to_sexp() {
        int size = table_.size();

        SEXP r_call_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_call_expr = PROTECT(allocVector(STRSXP, size));
        SEXP r_fun_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_fun_name = PROTECT(allocVector(STRSXP, size));
        SEXP r_result_env_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_envir_expr = PROTECT(allocVector(STRSXP, size));
        SEXP r_envir_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_envir_depth = PROTECT(allocVector(INTSXP, size));
        SEXP r_hash_expr = PROTECT(allocVector(STRSXP, size));
        SEXP r_hash = PROTECT(allocVector(LGLSXP, size));
        SEXP r_parent_expr = PROTECT(allocVector(STRSXP, size));
        SEXP r_parent_env_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_parent_env_depth = PROTECT(allocVector(INTSXP, size));
        SEXP r_size_expr = PROTECT(allocVector(STRSXP, size));
        SEXP r_size = PROTECT(allocVector(INTSXP, size));
        SEXP r_chain_length = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id = PROTECT(allocVector(INTSXP, size));

        int index = 0;
        for (auto iter = table_.begin(); iter != table_.end();
             ++iter, ++index) {
            EnvironmentConstructor* env_constructor = iter->second;

            env_constructor->to_sexp(index,
                                     r_call_id,
                                     r_call_expr,
                                     r_fun_id,
                                     r_fun_name,
                                     r_result_env_id,
                                     r_envir_expr,
                                     r_envir_id,
                                     r_envir_depth,
                                     r_hash_expr,
                                     r_hash,
                                     r_parent_expr,
                                     r_parent_env_id,
                                     r_parent_env_depth,
                                     r_size_expr,
                                     r_size,
                                     r_chain_length,
                                     r_source_call_id,
                                     r_source_fun_id);
        }

        std::vector<SEXP> columns({r_call_id,
                                   r_call_expr,
                                   r_fun_id,
                                   r_fun_name,
                                   r_result_env_id,
                                   r_envir_expr,
                                   r_envir_id,
                                   r_envir_depth,
                                   r_hash_expr,
                                   r_hash,
                                   r_parent_expr,
                                   r_parent_env_id,
                                   r_parent_env_depth,
                                   r_size_expr,
                                   r_size,
                                   r_chain_length,
                                   r_source_call_id,
                                   r_source_fun_id});

        std::vector<std::string> names({"call_id",
                                        "call_expr",
                                        "fun_id",
                                        "fun_name",
                                        "result_env_id",
                                        "envir_expr",
                                        "envir_id",
                                        "envir_depth",
                                        "hash_expr",
                                        "hash",
                                        "parent_expr",
                                        "parent_env_id",
                                        "parent_env_depth",
                                        "size_expr",
                                        "size",
                                        "chain_length",
                                        "source_call_id",
                                        "source_fun_id"});

        SEXP df = create_data_frame(names, columns);

        UNPROTECT(18);

        return df;
    }

  private:
    std::unordered_map<int, EnvironmentConstructor*> table_;
};

#endif /* ENVTRACER_ENVIRONMENT_CONSTRUCTOR_TABLE_H */
