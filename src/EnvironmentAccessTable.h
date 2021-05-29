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

        SEXP r_time = PROTECT(allocVector(INTSXP, size));
        SEXP r_depth = PROTECT(allocVector(INTSXP, size));
        SEXP r_fun_name = PROTECT(allocVector(STRSXP, size));
        SEXP r_result_env_type = PROTECT(allocVector(STRSXP, size));
        SEXP r_result_env_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_arg_env_type_1 = PROTECT(allocVector(STRSXP, size));
        SEXP r_arg_env_id_1 = PROTECT(allocVector(INTSXP, size));
        SEXP r_arg_env_type_2 = PROTECT(allocVector(STRSXP, size));
        SEXP r_arg_env_id_2 = PROTECT(allocVector(INTSXP, size));
        SEXP r_env_name = PROTECT(allocVector(STRSXP, size));
        SEXP r_symbol = PROTECT(allocVector(STRSXP, size));
        SEXP r_bindings = PROTECT(allocVector(INTSXP, size));
        SEXP r_fun_type = PROTECT(allocVector(STRSXP, size));
        SEXP r_fun_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_n_type = PROTECT(allocVector(STRSXP, size));
        SEXP r_n = PROTECT(allocVector(INTSXP, size));
        SEXP r_which_type = PROTECT(allocVector(STRSXP, size));
        SEXP r_which = PROTECT(allocVector(INTSXP, size));
        SEXP r_x_type = PROTECT(allocVector(STRSXP, size));
        SEXP r_x_int = PROTECT(allocVector(INTSXP, size));
        SEXP r_x_char = PROTECT(allocVector(STRSXP, size));
        SEXP r_seq_env_id = PROTECT(allocVector(STRSXP, size));
        SEXP r_se_env_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_se_val_type = PROTECT(allocVector(STRSXP, size));
        SEXP r_source_fun_id_1 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_1 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_2 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_2 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_3 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_3 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_4 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_4 = PROTECT(allocVector(INTSXP, size));
        SEXP r_backtrace = PROTECT(allocVector(STRSXP, size));

        for (int index = 0; index < size; ++index) {
            EnvironmentAccess* env_access = table_[index];

            env_access->to_sexp(index,
                                r_time,
                                r_depth,
                                r_fun_name,
                                r_result_env_type,
                                r_result_env_id,
                                r_arg_env_type_1,
                                r_arg_env_id_1,
                                r_arg_env_type_2,
                                r_arg_env_id_2,
                                r_env_name,
                                r_symbol,
                                r_bindings,
                                r_fun_type,
                                r_fun_id,
                                r_n_type,
                                r_n,
                                r_which_type,
                                r_which,
                                r_x_type,
                                r_x_int,
                                r_x_char,
                                r_seq_env_id,
                                r_se_env_id,
                                r_se_val_type,
                                r_source_fun_id_1,
                                r_source_call_id_1,
                                r_source_fun_id_2,
                                r_source_call_id_2,
                                r_source_fun_id_3,
                                r_source_call_id_3,
                                r_source_fun_id_4,
                                r_source_call_id_4,
                                r_backtrace);
        }

        std::vector<SEXP> columns({r_time,
                                   r_depth,
                                   r_fun_name,
                                   r_result_env_type,
                                   r_result_env_id,
                                   r_arg_env_type_1,
                                   r_arg_env_id_1,
                                   r_arg_env_type_2,
                                   r_arg_env_id_2,
                                   r_env_name,
                                   r_symbol,
                                   r_bindings,
                                   r_fun_type,
                                   r_fun_id,
                                   r_n_type,
                                   r_n,
                                   r_which_type,
                                   r_which,
                                   r_x_type,
                                   r_x_int,
                                   r_x_char,
                                   r_seq_env_id,
                                   r_se_env_id,
                                   r_se_val_type,
                                   r_source_fun_id_1,
                                   r_source_call_id_1,
                                   r_source_fun_id_2,
                                   r_source_call_id_2,
                                   r_source_fun_id_3,
                                   r_source_call_id_3,
                                   r_source_fun_id_4,
                                   r_source_call_id_4,
                                   r_backtrace});

        std::vector<std::string> names({"time",
                                        "depth",
                                        "fun_name",
                                        "result_env_type",
                                        "result_env_id",
                                        "arg_env_type_1",
                                        "arg_env_id_1",
                                        "arg_env_type_2",
                                        "arg_env_id_2",
                                        "env_name",
                                        "symbol",
                                        "bindings",
                                        "fun_type",
                                        "fun_id",
                                        "n_type",
                                        "n",
                                        "which_type",
                                        "which",
                                        "x_type",
                                        "x_int",
                                        "x_char",
                                        "seq_env_id",
                                        "se_env_id",
                                        "se_val_type",
                                        "source_fun_id_1",
                                        "source_call_id_1",
                                        "source_fun_id_2",
                                        "source_call_id_2",
                                        "source_fun_id_3",
                                        "source_call_id_3",
                                        "source_fun_id_4",
                                        "source_call_id_4",
                                        "backtrace"});

        SEXP df = create_data_frame(names, columns);

        UNPROTECT(33);

        return df;
    }

  private:
    int library_counter_;
    std::vector<EnvironmentAccess*> table_;
};

#endif /* ENVTRACER_ENVIRONMENT_ACCESS_TABLE_H */
