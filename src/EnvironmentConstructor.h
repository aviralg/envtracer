#ifndef ENVTRACER_ENVIRONMENT_CONSTRUCTOR_H
#define ENVTRACER_ENVIRONMENT_CONSTRUCTOR_H

#include <string>
#include "utilities.h"

class EnvironmentConstructor {
  public:
    EnvironmentConstructor(int env_id,
                           int source_fun_id_1,
                           int source_call_id_1,
                           int source_fun_id_2,
                           int source_call_id_2,
                           int source_fun_id_3,
                           int source_call_id_3,
                           int source_fun_id_4,
                           int source_call_id_4,
                           int hash,
                           int parent_env_id,
                           int parent_env_depth,
                           int size,
                           int frame_count,
                           const std::string& parent_type,
                           const std::string& backtrace)
        : env_id_(env_id)
        , source_fun_id_1_(source_fun_id_1)
        , source_call_id_1_(source_call_id_1)
        , source_fun_id_2_(source_fun_id_2)
        , source_call_id_2_(source_call_id_2)
        , source_fun_id_3_(source_fun_id_3)
        , source_call_id_3_(source_call_id_3)
        , source_fun_id_4_(source_fun_id_4)
        , source_call_id_4_(source_call_id_4)
        , hash_(hash)
        , parent_env_id_(parent_env_id)
        , parent_env_depth_(parent_env_depth)
        , size_(size)
        , frame_count_(frame_count)
        , parent_type_(parent_type)
        , backtrace_(backtrace) {
    }

    void to_sexp(int position,
                 SEXP r_env_id,
                 SEXP r_source_fun_id_1,
                 SEXP r_source_call_id_1,
                 SEXP r_source_fun_id_2,
                 SEXP r_source_call_id_2,
                 SEXP r_source_fun_id_3,
                 SEXP r_source_call_id_3,
                 SEXP r_source_fun_id_4,
                 SEXP r_source_call_id_4,
                 SEXP r_hash,
                 SEXP r_parent_env_id,
                 SEXP r_parent_env_depth,
                 SEXP r_size,
                 SEXP r_frame_count,
                 SEXP r_parent_type,
                 SEXP r_backtrace) {
        SET_INTEGER_ELT(r_env_id, position, env_id_);
        SET_INTEGER_ELT(r_source_fun_id_1, position, source_fun_id_1_);
        SET_INTEGER_ELT(r_source_call_id_1, position, source_call_id_1_);
        SET_INTEGER_ELT(r_source_fun_id_2, position, source_fun_id_2_);
        SET_INTEGER_ELT(r_source_call_id_2, position, source_call_id_2_);
        SET_INTEGER_ELT(r_source_fun_id_3, position, source_fun_id_3_);
        SET_INTEGER_ELT(r_source_call_id_3, position, source_call_id_3_);
        SET_INTEGER_ELT(r_source_fun_id_4, position, source_fun_id_4_);
        SET_INTEGER_ELT(r_source_call_id_4, position, source_call_id_4_);
        SET_INTEGER_ELT(r_hash, position, hash_);
        SET_INTEGER_ELT(r_parent_env_id, position, parent_env_id_);
        SET_INTEGER_ELT(r_parent_env_depth, position, parent_env_depth_);
        SET_INTEGER_ELT(r_size, position, size_);
        SET_INTEGER_ELT(r_frame_count, position, frame_count_);
        SET_STRING_ELT(r_parent_type, position, make_char(parent_type_));
        SET_STRING_ELT(r_backtrace, position, make_char(backtrace_));
    }

  private:
    int env_id_;
    int source_fun_id_1_;
    int source_call_id_1_;
    int source_fun_id_2_;
    int source_call_id_2_;
    int source_fun_id_3_;
    int source_call_id_3_;
    int source_fun_id_4_;
    int source_call_id_4_;
    int hash_;
    int parent_env_id_;
    int parent_env_depth_;
    int size_;
    int frame_count_;
    const std::string parent_type_;
    const std::string backtrace_;
};

#endif /* ENVTRACER_ENVIRONMENT_CONSTRUCTOR_H */
