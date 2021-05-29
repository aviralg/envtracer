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
            delete *iter;
        }
        table_.clear();
    }

    EnvironmentConstructor* insert(EnvironmentConstructor* env_constructor) {
        table_.push_back(env_constructor);
        return env_constructor;
    }

    SEXP to_sexp() {
        int size = table_.size();

        SEXP r_env_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_1 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_1 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_2 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_2 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_3 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_3 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_fun_id_4 = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id_4 = PROTECT(allocVector(INTSXP, size));
        SEXP r_hash = PROTECT(allocVector(INTSXP, size));
        SEXP r_parent_env_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_parent_env_depth = PROTECT(allocVector(INTSXP, size));
        SEXP r_size = PROTECT(allocVector(INTSXP, size));
        SEXP r_frame_count = PROTECT(allocVector(INTSXP, size));
        SEXP r_backtrace = PROTECT(allocVector(STRSXP, size));

        for (int index = 0; index < table_.size(); ++index) {
            EnvironmentConstructor* env_constructor = table_[index];

            env_constructor->to_sexp(index,
                                     r_env_id,
                                     r_source_fun_id_1,
                                     r_source_call_id_1,
                                     r_source_fun_id_2,
                                     r_source_call_id_2,
                                     r_source_fun_id_3,
                                     r_source_call_id_3,
                                     r_source_fun_id_4,
                                     r_source_call_id_4,
                                     r_hash,
                                     r_parent_env_id,
                                     r_parent_env_depth,
                                     r_size,
                                     r_frame_count,
                                     r_backtrace);
        }

        std::vector<SEXP> columns({r_env_id,
                                   r_source_fun_id_1,
                                   r_source_call_id_1,
                                   r_source_fun_id_2,
                                   r_source_call_id_2,
                                   r_source_fun_id_3,
                                   r_source_call_id_3,
                                   r_source_fun_id_4,
                                   r_source_call_id_4,
                                   r_hash,
                                   r_parent_env_id,
                                   r_parent_env_depth,
                                   r_size,
                                   r_frame_count,
                                   r_backtrace});

        std::vector<std::string> names({"env_id",
                                        "source_fun_id_1",
                                        "source_call_id_1",
                                        "source_fun_id_2",
                                        "source_call_id_2",
                                        "source_fun_id_3",
                                        "source_call_id_3",
                                        "source_fun_id_4",
                                        "source_call_id_4",
                                        "hash",
                                        "parent_env_id",
                                        "parent_env_depth",
                                        "size",
                                        "frame_count",
                                        "backtrace"});

        SEXP df = create_data_frame(names, columns);

        UNPROTECT(15);

        return df;
    }

  private:
    std::vector<EnvironmentConstructor*> table_;
};

#endif /* ENVTRACER_ENVIRONMENT_CONSTRUCTOR_TABLE_H */
