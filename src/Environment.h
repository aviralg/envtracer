#ifndef ENVTRACER_ENVIRONMENT_H
#define ENVTRACER_ENVIRONMENT_H

#include <string>
#include "utilities.h"

class Environment {
  public:
    Environment(int env_id, int call_id)
        : env_id_(env_id)
        , env_type_(ENVTRACER_NA_STRING)
        , env_name_(ENVTRACER_NA_STRING)
        , call_id_(call_id)
        , package_(ENVTRACER_NA_STRING)
        , constructor_(ENVTRACER_NA_STRING)
        , source_fun_id_(NA_INTEGER)
        , source_call_id_(NA_INTEGER) {
    }

    int get_id() {
        return env_id_;
    }

    bool has_name() const {
        return env_name_ != ENVTRACER_NA_STRING;
    }
    const std::string& get_name() const {
        return env_name_;
    }

    const std::string& get_type() const {
        return env_type_;
    }

    void set_name(const char* env_name) {
        env_name_ = charptr_to_string(env_name);
    }

    void set_type(const char* env_type) {
        env_type_ = charptr_to_string(env_type);
    }

    int get_call_id() const {
        return call_id_;
    }

    void set_call_id(int call_id) {
        call_id_ = call_id;
    }

    void update_class(const char* klass) {
        for (const std::string& c: classes_) {
            if (c == klass) {
                return;
            }
        }

        classes_.push_back(klass);
    }

    void add_eval(const char* eval) {
        for (const std::string& e: evals_) {
            if (e == eval) {
                return;
            }
        }

        evals_.push_back(eval);
    }

    void set_package(const std::string& package) {
        package_ = package;
    }

    bool is_package() const {
        return package_ != ENVTRACER_NA_STRING;
    }

    void set_source(const std::string& constructor,
                    int source_fun_id,
                    int source_call_id) {
        constructor_ = constructor;
        source_fun_id_ = source_fun_id;
        source_call_id_ = source_call_id;
    }

    void to_sexp(int position,
                 SEXP r_env_id,
                 SEXP r_env_type,
                 SEXP r_env_name,
                 SEXP r_call_id,
                 SEXP r_classes,
                 SEXP r_evals,
                 SEXP r_package,
                 SEXP r_constructor,
                 SEXP r_source_fun_id,
                 SEXP r_source_call_id) {
        SET_INTEGER_ELT(r_env_id, position, env_id_);
        SET_STRING_ELT(r_env_type, position, make_char(env_type_));
        SET_STRING_ELT(r_env_name, position, make_char(env_name_));
        SET_INTEGER_ELT(r_call_id, position, call_id_);
        SET_STRING_ELT(r_classes, position, make_char(classes_));
        SET_STRING_ELT(r_evals, position, make_char(evals_));
        SET_STRING_ELT(r_package, position, make_char(package_));
        SET_STRING_ELT(r_constructor, position, make_char(constructor_));
        SET_INTEGER_ELT(r_source_fun_id, position, source_fun_id_);
        SET_INTEGER_ELT(r_source_call_id, position, source_call_id_);
    }

  private:
    int env_id_;
    std::string env_type_;
    std::string env_name_;
    int call_id_;
    std::vector<std::string> classes_;
    std::vector<std::string> evals_;
    std::string package_;
    std::string constructor_;
    int source_fun_id_;
    int source_call_id_;
};

#endif /* ENVTRACER_ENVIRONMENT_H */
