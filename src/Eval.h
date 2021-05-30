#ifndef ENVTRACER_EVAL_H
#define ENVTRACER_EVAL_H

#include <string>
#include "utilities.h"

class Eval {
  public:
    Eval(int time,
         int env_id,
         int direct,
         const std::string& expression,
         int source_fun_id_1,
         int source_call_id_1,
         int source_fun_id_2,
         int source_call_id_2,
         int source_fun_id_3,
         int source_call_id_3,
         int source_fun_id_4,
         int source_call_id_4,
         const std::string& backtrace)
        : time_(time)
        , env_id_(env_id)
        , direct_(direct)
        , expression_(expression)
        , source_fun_id_1_(source_fun_id_1)
        , source_call_id_1_(source_call_id_1)
        , source_fun_id_2_(source_fun_id_2)
        , source_call_id_2_(source_call_id_2)
        , source_fun_id_3_(source_fun_id_3)
        , source_call_id_3_(source_call_id_3)
        , source_fun_id_4_(source_fun_id_4)
        , source_call_id_4_(source_call_id_4)
        , backtrace_(backtrace) {
    }

    void to_sexp(int position,
                 SEXP r_time,
                 SEXP r_env_id,
                 SEXP r_direct,
                 SEXP r_expression,
                 SEXP r_source_fun_id_1,
                 SEXP r_source_call_id_1,
                 SEXP r_source_fun_id_2,
                 SEXP r_source_call_id_2,
                 SEXP r_source_fun_id_3,
                 SEXP r_source_call_id_3,
                 SEXP r_source_fun_id_4,
                 SEXP r_source_call_id_4,
                 SEXP r_backtrace) {
        SET_INTEGER_ELT(r_time, position, time_);
        SET_INTEGER_ELT(r_env_id, position, env_id_);
        SET_LOGICAL_ELT(r_direct, position, direct_);
        SET_STRING_ELT(r_expression, position, make_char(expression_));
        SET_INTEGER_ELT(r_source_fun_id_1, position, source_fun_id_1_);
        SET_INTEGER_ELT(r_source_call_id_1, position, source_call_id_1_);
        SET_INTEGER_ELT(r_source_fun_id_2, position, source_fun_id_2_);
        SET_INTEGER_ELT(r_source_call_id_2, position, source_call_id_2_);
        SET_INTEGER_ELT(r_source_fun_id_3, position, source_fun_id_3_);
        SET_INTEGER_ELT(r_source_call_id_3, position, source_call_id_3_);
        SET_INTEGER_ELT(r_source_fun_id_4, position, source_fun_id_4_);
        SET_INTEGER_ELT(r_source_call_id_4, position, source_call_id_4_);
        SET_STRING_ELT(r_backtrace, position, make_char(backtrace_));
    }

  private:
    int time_;
    int env_id_;
    int direct_;
    const std::string expression_;
    int source_fun_id_1_;
    int source_call_id_1_;
    int source_fun_id_2_;
    int source_call_id_2_;
    int source_fun_id_3_;
    int source_call_id_3_;
    int source_fun_id_4_;
    int source_call_id_4_;
    const std::string backtrace_;
};

#endif /* ENVTRACER_EVAL_H */
