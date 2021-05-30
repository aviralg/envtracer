#ifndef ENVTRACER_ENVIRONMENT_H
#define ENVTRACER_ENVIRONMENT_H

#include <string>
#include "utilities.h"

class Environment {
  public:
    Environment(int env_id, bool hashed, int parent_env_id, int call_id)
        : env_id_(env_id)
        , hashed_(hashed)
        , parent_env_id_(parent_env_id)
        , env_type_(ENVTRACER_NA_STRING)
        , env_name_(ENVTRACER_NA_STRING)
        , call_id_(call_id)
        , evals_(0)
        , eval_counter_(0)
        , package_(ENVTRACER_NA_STRING)
        , constructor_(ENVTRACER_NA_STRING)
        , source_fun_id_1_(NA_INTEGER)
        , source_call_id_1_(NA_INTEGER)
        , source_fun_id_2_(NA_INTEGER)
        , source_call_id_2_(NA_INTEGER)
        , source_fun_id_3_(NA_INTEGER)
        , source_call_id_3_(NA_INTEGER)
        , source_fun_id_4_(NA_INTEGER)
        , source_call_id_4_(NA_INTEGER)
        , dispatch_(false)
        , backtrace_(ENVTRACER_NA_STRING)
        , event_seq_("|") {
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

    void push_eval() {
        ++evals_;
        ++eval_counter_;
    }

    void pop_eval() {
        --eval_counter_;
    }

    bool inside_eval() const {
        return eval_counter_ > 0;
    }

    void set_package(const std::string& package) {
        package_ = package;
    }

    bool is_package() const {
        return package_ != ENVTRACER_NA_STRING;
    }

    void set_source(const std::string& constructor,
                    int source_fun_id_1,
                    int source_call_id_1,
                    int source_fun_id_2,
                    int source_call_id_2,
                    int source_fun_id_3,
                    int source_call_id_3,
                    int source_fun_id_4,
                    int source_call_id_4) {
        constructor_ = constructor;
        source_fun_id_1_ = source_fun_id_1;
        source_call_id_1_ = source_call_id_1;
        source_fun_id_2_ = source_fun_id_2;
        source_call_id_2_ = source_call_id_2;
        source_fun_id_3_ = source_fun_id_3;
        source_call_id_3_ = source_call_id_3;
        source_fun_id_4_ = source_fun_id_4;
        source_call_id_4_ = source_call_id_4;
    }

    void set_backtrace(const std::string& backtrace) {
        backtrace_ = backtrace;
    }

    void set_dispatch() {
        dispatch_ = true;
    }

    void add_event(const std::string& event) {
        event_seq_.append(event);
        event_seq_.append("|");
    }

    void lookup() {
        event_seq_.push_back('L');
    }

    void assign() {
        event_seq_.push_back('A');
    }

    void define() {
        event_seq_.push_back('D');
    }

    void remove() {
        event_seq_.push_back('R');
    }

    void escape() {
        event_seq_.push_back('E');
    }

    void lock() {
        event_seq_.push_back('+');
    }

    void unlock() {
        event_seq_.push_back('-');
    }

    void set_parent(int parent_env_id) {
        parent_env_id_ = parent_env_id;
    }

    void to_sexp(int position,
                 SEXP r_env_id,
                 SEXP r_hashed,
                 SEXP r_parent_env_id,
                 SEXP r_env_type,
                 SEXP r_env_name,
                 SEXP r_call_id,
                 SEXP r_classes,
                 SEXP r_evals,
                 SEXP r_package,
                 SEXP r_constructor,
                 SEXP r_source_fun_id_1,
                 SEXP r_source_call_id_1,
                 SEXP r_source_fun_id_2,
                 SEXP r_source_call_id_2,
                 SEXP r_source_fun_id_3,
                 SEXP r_source_call_id_3,
                 SEXP r_source_fun_id_4,
                 SEXP r_source_call_id_4,
                 SEXP r_dispatch,
                 SEXP r_event_seq,
                 SEXP r_backtrace) {
        SET_INTEGER_ELT(r_env_id, position, env_id_);
        SET_LOGICAL_ELT(r_hashed, position, hashed_);
        SET_INTEGER_ELT(r_parent_env_id, position, parent_env_id_);
        SET_STRING_ELT(r_env_type, position, make_char(env_type_));
        SET_STRING_ELT(r_env_name, position, make_char(env_name_));
        SET_INTEGER_ELT(r_call_id, position, call_id_);
        SET_STRING_ELT(r_classes, position, make_char(classes_));
        SET_INTEGER_ELT(r_evals, position, evals_);
        SET_STRING_ELT(r_package, position, make_char(package_));
        SET_STRING_ELT(r_constructor, position, make_char(constructor_));
        SET_INTEGER_ELT(r_source_fun_id_1, position, source_fun_id_1_);
        SET_INTEGER_ELT(r_source_call_id_1, position, source_call_id_1_);
        SET_INTEGER_ELT(r_source_fun_id_2, position, source_fun_id_2_);
        SET_INTEGER_ELT(r_source_call_id_2, position, source_call_id_2_);
        SET_INTEGER_ELT(r_source_fun_id_3, position, source_fun_id_3_);
        SET_INTEGER_ELT(r_source_call_id_3, position, source_call_id_3_);
        SET_INTEGER_ELT(r_source_fun_id_4, position, source_fun_id_4_);
        SET_INTEGER_ELT(r_source_call_id_4, position, source_call_id_4_);
        SET_LOGICAL_ELT(r_dispatch, position, dispatch_ ? 1 : 0);
        SET_STRING_ELT(r_event_seq, position, make_char(event_seq_));
        SET_STRING_ELT(r_backtrace, position, make_char(backtrace_));
    }

  private:
    int env_id_;
    bool hashed_;
    int parent_env_id_;
    std::string env_type_;
    std::string env_name_;
    int call_id_;
    std::vector<std::string> classes_;
    int evals_;
    int eval_counter_;
    std::string package_;
    std::string constructor_;
    int source_fun_id_1_;
    int source_call_id_1_;
    int source_fun_id_2_;
    int source_call_id_2_;
    int source_fun_id_3_;
    int source_call_id_3_;
    int source_fun_id_4_;
    int source_call_id_4_;
    bool dispatch_;
    std::string event_seq_;
    std::string backtrace_;
};

#endif /* ENVTRACER_ENVIRONMENT_H */
