#ifndef ENVTRACER_ENVIRONMENT_ACCESS_H
#define ENVTRACER_ENVIRONMENT_ACCESS_H

#include <string>
#include "utilities.h"

class EnvironmentAccess {
  public:
    EnvironmentAccess(int time,
                      int source_fun_id,
                      int source_call_id,
                      const std::string& backtrace,
                      int depth,
                      int env_id,
                      int call_id,
                      const std::string& fun_name,
                      const std::string& arg_type)
        : time_(time)
        , source_fun_id_(source_fun_id)
        , source_call_id_(source_call_id)
        , backtrace_(backtrace)
        , depth_(depth)
        , env_id_(NA_INTEGER)
        , call_id_(call_id)
        , fun_name_(fun_name)
        , arg_type_(arg_type)
        , n_(NA_INTEGER)
        , which_(NA_INTEGER)
        , x_int_(NA_INTEGER)
        , x_char_(ENVTRACER_NA_STRING)
        , fun_id_(NA_INTEGER)
        , symbol_(ENVTRACER_NA_STRING)
        , bindings_(NA_LOGICAL) {
    }

    int get_call_id() const {
        return call_id_;
    }

    const std::string& get_function_name() const {
        return fun_name_;
    }

    int get_n() const {
        return n_;
    }

    void set_n(int n) {
        n_ = n;
    }

    int get_which() const {
        return which_;
    }

    void set_which(int which) {
        which_ = which;
    }

    int get_x_int() const {
        return x_int_;
    }

    void set_x_int(int x_int) {
        x_int_ = x_int;
    }

    const std::string& get_x_char() const {
        return x_char_;
    }

    void set_x_char(const std::string& x_char) {
        x_char_ = x_char;
    }

    void set_fun_id(int fun_id) {
        fun_id_ = fun_id;
    }

    void set_symbol(const std::string& symbol) {
        symbol_ = symbol;
    }

    void set_bindings(int bindings) {
        bindings_ = bindings;
    }

    void set_env_id(int env_id) {
        env_id_ = env_id;
    }

    void to_sexp(int position,
                 SEXP r_call_id,
                 SEXP r_depth,
                 SEXP r_fun_name,
                 SEXP r_source_fun_id,
                 SEXP r_source_call_id,
                 SEXP r_arg_type,
                 SEXP r_n,
                 SEXP r_which,
                 SEXP r_x_int,
                 SEXP r_x_char,
                 SEXP r_fun_id,
                 SEXP r_symbol,
                 SEXP r_bindings,
                 SEXP r_env_id) {
        SET_INTEGER_ELT(r_call_id, position, call_id_);
        SET_INTEGER_ELT(r_depth, position, depth_);
        SET_STRING_ELT(r_fun_name, position, make_char(fun_name_));
        SET_INTEGER_ELT(r_source_fun_id, position, source_fun_id_);
        SET_INTEGER_ELT(r_source_call_id, position, source_call_id_);
        SET_STRING_ELT(r_arg_type, position, make_char(arg_type_));
        SET_INTEGER_ELT(r_n, position, n_);
        SET_INTEGER_ELT(r_which, position, which_);
        SET_INTEGER_ELT(r_x_int, position, x_int_);
        SET_STRING_ELT(r_x_char, position, make_char(x_char_));
        SET_INTEGER_ELT(r_fun_id, position, fun_id_);
        SET_STRING_ELT(r_symbol, position, make_char(symbol_));
        SET_LOGICAL_ELT(r_bindings, position, bindings_);
        SET_INTEGER_ELT(r_env_id, position, env_id_);
    }

  private:
    int time_;
    int source_fun_id_;
    int source_call_id_;
    const std::string backtrace_;
    int depth_;
    int env_id_;
    int call_id_;
    std::string fun_name_;
    std::string arg_type_;
    int n_;
    int which_;
    int x_int_;
    std::string x_char_;
    int fun_id_;
    std::string symbol_;
    int bindings_;
};

#endif /* ENVTRACER_ENVIRONMENT_ACCESS_H */
