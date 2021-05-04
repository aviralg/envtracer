#ifndef ENVTRACER_ENVIRONMENT_ACCESS_H
#define ENVTRACER_ENVIRONMENT_ACCESS_H

#include <string>
#include "utilities.h"

class EnvironmentAccess {
  public:
    EnvironmentAccess(int call_id,
                      const std::string& fun_name,
                      const std::string& arg_type)
        : call_id_(call_id)
        , fun_name_(fun_name)
        , source_fun_id_(NA_INTEGER)
        , source_call_id_(NA_INTEGER)
        , arg_type_(arg_type)
        , n_(NA_INTEGER)
        , which_(NA_INTEGER)
        , x_int_(NA_INTEGER)
        , x_char_(ENVTRACER_NA_STRING) {
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

    void set_source(int source_fun_id, int source_call_id) {
        source_fun_id_ = source_fun_id;
        source_call_id_ = source_call_id;
    }

    void to_sexp(int position,
                 SEXP r_call_id,
                 SEXP r_fun_name,
                 SEXP r_source_fun_id,
                 SEXP r_source_call_id,
                 SEXP r_arg_type,
                 SEXP r_n,
                 SEXP r_which,
                 SEXP r_x_int,
                 SEXP r_x_char) {
        SET_INTEGER_ELT(r_call_id, position, call_id_);
        SET_STRING_ELT(r_fun_name, position, make_char(fun_name_));
        SET_INTEGER_ELT(r_source_fun_id, position, source_fun_id_);
        SET_INTEGER_ELT(r_source_call_id, position, source_call_id_);
        SET_STRING_ELT(r_arg_type, position, make_char(arg_type_));
        SET_INTEGER_ELT(r_n, position, n_);
        SET_INTEGER_ELT(r_which, position, which_);
        SET_INTEGER_ELT(r_x_int, position, x_int_);
        SET_STRING_ELT(r_x_char, position, make_char(x_char_));
    }

    static EnvironmentAccess* Which(int call_id,
                                    const std::string& fun_name,
                                    const std::string& arg_type,
                                    int which);

    static EnvironmentAccess* N(int call_id,
                                const std::string& fun_name,
                                const std::string& arg_type,
                                int n);

    static EnvironmentAccess* XInt(int call_id,
                                   const std::string& fun_name,
                                   const std::string& arg_type,
                                   int x_int);

    static EnvironmentAccess* XChar(int call_id,
                                    const std::string& fun_name,
                                    const std::string& arg_type,
                                    const std::string& x_char);

  private:
    int call_id_;
    std::string fun_name_;
    int source_fun_id_;
    int source_call_id_;
    std::string arg_type_;
    int n_;
    int which_;
    int x_int_;
    std::string x_char_;
};

#endif /* ENVTRACER_ENVIRONMENT_ACCESS_H */
