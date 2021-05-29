#ifndef ENVTRACER_ENVIRONMENT_ACCESS_H
#define ENVTRACER_ENVIRONMENT_ACCESS_H

#include <string>
#include "utilities.h"

class EnvironmentAccess {
  public:
    EnvironmentAccess(int time, int depth, const std::string& fun_name)
        : time_(time)
        , depth_(depth)
        , fun_name_(fun_name)
        , result_env_type_(ENVTRACER_NA_STRING)
        , result_env_id_(NA_INTEGER)
        , arg_env_type_1_(ENVTRACER_NA_STRING)
        , arg_env_id_1_(NA_INTEGER)
        , arg_env_type_2_(ENVTRACER_NA_STRING)
        , arg_env_id_2_(NA_INTEGER)
        , env_name_(ENVTRACER_NA_STRING)
        , symbol_(ENVTRACER_NA_STRING)
        , bindings_(NA_LOGICAL)
        , fun_type_(ENVTRACER_NA_STRING)
        , fun_id_(NA_INTEGER)
        , n_type_(ENVTRACER_NA_STRING)
        , n_(NA_INTEGER)
        , which_type_(ENVTRACER_NA_STRING)
        , which_(NA_INTEGER)
        , x_type_(ENVTRACER_NA_STRING)
        , x_int_(NA_INTEGER)
        , x_char_(ENVTRACER_NA_STRING)
        , seq_env_id_(ENVTRACER_NA_STRING)
        , se_env_id_(NA_INTEGER)
        , se_val_type_(ENVTRACER_NA_STRING)
        , source_fun_id_1_(NA_INTEGER)
        , source_call_id_1_(NA_INTEGER)
        , source_fun_id_2_(NA_INTEGER)
        , source_call_id_2_(NA_INTEGER)
        , source_fun_id_3_(NA_INTEGER)
        , source_call_id_3_(NA_INTEGER)
        , source_fun_id_4_(NA_INTEGER)
        , source_call_id_4_(NA_INTEGER)
        , backtrace_(ENVTRACER_NA_STRING) {
    }

    void set_result_env(const std::string& result_env_type, int result_env_id) {
        result_env_type_ = result_env_type;
        result_env_id_ = result_env_id;
    }

    void set_arg_env_1(const std::string& arg_env_type_1, int arg_env_id_1) {
        arg_env_type_1_ = arg_env_type_1;
        arg_env_id_1_ = arg_env_id_1;
    }

    void set_arg_env_2(const std::string& arg_env_type_2, int arg_env_id_2) {
        arg_env_type_2_ = arg_env_type_2;
        arg_env_id_2_ = arg_env_id_2;
    }

    void set_env_name(const std::string& env_name) {
        env_name_ = env_name;
    }

    void set_symbol(const std::string& symbol) {
        symbol_ = symbol;
    }

    void set_bindings(int bindings) {
        bindings_ = bindings;
    }

    void set_fun(const std::string& fun_type, int fun_id) {
        fun_type_ = fun_type;
        fun_id_ = fun_id;
    }

    void set_n(const std::string& n_type, int n) {
        n_type_ = n_type;
        n_ = n;
    }

    void set_which(const std::string& which_type, int which) {
        which_type_ = which_type;
        which_ = which;
    }

    void
    set_x(const std::string& x_type, int x_int, const std::string& x_char) {
        x_type_ = x_type;
        x_int_ = x_int;
        x_char_ = x_char;
    }

    void set_seq_env_id(const std::string& seq_env_id) {
        seq_env_id_ = seq_env_id;
    }

    void set_side_effect(int se_env_id, const std::string& se_val_type) {
        se_env_id_ = se_env_id;
        se_val_type_ = se_val_type;
    }

    void set_source(int source_fun_id_1,
                    int source_call_id_1,
                    int source_fun_id_2,
                    int source_call_id_2,
                    int source_fun_id_3,
                    int source_call_id_3,
                    int source_fun_id_4,
                    int source_call_id_4) {
        source_fun_id_1_ = source_fun_id_1;
        source_call_id_1_ = source_call_id_1;
        source_fun_id_2_ = source_fun_id_2;
        source_call_id_2_ = source_call_id_2;
        source_fun_id_3_ = source_fun_id_3;
        source_call_id_3_ = source_call_id_3;
        source_fun_id_4 = source_fun_id_4;
        source_call_id_4 = source_call_id_4;
    }

    void set_backtrace(const std::string& backtrace) {
        backtrace_ = backtrace;
    }

    void to_sexp(int position,
                 SEXP r_time,
                 SEXP r_depth,
                 SEXP r_fun_name,
                 SEXP r_result_env_type,
                 SEXP r_result_env_id,
                 SEXP r_arg_env_type_1,
                 SEXP r_arg_env_id_1,
                 SEXP r_arg_env_type_2,
                 SEXP r_arg_env_id_2,
                 SEXP r_env_name,
                 SEXP r_symbol,
                 SEXP r_bindings,
                 SEXP r_fun_type,
                 SEXP r_fun_id,
                 SEXP r_n_type,
                 SEXP r_n,
                 SEXP r_which_type,
                 SEXP r_which,
                 SEXP r_x_type,
                 SEXP r_x_int,
                 SEXP r_x_char,
                 SEXP r_seq_env_id,
                 SEXP r_se_env_id,
                 SEXP r_se_val_type,
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
        SET_INTEGER_ELT(r_depth, position, depth_);
        SET_STRING_ELT(r_fun_name, position, make_char(fun_name_));

        SET_STRING_ELT(
            r_result_env_type, position, make_char(result_env_type_));
        SET_INTEGER_ELT(r_result_env_id, position, result_env_id_);

        SET_STRING_ELT(r_arg_env_type_1, position, make_char(arg_env_type_1_));
        SET_INTEGER_ELT(r_arg_env_id_1, position, arg_env_id_1_);

        SET_STRING_ELT(r_arg_env_type_2, position, make_char(arg_env_type_2_));
        SET_INTEGER_ELT(r_arg_env_id_2, position, arg_env_id_2_);

        SET_STRING_ELT(r_env_name, position, make_char(env_name_));

        SET_STRING_ELT(r_symbol, position, make_char(symbol_));

        SET_LOGICAL_ELT(r_bindings, position, bindings_);

        SET_STRING_ELT(r_fun_type, position, make_char(fun_type_));
        SET_INTEGER_ELT(r_fun_id, position, fun_id_);

        SET_STRING_ELT(r_n_type, position, make_char(n_type_));
        SET_INTEGER_ELT(r_n, position, n_);

        SET_STRING_ELT(r_which_type, position, make_char(which_type_));
        SET_INTEGER_ELT(r_which, position, which_);

        SET_STRING_ELT(r_x_type, position, make_char(x_type_));
        SET_INTEGER_ELT(r_x_int, position, x_int_);
        SET_STRING_ELT(r_x_char, position, make_char(x_char_));

        SET_STRING_ELT(r_seq_env_id, position, make_char(seq_env_id_));

        SET_INTEGER_ELT(r_se_env_id, position, se_env_id_);
        SET_STRING_ELT(r_se_val_type, position, make_char(se_val_type_));

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
    int depth_;
    std::string fun_name_;

    std::string result_env_type_;
    int result_env_id_;

    std::string arg_env_type_1_;
    int arg_env_id_1_;

    std::string arg_env_type_2_;
    int arg_env_id_2_;

    std::string env_name_;

    std::string symbol_;

    int bindings_;

    std::string fun_type_;
    int fun_id_;

    std::string n_type_;
    int n_;

    std::string which_type_;
    int which_;

    std::string x_type_;
    int x_int_;
    std::string x_char_;

    std::string seq_env_id_;

    int se_env_id_;
    std::string se_val_type_;

    int source_fun_id_1_;
    int source_call_id_1_;
    int source_fun_id_2_;
    int source_call_id_2_;
    int source_fun_id_3_;
    int source_call_id_3_;
    int source_fun_id_4_;
    int source_call_id_4_;

    std::string backtrace_;
};

#endif /* ENVTRACER_ENVIRONMENT_ACCESS_H */
