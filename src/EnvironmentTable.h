#ifndef ENVTRACER_ENVIRONMENT_TABLE_H
#define ENVTRACER_ENVIRONMENT_TABLE_H

#include "Environment.h"
#include <unordered_map>
#include <instrumentr/instrumentr.h>

class EnvironmentTable {
  public:
    EnvironmentTable() {
    }

    ~EnvironmentTable() {
        for (auto iter = table_.begin(); iter != table_.end(); ++iter) {
            delete iter->second;
        }
        table_.clear();
    }

    Environment* insert(instrumentr_environment_t environment) {
        int env_id = instrumentr_environment_get_id(environment);

        int call_id = NA_INTEGER;

        instrumentr_environment_type_t type =
            instrumentr_environment_get_type(environment);

        if (type == INSTRUMENTR_ENVIRONMENT_TYPE_CALL) {
            instrumentr_call_t call =
                instrumentr_environment_get_call(environment);
            call_id = instrumentr_call_get_id(call);
        }

        auto iter = table_.find(env_id);

        if (iter != table_.end()) {
            iter->second->set_call_id(call_id);
            return iter->second;
        }

        Environment* env = new Environment(env_id, call_id);

        const char* env_name = instrumentr_environment_get_name(environment);
        env->set_name(env_name);

        const char* env_type = instrumentr_environment_type_to_string(type);
        env->set_type(env_type);

        auto result = table_.insert({env_id, env});
        return result.first->second;
    }

    Environment* lookup(int environment_id) {
        auto result = table_.find(environment_id);
        if (result == table_.end()) {
            return NULL;
        }
        return result->second;
    }

    SEXP to_sexp() {
        int size = table_.size();

        SEXP r_env_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_env_type = PROTECT(allocVector(STRSXP, size));
        SEXP r_env_name = PROTECT(allocVector(STRSXP, size));
        SEXP r_call_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_classes = PROTECT(allocVector(STRSXP, size));
        SEXP r_evals = PROTECT(allocVector(STRSXP, size));
        SEXP r_package = PROTECT(allocVector(STRSXP, size));
        SEXP r_constructor = PROTECT(allocVector(STRSXP, size));
        SEXP r_source_fun_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_source_call_id = PROTECT(allocVector(INTSXP, size));
        SEXP r_dispatch = PROTECT(allocVector(LGLSXP, size));
        SEXP r_backtrace = PROTECT(allocVector(STRSXP, size));

        int index = 0;
        for (auto iter = table_.begin(); iter != table_.end();
             ++iter, ++index) {
            Environment* environment = iter->second;

            environment->to_sexp(index,
                                 r_env_id,
                                 r_env_type,
                                 r_env_name,
                                 r_call_id,
                                 r_classes,
                                 r_evals,
                                 r_package,
                                 r_constructor,
                                 r_source_fun_id,
                                 r_source_call_id,
                                 r_dispatch,
                                 r_backtrace);
        }

        std::vector<SEXP> columns({r_env_id,
                                   r_env_type,
                                   r_env_name,
                                   r_call_id,
                                   r_classes,
                                   r_evals,
                                   r_package,
                                   r_constructor,
                                   r_source_fun_id,
                                   r_source_call_id,
                                   r_dispatch,
                                   r_backtrace});

        std::vector<std::string> names({"env_id",
                                        "env_type",
                                        "env_name",
                                        "call_id",
                                        "class",
                                        "eval",
                                        "package",
                                        "constructor",
                                        "source_fun_id",
                                        "source_call_id",
                                        "dispatch",
                                        "backtrace"});

        SEXP df = create_data_frame(names, columns);

        UNPROTECT(12);

        return df;
    }

  private:
    std::unordered_map<int, Environment*> table_;
};

#endif /* ENVTRACER_ENVIRONMENT_TABLE_H */
