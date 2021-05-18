#ifndef ENVTRACER_ENVIRONMENT_CONSTRUCTOR_H
#define ENVTRACER_ENVIRONMENT_CONSTRUCTOR_H

#include <string>
#include "utilities.h"

class EnvironmentConstructor {
  public:
    EnvironmentConstructor(int call_id,
                           const std::string& call_expr,
                           int fun_id,
                           const std::string& fun_name,
                           int result_env_id,
                           const std::string& envir_expr,
                           int envir_id,
                           int envir_depth,
                           const std::string& hash_expr,
                           int hash,
                           const std::string& parent_expr,
                           int parent_env_id,
                           int parent_env_depth,
                           const std::string& size_expr,
                           int size,
                           int chain_length,
                           int source_call_id,
                           int source_fun_id)
        : call_id_(call_id)
        , call_expr_(call_expr)
        , fun_id_(fun_id)
        , fun_name_(fun_name)
        , result_env_id_(result_env_id)
        , envir_expr_(envir_expr)
        , envir_id_(envir_id)
        , envir_depth_(envir_depth)
        , hash_expr_(hash_expr)
        , hash_(hash)
        , parent_expr_(parent_expr)
        , parent_env_id_(parent_env_id)
        , parent_env_depth_(parent_env_depth)
        , size_expr_(size_expr)
        , size_(size)
        , chain_length_(chain_length)
        , source_call_id_(source_call_id)
        , source_fun_id_(source_fun_id) {
    }

    int get_call_id() const {
        return call_id_;
    }

    void to_sexp(int position,
                 SEXP r_call_id,
                 SEXP r_call_expr,
                 SEXP r_fun_id,
                 SEXP r_fun_name,
                 SEXP r_result_env_id,
                 SEXP r_envir_expr,
                 SEXP r_envir_id,
                 SEXP r_envir_depth,
                 SEXP r_hash_expr,
                 SEXP r_hash,
                 SEXP r_parent_expr,
                 SEXP r_parent_env_id,
                 SEXP r_parent_env_depth,
                 SEXP r_size_expr,
                 SEXP r_size,
                 SEXP r_chain_length,
                 SEXP r_source_call_id,
                 SEXP r_source_fun_id) {
        SET_INTEGER_ELT(r_call_id, position, call_id_);
        SET_STRING_ELT(r_call_expr, position, make_char(call_expr_));
        SET_INTEGER_ELT(r_fun_id, position, fun_id_);
        SET_STRING_ELT(r_fun_name, position, make_char(fun_name_));
        SET_INTEGER_ELT(r_result_env_id, position, result_env_id_);
        SET_STRING_ELT(r_envir_expr, position, make_char(envir_expr_));
        SET_INTEGER_ELT(r_envir_id, position, envir_id_);
        SET_INTEGER_ELT(r_envir_depth, position, envir_depth_);
        SET_STRING_ELT(r_hash_expr, position, make_char(hash_expr_));
        SET_LOGICAL_ELT(r_hash, position, hash_);
        SET_STRING_ELT(r_parent_expr, position, make_char(parent_expr_));
        SET_INTEGER_ELT(r_parent_env_id, position, parent_env_id_);
        SET_INTEGER_ELT(r_parent_env_depth, position, parent_env_depth_);
        SET_STRING_ELT(r_size_expr, position, make_char(size_expr_));
        SET_INTEGER_ELT(r_size, position, size_);
        SET_INTEGER_ELT(r_chain_length, position, chain_length_);
        SET_INTEGER_ELT(r_source_call_id, position, source_call_id_);
        SET_INTEGER_ELT(r_source_fun_id, position, source_fun_id_);
    }

  private:
    int call_id_;
    std::string call_expr_;
    int fun_id_;
    std::string fun_name_;
    int result_env_id_;
    std::string envir_expr_;
    int envir_id_;
    int envir_depth_;
    std::string hash_expr_;
    int hash_;
    std::string parent_expr_;
    int parent_env_id_;
    int parent_env_depth_;
    std::string size_expr_;
    int size_;
    int chain_length_;
    int source_call_id_;
    int source_fun_id_;
};

#endif /* ENVTRACER_ENVIRONMENT_CONSTRUCTOR_H */
