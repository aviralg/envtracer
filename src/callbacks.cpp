#include "tracer.h"
#include "callbacks.h"
#include "TracingState.h"
#include "utilities.h"
#include <instrumentr/instrumentr.h>
#include <vector>

int extract_logical(instrumentr_value_t value, int index = 0) {
    if (value == NULL) {
        return NA_LOGICAL;
    }

    else if (instrumentr_value_is_logical(value)) {
        return instrumentr_logical_get_element(
            instrumentr_value_as_logical(value), 0);
    }

    else if (instrumentr_value_is_real(value)) {
        return instrumentr_real_get_element(instrumentr_value_as_real(value),
                                            0);
    }

    else if (instrumentr_value_is_integer(value)) {
        return instrumentr_integer_get_element(
            instrumentr_value_as_integer(value), 0);
    }

    return NA_LOGICAL;
}

int extract_integer(instrumentr_value_t value, int index = 0) {
    if (value == NULL) {
        return NA_INTEGER;
    }

    else if (instrumentr_value_is_logical(value)) {
        return instrumentr_logical_get_element(
            instrumentr_value_as_logical(value), 0);
    }

    else if (instrumentr_value_is_real(value)) {
        return instrumentr_real_get_element(instrumentr_value_as_real(value),
                                            0);
    }

    else if (instrumentr_value_is_integer(value)) {
        return instrumentr_integer_get_element(
            instrumentr_value_as_integer(value), 0);
    }

    return NA_INTEGER;
}

instrumentr_value_t lookup_environment(instrumentr_state_t state,
                                       instrumentr_environment_t environment,
                                       const char* name,
                                       bool unwrap_promise = true) {
    instrumentr_symbol_t sym = instrumentr_state_get_symbol(state, name);

    instrumentr_value_t val = instrumentr_environment_lookup(environment, sym);

    while (instrumentr_value_is_promise(val)) {
        instrumentr_promise_t val_prom = instrumentr_value_as_promise(val);

        if (!instrumentr_promise_is_forced(val_prom)) {
            val = NULL;
            break;
        }

        val = instrumentr_promise_get_value(val_prom);
    }

    return val;
}

void analyze_package_environment(instrumentr_state_t state,
                                 EnvironmentTable& env_table,
                                 instrumentr_environment_t environment,
                                 const std::string& name) {
    Environment* env = env_table.insert(environment);

    if (env->is_package()) {
        return;
    }

    env->set_package(name);

    SEXP r_names = PROTECT(
        R_lsInternal(instrumentr_environment_get_sexp(environment), TRUE));

    for (int i = 0; i < Rf_length(r_names); ++i) {
        const char* binding_name = CHAR(STRING_ELT(r_names, i));
        instrumentr_symbol_t binding_sym =
            instrumentr_state_get_symbol(state, binding_name);
        instrumentr_value_t value =
            instrumentr_environment_lookup(environment, binding_sym);

        if (instrumentr_value_is_environment(value)) {
            const std::string new_name =
                name + "::" + std::string(binding_name);
            analyze_package_environment(state,
                                        env_table,
                                        instrumentr_value_as_environment(value),
                                        new_name);
        }
    }
    UNPROTECT(1);
}

void package_load_callback(instrumentr_tracer_t tracer,
                           instrumentr_callback_t callback,
                           instrumentr_state_t state,
                           instrumentr_application_t application,
                           instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    analyze_package_environment(state, env_table, environment, "namespace");
}

void package_attach_callback(instrumentr_tracer_t tracer,
                             instrumentr_callback_t callback,
                             instrumentr_state_t state,
                             instrumentr_application_t application,
                             instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    analyze_package_environment(state, env_table, environment, "package");
}

void mark_promises(int ref_call_id,
                   const std::string& ref_type,
                   ArgumentTable& arg_tab,
                   ArgumentReflectionTable& ref_tab,
                   instrumentr_call_stack_t call_stack,
                   Backtrace& backtrace) {
    bool transitive = false;

    int source_fun_id = NA_INTEGER;
    int source_call_id = NA_INTEGER;
    int source_arg_id = NA_INTEGER;
    int source_formal_pos = NA_INTEGER;

    for (int i = 1; i < instrumentr_call_stack_get_size(call_stack); ++i) {
        instrumentr_frame_t frame =
            instrumentr_call_stack_peek_frame(call_stack, i);

        if (!instrumentr_frame_is_promise(frame)) {
            continue;
        }

        instrumentr_promise_t promise = instrumentr_frame_as_promise(frame);

        if (instrumentr_promise_get_type(promise) !=
            INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
            continue;
        }

        /* at this point, we are inside an argument promise which does
         * reflective environment operation. */

        int promise_id = instrumentr_promise_get_id(promise);

        const std::vector<Argument*>& args = arg_tab.lookup(promise_id);

        for (auto& arg: args) {
            arg->reflection(ref_type, transitive);

            int arg_id = arg->get_id();
            int fun_id = arg->get_fun_id();
            int call_id = arg->get_call_id();
            int formal_pos = arg->get_formal_pos();
            ref_tab.insert(ref_call_id,
                           ref_type,
                           transitive,
                           source_fun_id,
                           source_call_id,
                           source_arg_id,
                           source_formal_pos,
                           fun_id,
                           call_id,
                           arg_id,
                           formal_pos,
                           backtrace.to_string());

            if (!transitive) {
                source_fun_id = arg->get_fun_id();
                source_call_id = arg->get_call_id();
                source_arg_id = promise_id;
                source_formal_pos = arg->get_formal_pos();
            }
        }

        transitive = true;
    }
}

void mark_escaped_environment_call(int ref_call_id,
                                   const std::string& ref_type,
                                   ArgumentTable& arg_tab,
                                   CallTable& call_tab,
                                   CallReflectionTable& call_ref_tab,
                                   instrumentr_call_stack_t call_stack,
                                   instrumentr_environment_t environment) {
    int depth = 1;
    int source_fun_id = NA_INTEGER;
    int source_call_id = NA_INTEGER;
    int sink_fun_id = NA_INTEGER;
    int sink_call_id = NA_INTEGER;
    int sink_arg_id = NA_INTEGER;
    int sink_formal_pos = NA_INTEGER;

    for (int i = 0; i < instrumentr_call_stack_get_size(call_stack); ++i) {
        instrumentr_frame_t frame =
            instrumentr_call_stack_peek_frame(call_stack, i);

        if (sink_fun_id == NA_INTEGER && instrumentr_frame_is_promise(frame)) {
            instrumentr_promise_t promise = instrumentr_frame_as_promise(frame);

            if (instrumentr_promise_get_type(promise) !=
                INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
                continue;
            }

            int promise_id = instrumentr_promise_get_id(promise);

            /* the last argument refers to the topmost call id */
            Argument* arg = arg_tab.lookup(promise_id).back();

            sink_arg_id = arg->get_id();
            sink_fun_id = arg->get_fun_id();
            sink_call_id = arg->get_call_id();
            sink_formal_pos = arg->get_formal_pos();
        }

        if (instrumentr_frame_is_call(frame)) {
            instrumentr_call_t call = instrumentr_frame_as_call(frame);

            instrumentr_value_t function = instrumentr_call_get_function(call);

            if (!instrumentr_value_is_closure(function)) {
                continue;
            }

            instrumentr_closure_t closure =
                instrumentr_value_as_closure(function);

            if (sink_fun_id == NA_INTEGER) {
                sink_call_id = instrumentr_call_get_id(call);
                sink_fun_id = instrumentr_closure_get_id(closure);
            }

            instrumentr_environment_t call_env =
                instrumentr_call_get_environment(call);

            if (environment == NULL || call_env == environment) {
                source_call_id = instrumentr_call_get_id(call);
                source_fun_id = instrumentr_closure_get_id(closure);

                call_ref_tab.insert(ref_call_id,
                                    ref_type,
                                    source_fun_id,
                                    source_call_id,
                                    sink_fun_id,
                                    sink_call_id,
                                    sink_arg_id,
                                    sink_formal_pos,
                                    depth);

                Call* call_data = call_tab.lookup(source_call_id);
                call_data->esc_env();

                return;
            }

            ++depth;
        }
    }
}

bool has_minus_one_argument(instrumentr_call_t call) {
    instrumentr_value_t arguments = instrumentr_call_get_arguments(call);
    SEXP r_arguments = instrumentr_value_get_sexp(arguments);
    SEXP r_argument = CAR(r_arguments);
    bool valid = false;

    if (TYPEOF(r_argument) == REALSXP) {
        for (int i = 0; i < Rf_length(r_argument); ++i) {
            double pos = REAL_ELT(r_argument, i);
            if (pos == -1) {
                valid = true;
                break;
            }
        }
    }

    else if (TYPEOF(r_argument) == INTSXP) {
        for (int i = 0; i < Rf_length(r_argument); ++i) {
            int pos = INTEGER_ELT(r_argument, i);
            if (pos == -1) {
                valid = true;
                break;
            }
        }
    }

    return valid;
}

instrumentr_call_t get_caller(instrumentr_call_stack_t call_stack, int& index) {
    int env_id = NA_INTEGER;

    for (; index < instrumentr_call_stack_get_size(call_stack); ++index) {
        instrumentr_frame_t frame =
            instrumentr_call_stack_peek_frame(call_stack, index);

        if (instrumentr_frame_is_promise(frame) && env_id == NA_INTEGER) {
            instrumentr_promise_t promise = instrumentr_frame_as_promise(frame);
            instrumentr_value_t env =
                instrumentr_promise_get_environment(promise);

            if (instrumentr_value_is_environment(env)) {
                env_id = instrumentr_value_get_id(env);
            }
        }

        if (instrumentr_frame_is_call(frame)) {
            instrumentr_call_t call = instrumentr_frame_as_call(frame);

            instrumentr_value_t fun = instrumentr_call_get_function(call);

            if (!instrumentr_value_is_closure(fun)) {
                continue;
            }

            instrumentr_closure_t closure = instrumentr_value_as_closure(fun);

            instrumentr_environment_t env =
                instrumentr_call_get_environment(call);

            int cur_env_id = instrumentr_environment_get_id(env);

            if (env_id == NA_INTEGER || cur_env_id == env_id) {
                return call;
            }
        }
    }

    return nullptr;
}

instrumentr_call_t get_caller_info(instrumentr_call_stack_t call_stack,
                                   int& fun_id,
                                   int& call_id,
                                   std::string& fun_name,
                                   int& index) {
    instrumentr_call_t call = get_caller(call_stack, index);

    fun_id = NA_INTEGER;
    call_id = NA_INTEGER;
    fun_name = ENVTRACER_NA_STRING;

    if (call != nullptr) {
        instrumentr_closure_t closure =
            instrumentr_value_as_closure(instrumentr_call_get_function(call));
        call_id = instrumentr_call_get_id(call);
        fun_id = instrumentr_closure_get_id(closure);
        fun_name = charptr_to_string(instrumentr_closure_get_name(closure));
    }

    return call;
}

void get_four_caller_info(instrumentr_call_stack_t call_stack,
                          int& source_fun_id_1,
                          int& source_call_id_1,
                          int& source_fun_id_2,
                          int& source_call_id_2,
                          int& source_fun_id_3,
                          int& source_call_id_3,
                          int& source_fun_id_4,
                          int& source_call_id_4,
                          int& index) {
    std::string fun_name;

    get_caller_info(
        call_stack, source_fun_id_1, source_call_id_1, fun_name, index);
    ++index;
    get_caller_info(
        call_stack, source_fun_id_2, source_call_id_2, fun_name, index);
    ++index;
    get_caller_info(
        call_stack, source_fun_id_3, source_call_id_3, fun_name, index);
    ++index;
    get_caller_info(
        call_stack, source_fun_id_4, source_call_id_4, fun_name, index);
}

int get_environment_depth(instrumentr_call_stack_t call_stack,
                          instrumentr_id_t environment_id,
                          int index = 0) {
    int depth = 0;

    for (int i = index; i < instrumentr_call_stack_get_size(call_stack); ++i) {
        instrumentr_frame_t frame =
            instrumentr_call_stack_peek_frame(call_stack, i);

        if (instrumentr_frame_is_call(frame)) {
            instrumentr_call_t call = instrumentr_frame_as_call(frame);

            instrumentr_value_t fun = instrumentr_call_get_function(call);

            if (!instrumentr_value_is_closure(fun)) {
                continue;
            }

            ++depth;

            instrumentr_environment_t env =
                instrumentr_call_get_environment(call);

            if (instrumentr_environment_get_id(env) == environment_id) {
                return depth;
            }
        }
    }

    return NA_INTEGER;
}

void handle_builtin_environment_access(instrumentr_state_t state,
                                       instrumentr_call_stack_t call_stack,
                                       instrumentr_call_t call,
                                       instrumentr_builtin_t builtin,
                                       Backtrace& backtrace,
                                       EnvironmentAccessTable& env_access_table,
                                       EnvironmentTable& env_table,
                                       FunctionTable& function_table) {
    bool record = false;

    std::string fun_name =
        charptr_to_string(instrumentr_builtin_get_name(builtin));

    int result_env_id = NA_INTEGER;

    std::string result_env_type = ENVTRACER_NA_STRING;

    instrumentr_value_t result = nullptr;

    instrumentr_environment_t environment = nullptr;

    if (instrumentr_call_has_result(call)) {
        result = instrumentr_call_get_result(call);
        result_env_type = get_sexp_type(instrumentr_value_get_sexp(result));

        if (instrumentr_value_is_environment(result)) {
            environment = instrumentr_value_as_environment(result);
            result_env_id = instrumentr_value_get_id(result);
        }
    }

    instrumentr_value_t arg_val = instrumentr_call_get_arguments(call);
    SEXP r_arguments = instrumentr_value_get_sexp(arg_val);

    instrumentr_environment_t arg_environment_1 = nullptr;
    std::string arg_env_type_1 = ENVTRACER_NA_STRING;
    int arg_env_id_1 = NA_INTEGER;

    instrumentr_environment_t arg_environment_2 = nullptr;
    std::string arg_env_type_2 = ENVTRACER_NA_STRING;
    int arg_env_id_2 = NA_INTEGER;

    std::string env_name = ENVTRACER_NA_STRING;

    std::string symbol = ENVTRACER_NA_STRING;

    int bindings = NA_LOGICAL;

    std::string fun_type = ENVTRACER_NA_STRING;
    int fun_id = NA_INTEGER;

    std::string n_type = ENVTRACER_NA_STRING;
    int n = NA_INTEGER;

    std::string which_type = ENVTRACER_NA_STRING;
    int which = NA_INTEGER;

    std::string x_type = ENVTRACER_NA_STRING;
    int x_int = NA_INTEGER;
    std::string x_char = ENVTRACER_NA_STRING;

    std::string seq_env_id = ENVTRACER_NA_STRING;

    if (fun_name == "length" || fun_name == "env2list") {
        instrumentr_pairlist_t arguments =
            instrumentr_value_as_pairlist(arg_val);

        instrumentr_value_t arg_env_val_1 =
            instrumentr_pairlist_get_element(arguments, 0);

        if (instrumentr_value_is_environment(arg_env_val_1)) {
            record = true;
            arg_environment_1 = instrumentr_value_as_environment(arg_env_val_1);
            arg_env_id_1 = instrumentr_value_get_id(arg_env_val_1);
            arg_env_type_1 =
                get_sexp_type(instrumentr_value_get_sexp(arg_env_val_1));
        }
    }

    else if (fun_name == "list2env") {
        instrumentr_pairlist_t arguments =
            instrumentr_value_as_pairlist(arg_val);

        instrumentr_value_t arg_env_val_1 =
            instrumentr_pairlist_get_element(arguments, 1);

        if (instrumentr_value_is_environment(arg_env_val_1)) {
            record = true;
            arg_environment_1 = instrumentr_value_as_environment(arg_env_val_1);
            arg_env_id_1 = instrumentr_value_get_id(arg_env_val_1);
            arg_env_type_1 =
                get_sexp_type(instrumentr_value_get_sexp(arg_env_val_1));
        }
    }

    else if (fun_name == "as.environment" || fun_name == "pos.to.env") {
        record = true;

        SEXP r_x = CAR(r_arguments);

        x_type = get_sexp_type(r_x);

        if (x_type == "integer") {
            x_int = INTEGER_ELT(r_x, 0);
        } else if (x_type == "double") {
            x_int = REAL_ELT(r_x, 0);
        } else if (x_type == "character") {
            x_char = CHAR(STRING_ELT(r_x, 0));
        }
    }

    else if (fun_name == "sys.call" || fun_name == "sys.frame" ||
             fun_name == "sys.function") {
        record = true;

        SEXP r_which = CAR(r_arguments);

        which_type = get_sexp_type(r_which);

        which = NA_INTEGER;

        if (TYPEOF(r_which) == REALSXP) {
            which = REAL_ELT(r_which, 0);
        }

        else if (TYPEOF(r_which) == INTSXP) {
            which = INTEGER_ELT(r_which, 0);
        }

        else {
            Rf_error("incorrect type of which: %s", which_type.c_str());
        }
    }

    else if (fun_name == "sys.parent" || fun_name == "parent.frame") {
        record = true;

        SEXP r_n = CAR(r_arguments);

        n_type = get_sexp_type(r_n);

        n = NA_INTEGER;

        if (TYPEOF(r_n) == REALSXP) {
            n = REAL_ELT(r_n, 0);
        }

        else if (TYPEOF(r_n) == INTSXP) {
            n = INTEGER_ELT(r_n, 0);
        }

        else {
            Rf_error("incorrect type of n: %s", get_sexp_type(r_n).c_str());
        }
    }

    else if (fun_name == "sys.calls" || fun_name == "sys.parents" ||
             fun_name == "sys.on.exit" || fun_name == "sys.status" ||
             fun_name == "sys.nframe") {
        record = true;
    }

    else if (fun_name == "sys.frames" && result != nullptr) {
        record = true;

        if (instrumentr_value_is_pairlist(result)) {
            instrumentr_pairlist_t pairlist =
                instrumentr_value_as_pairlist(result);

            seq_env_id = "|";

            for (int i = 0; i < instrumentr_pairlist_get_length(pairlist);
                 ++i) {
                instrumentr_value_t elt =
                    instrumentr_pairlist_get_element(pairlist, i);

                if (instrumentr_value_is_environment(elt)) {
                    seq_env_id.append(
                        std::to_string(instrumentr_value_get_id(elt)));
                    seq_env_id.append("|");

                    Environment* env =
                        env_table.insert(instrumentr_value_as_environment(elt));

                    env->add_event(fun_name + "_0");
                }
            }

            if (seq_env_id.size() == 1) {
                seq_env_id.push_back('|');
            }
        }

    }

    else if (fun_name == "environment") {
        record = true;

        instrumentr_pairlist_t arguments =
            instrumentr_value_as_pairlist(arg_val);

        instrumentr_value_t fun_val =
            instrumentr_pairlist_get_element(arguments, 0);

        fun_id = instrumentr_value_is_closure(fun_val)
                     ? instrumentr_value_get_id(fun_val)
                     : NA_INTEGER;

        fun_type = get_sexp_type(instrumentr_value_get_sexp(fun_val));
    }

    else if (fun_name == "environment<-") {
        record = true;

        instrumentr_pairlist_t arguments =
            instrumentr_value_as_pairlist(arg_val);

        instrumentr_value_t fun_val =
            instrumentr_pairlist_get_element(arguments, 0);

        fun_id = instrumentr_value_is_closure(fun_val)
                     ? instrumentr_value_get_id(fun_val)
                     : NA_INTEGER;

        fun_type = get_sexp_type(instrumentr_value_get_sexp(fun_val));

        instrumentr_value_t arg_env_val_1 =
            instrumentr_pairlist_get_element(arguments, 1);

        arg_env_type_1 =
            get_sexp_type(instrumentr_value_get_sexp(arg_env_val_1));

        if (instrumentr_value_is_environment(arg_env_val_1)) {
            arg_environment_1 = instrumentr_value_as_environment(arg_env_val_1);
            arg_env_id_1 = instrumentr_value_get_id(arg_env_val_1);
        }
    }

    else if (fun_name == "lockEnvironment") {
        record = true;

        instrumentr_pairlist_t arguments =
            instrumentr_value_as_pairlist(arg_val);

        instrumentr_value_t arg_env_val_1 =
            instrumentr_pairlist_get_element(arguments, 0);

        arg_env_type_1 =
            get_sexp_type(instrumentr_value_get_sexp(arg_env_val_1));

        if (instrumentr_value_is_environment(arg_env_val_1)) {
            arg_environment_1 = instrumentr_value_as_environment(arg_env_val_1);
            arg_env_id_1 = instrumentr_value_get_id(arg_env_val_1);
        }

        instrumentr_value_t bindings_val =
            instrumentr_pairlist_get_element(arguments, 1);

        if (instrumentr_value_is_logical(bindings_val)) {
            bindings = LOGICAL_ELT(instrumentr_value_get_sexp(bindings_val), 0);
        }
    }

    else if (fun_name == "lockBinding" || fun_name == "unlockBinding") {
        record = true;

        instrumentr_pairlist_t arguments =
            instrumentr_value_as_pairlist(arg_val);

        instrumentr_value_t sym_val =
            instrumentr_pairlist_get_element(arguments, 0);

        if (instrumentr_value_is_symbol(sym_val)) {
            symbol = CHAR(PRINTNAME(instrumentr_value_get_sexp(sym_val)));
        }

        instrumentr_value_t arg_env_val_1 =
            instrumentr_pairlist_get_element(arguments, 1);

        arg_env_type_1 =
            get_sexp_type(instrumentr_value_get_sexp(arg_env_val_1));

        if (instrumentr_value_is_environment(arg_env_val_1)) {
            arg_environment_1 = instrumentr_value_as_environment(arg_env_val_1);
            arg_env_id_1 = instrumentr_value_get_id(arg_env_val_1);
        }
    }

    else if (fun_name == "environmentName") {
        record = true;

        instrumentr_pairlist_t arguments =
            instrumentr_value_as_pairlist(arg_val);

        instrumentr_value_t arg_env_val_1 =
            instrumentr_pairlist_get_element(arguments, 0);

        arg_env_type_1 =
            get_sexp_type(instrumentr_value_get_sexp(arg_env_val_1));

        if (instrumentr_value_is_environment(arg_env_val_1)) {
            arg_environment_1 = instrumentr_value_as_environment(arg_env_val_1);
            arg_env_id_1 = instrumentr_value_get_id(arg_env_val_1);
        }

        if (result != nullptr && instrumentr_value_is_character(result)) {
            env_name = CHAR(STRING_ELT(instrumentr_value_get_sexp(result), 0));
        }
    }

    else if (fun_name == "parent.env<-") {
        record = true;

        instrumentr_pairlist_t arguments =
            instrumentr_value_as_pairlist(arg_val);

        instrumentr_value_t arg_env_val_1 =
            instrumentr_pairlist_get_element(arguments, 0);

        arg_env_type_1 =
            get_sexp_type(instrumentr_value_get_sexp(arg_env_val_1));

        if (instrumentr_value_is_s4(arg_env_val_1)) {
            arg_env_val_1 = instrumentr_s4_get_data_slot(
                instrumentr_value_as_s4(result), ENVSXP);
        }

        if (instrumentr_value_is_environment(arg_env_val_1)) {
            arg_environment_1 = instrumentr_value_as_environment(arg_env_val_1);
            arg_env_id_1 = instrumentr_value_get_id(arg_env_val_1);
        }

        instrumentr_value_t arg_env_val_2 =
            instrumentr_pairlist_get_element(arguments, 1);

        arg_env_type_2 =
            get_sexp_type(instrumentr_value_get_sexp(arg_env_val_2));

        if (instrumentr_value_is_s4(arg_env_val_2)) {
            arg_env_val_2 = instrumentr_s4_get_data_slot(
                instrumentr_value_as_s4(result), ENVSXP);
        }

        if (instrumentr_value_is_environment(arg_env_val_2)) {
            arg_environment_2 = instrumentr_value_as_environment(arg_env_val_2);
            arg_env_id_2 = instrumentr_value_get_id(arg_env_val_2);
        }
    }

    else if (fun_name == "parent.env") {
        record = true;

        instrumentr_pairlist_t arguments =
            instrumentr_value_as_pairlist(arg_val);

        instrumentr_value_t arg_env_val_1 =
            instrumentr_pairlist_get_element(arguments, 0);

        arg_env_type_1 =
            get_sexp_type(instrumentr_value_get_sexp(arg_env_val_1));

        if (instrumentr_value_is_s4(arg_env_val_1)) {
            arg_env_val_1 = instrumentr_s4_get_data_slot(
                instrumentr_value_as_s4(result), ENVSXP);
        }

        if (instrumentr_value_is_environment(arg_env_val_1)) {
            arg_environment_1 = instrumentr_value_as_environment(arg_env_val_1);
            arg_env_id_1 = instrumentr_value_get_id(arg_env_val_1);
        }
    }

    else if (fun_name == "emptyenv" || fun_name == "baseenv" ||
             fun_name == "globalenv") {
        record = true;
    }

    if (record) {
        int time = instrumentr_state_get_time(state);

        int depth = dyntrace_get_frame_depth();

        EnvironmentAccess* env_access =
            new EnvironmentAccess(time, depth, fun_name);

        env_access->set_result_env(result_env_type, result_env_id);

        if (environment != nullptr) {
            Environment* env = env_table.insert(environment);

            std::string event_name = fun_name + "_0";

            int model_depth = get_environment_depth(call_stack, result_env_id);

            if (model_depth != NA_INTEGER) {
                // subtract 1 to account for the reflective operation closure
                event_name.append("^").append(std::to_string(model_depth - 1));
            }

            env->add_event(event_name);
        }

        env_access->set_arg_env_1(arg_env_type_1, arg_env_id_1);

        if (arg_environment_1 != nullptr) {
            Environment* env = env_table.insert(arg_environment_1);

            env->add_event(fun_name + "_1");
        }

        env_access->set_arg_env_2(arg_env_type_2, arg_env_id_2);

        if (arg_environment_2 != nullptr) {
            Environment* env = env_table.insert(arg_environment_2);

            env->add_event(fun_name + "_2");
        }

        env_access->set_env_name(env_name);

        env_access->set_symbol(symbol);

        env_access->set_bindings(bindings);

        env_access->set_fun(fun_type, fun_id);

        env_access->set_n(n_type, n);

        env_access->set_which(which_type, which);

        env_access->set_x(x_type, x_int, x_char);

        env_access->set_seq_env_id(seq_env_id);

        int index = 1;

        int source_fun_id_1 = NA_INTEGER;
        int source_call_id_1 = NA_INTEGER;
        int source_fun_id_2 = NA_INTEGER;
        int source_call_id_2 = NA_INTEGER;
        int source_fun_id_3 = NA_INTEGER;
        int source_call_id_3 = NA_INTEGER;
        int source_fun_id_4 = NA_INTEGER;
        int source_call_id_4 = NA_INTEGER;

        get_four_caller_info(call_stack,
                             source_fun_id_1,
                             source_call_id_1,
                             source_fun_id_2,
                             source_call_id_2,
                             source_fun_id_3,
                             source_call_id_3,
                             source_fun_id_4,
                             source_call_id_4,
                             index);

        env_access->set_source(source_fun_id_1,
                               source_call_id_1,
                               source_fun_id_2,
                               source_call_id_2,
                               source_fun_id_3,
                               source_call_id_3,
                               source_fun_id_4,
                               source_call_id_4);

        env_access->set_backtrace(backtrace.to_string());

        env_access_table.insert(env_access);
    }
}

void handle_builtin_environment_construction(
    instrumentr_state_t state,
    instrumentr_call_stack_t call_stack,
    instrumentr_call_t call,
    instrumentr_builtin_t builtin,
    Backtrace& backtrace,
    EnvironmentConstructorTable& env_constructor_table,
    EnvironmentTable& env_table) {
    std::string fun_name =
        charptr_to_string(instrumentr_builtin_get_name(builtin));

    if (fun_name != "new.env") {
        return;
    }

    if (!instrumentr_call_has_result(call)) {
        return;
    }

    instrumentr_value_t result = instrumentr_call_get_result(call);

    instrumentr_environment_t result_env =
        instrumentr_value_as_environment(result);

    int result_env_id = instrumentr_environment_get_id(result_env);

    int index = 1;

    int source_fun_id_1 = NA_INTEGER;
    int source_call_id_1 = NA_INTEGER;
    int source_fun_id_2 = NA_INTEGER;
    int source_call_id_2 = NA_INTEGER;
    int source_fun_id_3 = NA_INTEGER;
    int source_call_id_3 = NA_INTEGER;
    int source_fun_id_4 = NA_INTEGER;
    int source_call_id_4 = NA_INTEGER;

    get_four_caller_info(call_stack,
                         source_fun_id_1,
                         source_call_id_1,
                         source_fun_id_2,
                         source_call_id_2,
                         source_fun_id_3,
                         source_call_id_3,
                         source_fun_id_4,
                         source_call_id_4,
                         index);

    instrumentr_value_t arg_val = instrumentr_call_get_arguments(call);
    instrumentr_pairlist_t arguments = instrumentr_value_as_pairlist(arg_val);

    instrumentr_value_t hash_val =
        instrumentr_pairlist_get_element(arguments, 0);
    int hash = extract_logical(hash_val);

    instrumentr_value_t parent_val =
        instrumentr_pairlist_get_element(arguments, 1);
    int parent_env_id = NA_INTEGER;
    int parent_env_depth = NA_INTEGER;
    instrumentr_environment_t parent_env = nullptr;
    std::string parent_type = ENVTRACER_NA_STRING;

    if (parent_val != NULL && instrumentr_value_is_environment(parent_val)) {
        parent_env = instrumentr_value_as_environment(parent_val);
        parent_env_id = instrumentr_value_get_id(parent_val);
        parent_env_depth = get_environment_depth(call_stack, parent_env_id, 0);

        Environment* env = env_table.insert(parent_env);
        env->add_event("new.env_1");

        SEXP parent_env_sexp = instrumentr_environment_get_sexp(parent_env);

        if (parent_env_sexp == R_GlobalEnv) {
            parent_type = "global";
        } else if (parent_env_sexp == R_EmptyEnv) {
            parent_type = "empty";
        } else if (parent_env_sexp == R_BaseEnv) {
            parent_type = "base";
        } else if (parent_env_sexp == R_BaseNamespace) {
            parent_type = "base";
        } else {
            parent_type =
                charptr_to_string(instrumentr_environment_get_name(parent_env));
        }
    }

    instrumentr_value_t size_val =
        instrumentr_pairlist_get_element(arguments, 2);
    int size = extract_integer(size_val);

    int frame_count = instrumentr_environment_get_frame_count(result_env);

    EnvironmentConstructor* cons =
        new EnvironmentConstructor(result_env_id,
                                   source_fun_id_1,
                                   source_call_id_1,
                                   source_fun_id_2,
                                   source_call_id_2,
                                   source_fun_id_3,
                                   source_call_id_3,
                                   source_fun_id_4,
                                   source_call_id_4,
                                   hash,
                                   parent_env_id,
                                   parent_env_depth,
                                   size,
                                   frame_count,
                                   parent_type,
                                   backtrace.to_string());

    env_constructor_table.insert(cons);

    Environment* env = env_table.insert(result_env);
    env->add_event("new.env_0");
}

void builtin_call_entry_callback(instrumentr_tracer_t tracer,
                                 instrumentr_callback_t callback,
                                 instrumentr_state_t state,
                                 instrumentr_application_t application,
                                 instrumentr_builtin_t builtin,
                                 instrumentr_call_t call) {
    TracingState& tracing_state = TracingState::lookup(state);

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    /* handle backtrace */
    Backtrace& backtrace = tracing_state.get_backtrace();

    backtrace.push(call);
}

void builtin_call_exit_callback(instrumentr_tracer_t tracer,
                                instrumentr_callback_t callback,
                                instrumentr_state_t state,
                                instrumentr_application_t application,
                                instrumentr_builtin_t builtin,
                                instrumentr_call_t call) {
    TracingState& tracing_state = TracingState::lookup(state);

    EnvironmentTable& env_table = tracing_state.get_environment_table();

    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();

    FunctionTable& function_table = tracing_state.get_function_table();

    EnvironmentConstructorTable& env_constructor_table =
        tracing_state.get_environment_constructor_table();

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    /* handle backtrace */
    Backtrace& backtrace = tracing_state.get_backtrace();

    handle_builtin_environment_access(state,
                                      call_stack,
                                      call,
                                      builtin,
                                      backtrace,
                                      env_access_table,
                                      env_table,
                                      function_table);

    handle_builtin_environment_construction(state,
                                            call_stack,
                                            call,
                                            builtin,
                                            backtrace,
                                            env_constructor_table,
                                            env_table);

    backtrace.pop();
}

void special_call_exit_callback(instrumentr_tracer_t tracer,
                                instrumentr_callback_t callback,
                                instrumentr_state_t state,
                                instrumentr_application_t application,
                                instrumentr_special_t special,
                                instrumentr_call_t call) {
    std::string name = instrumentr_special_get_name(special);

    if (name != "~") {
        return;
    }

    if (!instrumentr_call_has_result(call)) {
        return;
    }

    instrumentr_value_t formula = instrumentr_call_get_result(call);

    instrumentr_value_t env_val =
        instrumentr_value_get_attribute(formula, ".Environment");

    if (!instrumentr_value_is_environment(env_val)) {
        return;
    }

    int time = instrumentr_state_get_time(state);

    int depth = NA_INTEGER;

    instrumentr_environment_t environment =
        instrumentr_value_as_environment(env_val);

    TracingState& tracing_state = TracingState::lookup(state);

    EnvironmentTable& env_table = tracing_state.get_environment_table();

    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    Backtrace& backtrace = tracing_state.get_backtrace();

    int source_fun_id_1 = NA_INTEGER;
    int source_call_id_1 = NA_INTEGER;
    int source_fun_id_2 = NA_INTEGER;
    int source_call_id_2 = NA_INTEGER;
    int source_fun_id_3 = NA_INTEGER;
    int source_call_id_3 = NA_INTEGER;
    int source_fun_id_4 = NA_INTEGER;
    int source_call_id_4 = NA_INTEGER;
    int frame_index = 1;

    get_four_caller_info(call_stack,
                         source_fun_id_1,
                         source_call_id_1,
                         source_fun_id_2,
                         source_call_id_2,
                         source_fun_id_3,
                         source_call_id_3,
                         source_fun_id_4,
                         source_call_id_4,
                         frame_index);

    Environment* env = env_table.insert(environment);
    env->add_event("~");

    EnvironmentAccess* env_access = new EnvironmentAccess(time, depth, "~");

    env_access->set_result_env("environment", env->get_id());

    env_access->set_source(source_fun_id_1,
                           source_call_id_1,
                           source_fun_id_2,
                           source_call_id_2,
                           source_fun_id_3,
                           source_call_id_3,
                           source_fun_id_4,
                           source_call_id_4);

    env_access->set_backtrace(backtrace.to_string());

    env_access_table.insert(env_access);

    /* handle backtrace */
}

void process_arguments(ArgumentTable& argument_table,
                       instrumentr_call_t call,
                       instrumentr_closure_t closure,
                       Call* call_data,
                       Function* function_data,
                       Environment* environment_data) {
    instrumentr_environment_t call_env = instrumentr_call_get_environment(call);

    instrumentr_value_t formals = instrumentr_closure_get_formals(closure);

    int position = 0;

    while (instrumentr_value_is_pairlist(formals)) {
        instrumentr_pairlist_t pairlist =
            instrumentr_value_as_pairlist(formals);

        instrumentr_value_t tagval = instrumentr_pairlist_get_tag(pairlist);

        instrumentr_symbol_t nameval = instrumentr_value_as_symbol(tagval);

        const char* argument_name = instrumentr_char_get_element(
            instrumentr_symbol_get_element(nameval));

        instrumentr_value_t argval =
            instrumentr_environment_lookup(call_env, nameval);

        argument_table.insert(argval,
                              position,
                              argument_name,
                              call_data,
                              function_data,
                              environment_data);

        ++position;
        formals = instrumentr_pairlist_get_cdr(pairlist);
    }
}

void process_actuals(ArgumentTable& argument_table, instrumentr_call_t call) {
    int call_id = instrumentr_call_get_id(call);

    instrumentr_value_t arguments = instrumentr_call_get_arguments(call);

    int index = 0;

    while (!instrumentr_value_is_null(arguments)) {
        instrumentr_pairlist_t pairlist =
            instrumentr_value_as_pairlist(arguments);

        instrumentr_value_t value = instrumentr_pairlist_get_car(pairlist);

        if (instrumentr_value_is_promise(value)) {
            instrumentr_promise_t promise = instrumentr_value_as_promise(value);

            int promise_id = instrumentr_promise_get_id(promise);

            Argument* argument =
                argument_table.lookup_permissive(promise_id, call_id);

            if (argument != NULL) {
                argument->set_actual_position(index);
            }
        }

        ++index;
        arguments = instrumentr_pairlist_get_cdr(pairlist);
    }
}

void closure_call_entry_callback(instrumentr_tracer_t tracer,
                                 instrumentr_callback_t callback,
                                 instrumentr_state_t state,
                                 instrumentr_application_t application,
                                 instrumentr_closure_t closure,
                                 instrumentr_call_t call) {
    TracingState& tracing_state = TracingState::lookup(state);

    /* handle environments */

    EnvironmentTable& env_table = tracing_state.get_environment_table();

    Environment* fun_env_data =
        env_table.insert(instrumentr_closure_get_environment(closure));

    Environment* call_env_data =
        env_table.insert(instrumentr_call_get_environment(call));

    call_env_data->add_event("CallEntry");

    /* handle closure */

    FunctionTable& function_table = tracing_state.get_function_table();

    Function* function_data = function_table.insert(closure);

    function_data->call();

    /* handle call */

    CallTable& call_table = tracing_state.get_call_table();

    Call* call_data = call_table.insert(call, function_data);

    /* handle arguments */

    // NOTE: this table is not needed
    // ArgumentTable& argument_table = tracing_state.get_argument_table();
    //
    // process_arguments(
    //    argument_table, call, closure, call_data, function_data,
    //    call_env_data);
    //
    // process_actuals(argument_table, call);

    /* handle backtrace */
    Backtrace& backtrace = tracing_state.get_backtrace();

    backtrace.push(call);

    std::string closure_name =
        charptr_to_string(instrumentr_closure_get_name(closure));

    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();

    if (closure_name == "library" || closure_name == "loadNamespace") {
        env_access_table.push_library();
    }
}

// void handle_call_result(instrumentr_closure_t closure,
//                        instrumentr_call_t call,
//                        EnvironmentTable& env_table) {
//    if (!instrumentr_call_has_result(call)) {
//        return;
//    }
//
//    instrumentr_value_t result = instrumentr_call_get_result(call);
//
//    handle_environment(result, env_table);
//}
//
// void handle_environment(instrumentr_value_t value,
//                        EnvironmentTable& env_table) {
//    Environment* env = env_table;
//
//    if (instrumentr_value_is_environment(result)) {
//        Environment* env =
//            env_table.insert(instrumentr_value_as_environment(result));
//
//        SEXP r_classes =
//            Rf_getAttrib(instrumentr_value_get_sexp(result), R_ClassSymbol);
//
//        if (r_classes != R_NilValue && TYPEOF(r_classes) == STRSXP) {
//            for (int i = 0; i < Rf_length(r_classes); ++i) {
//                SEXP r_class = STRING_ELT(r_classes, i);
//                if (r_class != NA_STRING) {
//                    env->update_class(CHAR(r_class));
//                }
//            }
//        }
//    }
//
//    else if (instrumentr_value_is_environment(result))
//}

void handle_closure_environment_access(instrumentr_state_t state,
                                       instrumentr_call_stack_t call_stack,
                                       instrumentr_call_t call,
                                       instrumentr_closure_t closure,
                                       Backtrace& backtrace,
                                       EnvironmentAccessTable& env_access_table,
                                       EnvironmentTable& env_table) {
    std::string fun_name =
        charptr_to_string(instrumentr_closure_get_name(closure));

    int time = instrumentr_state_get_time(state);

    int result_env_id = NA_INTEGER;

    std::string result_env_type = ENVTRACER_NA_STRING;

    instrumentr_value_t result = nullptr;

    instrumentr_environment_t environment = nullptr;

    if (instrumentr_call_has_result(call)) {
        result = instrumentr_call_get_result(call);
        result_env_type = get_sexp_type(instrumentr_value_get_sexp(result));

        if (instrumentr_value_is_environment(result)) {
            environment = instrumentr_value_as_environment(result);
            result_env_id = instrumentr_value_get_id(result);
        }
    }

    if (result_env_id == NA_INTEGER) {
        return;
    }

    std::string name = ENVTRACER_NA_STRING;

    std::string name_type = ENVTRACER_NA_STRING;

    if (fun_name == "getNamespace") {
        instrumentr_value_t name_val = lookup_environment(
            state, instrumentr_call_get_environment(call), "name", true);

        if (name_val != nullptr) {
            SEXP r_name_val = instrumentr_value_get_sexp(name_val);
            name_type = get_sexp_type(r_name_val);

            if (instrumentr_value_is_character(name_val)) {
                name = CHAR(STRING_ELT(r_name_val, 0));
            } else if (instrumentr_value_is_symbol(name_val)) {
                name = CHAR(PRINTNAME(r_name_val));
            }
        }
    }

    int source_fun_id_1 = NA_INTEGER;
    int source_call_id_1 = NA_INTEGER;
    int source_fun_id_2 = NA_INTEGER;
    int source_call_id_2 = NA_INTEGER;
    int source_fun_id_3 = NA_INTEGER;
    int source_call_id_3 = NA_INTEGER;
    int source_fun_id_4 = NA_INTEGER;
    int source_call_id_4 = NA_INTEGER;
    int frame_index = 1;

    get_four_caller_info(call_stack,
                         source_fun_id_1,
                         source_call_id_1,
                         source_fun_id_2,
                         source_call_id_2,
                         source_fun_id_3,
                         source_call_id_3,
                         source_fun_id_4,
                         source_call_id_4,
                         frame_index);

    Environment* env = env_table.insert(environment);

    env->add_event(fun_name == "getNamespace" ? fun_name : "Return");

    EnvironmentAccess* env_access = new EnvironmentAccess(
        time, NA_INTEGER, fun_name == "getNamespace" ? fun_name : "Return");

    env_access->set_result_env("environment", env->get_id());

    if (fun_name == "getNamespace") {
        env_access->set_x(name_type, NA_INTEGER, name);
    }

    else {
        env_access->set_fun("closure", instrumentr_closure_get_id(closure));
    }

    env_access->set_source(source_fun_id_1,
                           source_call_id_1,
                           source_fun_id_2,
                           source_call_id_2,
                           source_fun_id_3,
                           source_call_id_3,
                           source_fun_id_4,
                           source_call_id_4);

    env_access->set_backtrace(backtrace.to_string());

    env_access_table.insert(env_access);
}

void closure_call_exit_callback(instrumentr_tracer_t tracer,
                                instrumentr_callback_t callback,
                                instrumentr_state_t state,
                                instrumentr_application_t application,
                                instrumentr_closure_t closure,
                                instrumentr_call_t call) {
    TracingState& tracing_state = TracingState::lookup(state);

    ArgumentTable& argument_table = tracing_state.get_argument_table();

    /* handle calls */
    CallTable& call_table = tracing_state.get_call_table();

    EnvironmentTable& env_table = tracing_state.get_environment_table();

    Environment* call_env_data =
        env_table.insert(instrumentr_call_get_environment(call));

    call_env_data->add_event("CallExit");

    int call_id = instrumentr_call_get_id(call);

    bool has_result = instrumentr_call_has_result(call);

    instrumentr_value_t result = nullptr;

    std::string result_type = ENVTRACER_NA_STRING;

    if (has_result) {
        result = instrumentr_call_get_result(call);
        instrumentr_value_type_t val_type = instrumentr_value_get_type(result);
        result_type = instrumentr_value_type_get_name(val_type);
    }

    Call* call_data = call_table.lookup(call_id);

    call_data->exit(result_type);

    /* handle backtrace */
    Backtrace& backtrace = tracing_state.get_backtrace();

    backtrace.pop();

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();

    std::string closure_name =
        charptr_to_string(instrumentr_closure_get_name(closure));

    if (closure_name == "library" || closure_name == "loadNamespace") {
        env_access_table.pop_library();
    }

    handle_closure_environment_access(state,
                                      call_stack,
                                      call,
                                      closure,
                                      backtrace,
                                      env_access_table,
                                      env_table);
}

void compute_meta_depth(instrumentr_state_t state,
                        const std::string& meta_type,
                        MetaprogrammingTable& meta_table,
                        Argument* argument,
                        int call_id) {
    int meta_depth = 0;
    int sink_fun_id = NA_INTEGER;
    int sink_call_id = NA_INTEGER;

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    /* topmost frame is substitute call */
    for (int i = 0; i < instrumentr_call_stack_get_size(call_stack); ++i) {
        instrumentr_frame_t frame =
            instrumentr_call_stack_peek_frame(call_stack, i);

        if (!instrumentr_frame_is_call(frame)) {
            continue;
        }

        instrumentr_call_t call = instrumentr_frame_as_call(frame);

        instrumentr_value_t function = instrumentr_call_get_function(call);

        if (!instrumentr_value_is_closure(function)) {
            continue;
        }

        int current_call_id = instrumentr_call_get_id(call);

        if (sink_fun_id == NA_INTEGER) {
            sink_call_id = current_call_id;
            sink_fun_id = instrumentr_value_get_id(function);
        }

        if (current_call_id == call_id) {
            meta_table.insert(meta_type,
                              argument->get_fun_id(),
                              argument->get_call_id(),
                              argument->get_id(),
                              argument->get_formal_pos(),
                              sink_fun_id,
                              sink_call_id,
                              meta_depth);

            /* NOTE: expression metaprogramming happens even when substitute
               is called. calling this for substitute will double count
               metaprogramming. */
            if (meta_type == "expression") {
                argument->metaprogram(meta_depth);
            }

            break;
        }

        ++meta_depth;
    }
}

void promise_substitute_callback(instrumentr_tracer_t tracer,
                                 instrumentr_callback_t callback,
                                 instrumentr_state_t state,
                                 instrumentr_application_t application,
                                 instrumentr_promise_t promise) {
    if (instrumentr_promise_get_type(promise) !=
        INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
        return;
    }

    TracingState& tracing_state = TracingState::lookup(state);
    CallTable& call_table = tracing_state.get_call_table();
    ArgumentTable& argument_table = tracing_state.get_argument_table();
    MetaprogrammingTable& meta_table =
        tracing_state.get_metaprogramming_table();

    int promise_id = instrumentr_promise_get_id(promise);

    const std::vector<Argument*>& arguments = argument_table.lookup(promise_id);
    std::vector<instrumentr_call_t> calls =
        instrumentr_promise_get_calls(promise);

    for (Argument* argument: arguments) {
        int call_id = argument->get_call_id();

        Call* call_data = call_table.lookup(call_id);

        /* NOTE: first check escaped then lookup */
        if (call_data->has_exited()) {
            argument->escaped();
        }

        compute_meta_depth(state, "substitute", meta_table, argument, call_id);
    }
}

void promise_expression_lookup_callback(instrumentr_tracer_t tracer,
                                        instrumentr_callback_t callback,
                                        instrumentr_state_t state,
                                        instrumentr_application_t application,
                                        instrumentr_promise_t promise) {
    if (instrumentr_promise_get_type(promise) !=
        INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
        return;
    }

    TracingState& tracing_state = TracingState::lookup(state);
    CallTable& call_table = tracing_state.get_call_table();
    ArgumentTable& argument_table = tracing_state.get_argument_table();
    MetaprogrammingTable& meta_table =
        tracing_state.get_metaprogramming_table();

    int promise_id = instrumentr_promise_get_id(promise);

    const std::vector<Argument*>& arguments = argument_table.lookup(promise_id);
    std::vector<instrumentr_call_t> calls =
        instrumentr_promise_get_calls(promise);

    for (Argument* argument: arguments) {
        int call_id = argument->get_call_id();

        Call* call_data = call_table.lookup(call_id);

        /* NOTE: first check escaped then lookup */
        if (call_data->has_exited()) {
            argument->escaped();
        }

        compute_meta_depth(state, "expression", meta_table, argument, call_id);
    }
}

void promise_value_lookup_callback(instrumentr_tracer_t tracer,
                                   instrumentr_callback_t callback,
                                   instrumentr_state_t state,
                                   instrumentr_application_t application,
                                   instrumentr_promise_t promise) {
    if (instrumentr_promise_get_type(promise) !=
        INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
        return;
    }

    TracingState& tracing_state = TracingState::lookup(state);
    CallTable& call_table = tracing_state.get_call_table();
    ArgumentTable& argument_table = tracing_state.get_argument_table();

    int promise_id = instrumentr_promise_get_id(promise);
    const std::vector<Argument*>& arguments = argument_table.lookup(promise_id);

    for (Argument* argument: arguments) {
        int call_id = argument->get_call_id();

        Call* call_data = call_table.lookup(call_id);

        /* NOTE: first check escaped then lookup */
        if (call_data->has_exited()) {
            argument->escaped();
        }

        argument->lookup();
    }
}

int compute_companion_position(ArgumentTable& argument_table,
                               int call_id,
                               instrumentr_frame_t frame) {
    instrumentr_promise_t frame_promise = instrumentr_frame_as_promise(frame);
    int frame_promise_id = instrumentr_promise_get_id(frame_promise);

    if (instrumentr_promise_get_type(frame_promise) !=
        INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
        return NA_INTEGER;
    }

    const std::vector<instrumentr_call_t>& calls =
        instrumentr_promise_get_calls(frame_promise);

    for (int i = 0; i < calls.size(); ++i) {
        instrumentr_call_t promise_call = calls[i];
        int promise_call_id = instrumentr_call_get_id(promise_call);

        if (promise_call_id == call_id) {
            Argument* promise_argument =
                argument_table.lookup(frame_promise_id, call_id);
            return promise_argument->get_formal_pos();
        }
    }

    return NA_INTEGER;
}

void compute_depth_and_companion(instrumentr_state_t state,
                                 ArgumentTable& argument_table,
                                 Call* call_data,
                                 Argument* argument) {
    int call_id = call_data->get_id();

    int argument_id = argument->get_id();

    int force_depth = 0;
    int companion_position = NA_INTEGER;

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    for (int i = 1; i < instrumentr_call_stack_get_size(call_stack); ++i) {
        instrumentr_frame_t frame =
            instrumentr_call_stack_peek_frame(call_stack, i);

        if (companion_position == NA_INTEGER &&
            instrumentr_frame_is_promise(frame)) {
            companion_position =
                compute_companion_position(argument_table, call_id, frame);
        }

        if (!argument->has_escaped() && instrumentr_frame_is_call(frame)) {
            instrumentr_call_t frame_call = instrumentr_frame_as_call(frame);

            int frame_call_id = instrumentr_call_get_id(frame_call);

            instrumentr_value_t function =
                instrumentr_call_get_function(frame_call);

            if (instrumentr_value_is_closure(function)) {
                ++force_depth;
            }

            if (frame_call_id == call_id) {
                break;
            }
        }
    }

    argument->force(argument->has_escaped() ? NA_INTEGER : force_depth,
                    companion_position);
}

void compute_parent_argument(instrumentr_state_t state,
                             ArgumentTable& argument_table,
                             instrumentr_promise_t promise) {
    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    int promise_id = instrumentr_promise_get_id(promise);
    int birth_time = instrumentr_promise_get_birth_time(promise);

    for (int i = 1; i < instrumentr_call_stack_get_size(call_stack); ++i) {
        instrumentr_frame_t frame =
            instrumentr_call_stack_peek_frame(call_stack, i);

        if (!instrumentr_frame_is_promise(frame)) {
            continue;
        }

        instrumentr_promise_t parent_promise =
            instrumentr_frame_as_promise(frame);

        if (instrumentr_promise_get_type(parent_promise) !=
            INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
            continue;
        }

        int force_time =
            instrumentr_promise_get_force_entry_time(parent_promise);

        /* this means the promise was born while this promise was being
           evaluated. this is not interesting. */
        if (force_time < birth_time) {
            continue;
        }

        int parent_promise_id = instrumentr_promise_get_id(parent_promise);

        const std::vector<Argument*>& parent_arguments =
            argument_table.lookup(parent_promise_id);

        /* we take last argument because it refers to the closest call. */
        Argument* parent_argument = parent_arguments.back();

        const std::vector<Argument*>& arguments =
            argument_table.lookup(promise_id);

        for (Argument* argument: arguments) {
            argument->set_parent(parent_argument);
        }

        return;
    }
}

void promise_force_entry_callback(instrumentr_tracer_t tracer,
                                  instrumentr_callback_t callback,
                                  instrumentr_state_t state,
                                  instrumentr_application_t application,
                                  instrumentr_promise_t promise) {
    if (instrumentr_promise_get_type(promise) !=
        INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
        return;
    }

    TracingState& tracing_state = TracingState::lookup(state);
    CallTable& call_table = tracing_state.get_call_table();
    ArgumentTable& argument_table = tracing_state.get_argument_table();

    int promise_id = instrumentr_promise_get_id(promise);
    const std::vector<Argument*>& arguments = argument_table.lookup(promise_id);

    for (Argument* argument: arguments) {
        int call_id = argument->get_call_id();

        Call* call_data = call_table.lookup(call_id);

        /* NOTE: the order of these statements is important.
         force position changes after adding to call*/
        int force_position = call_data->get_force_position();

        argument->set_force_position(force_position);

        call_data->force_argument(argument->get_formal_pos());

        /* NOTE: first check escaped */
        if (call_data->has_exited()) {
            argument->escaped();
        }

        compute_depth_and_companion(state, argument_table, call_data, argument);
    }

    compute_parent_argument(state, argument_table, promise);
}

void promise_force_exit_callback(instrumentr_tracer_t tracer,
                                 instrumentr_callback_t callback,
                                 instrumentr_state_t state,
                                 instrumentr_application_t application,
                                 instrumentr_promise_t promise) {
    if (instrumentr_promise_get_type(promise) !=
        INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
        return;
    }

    instrumentr_environment_t environment = nullptr;

    if (instrumentr_promise_is_forced(promise)) {
        instrumentr_value_t value = instrumentr_promise_get_value(promise);
        if (instrumentr_value_is_environment(value)) {
            environment = instrumentr_value_as_environment(value);
        }
    }

    if (environment == nullptr) {
        return;
    }

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    TracingState& tracing_state = TracingState::lookup(state);

    EnvironmentTable& env_table = tracing_state.get_environment_table();

    CallTable& call_table = tracing_state.get_call_table();

    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();

    Backtrace& backtrace = tracing_state.get_backtrace();

    Environment* env = env_table.insert(environment);

    env->add_event("Argument");

    instrumentr_call_t call = instrumentr_promise_get_call(promise);

    Call* call_data = call_table.lookup(instrumentr_call_get_id(call));

    int time = instrumentr_state_get_time(state);

    EnvironmentAccess* env_access =
        new EnvironmentAccess(time, NA_INTEGER, "Argument");

    /* NOTE: we are setting call id for a reason */
    env_access->set_fun("closure", call_data->get_fun_id());

    int source_fun_id_1 = NA_INTEGER;
    int source_call_id_1 = NA_INTEGER;
    int source_fun_id_2 = NA_INTEGER;
    int source_call_id_2 = NA_INTEGER;
    int source_fun_id_3 = NA_INTEGER;
    int source_call_id_3 = NA_INTEGER;
    int source_fun_id_4 = NA_INTEGER;
    int source_call_id_4 = NA_INTEGER;
    int frame_index = 1;

    get_four_caller_info(call_stack,
                         source_fun_id_1,
                         source_call_id_1,
                         source_fun_id_2,
                         source_call_id_2,
                         source_fun_id_3,
                         source_call_id_3,
                         source_fun_id_4,
                         source_call_id_4,
                         frame_index);

    env_access->set_source(source_fun_id_1,
                           source_call_id_1,
                           source_fun_id_2,
                           source_call_id_2,
                           source_fun_id_3,
                           source_call_id_3,
                           source_fun_id_4,
                           source_call_id_4);

    env_access->set_backtrace(backtrace.to_string());

    env_access_table.insert(env_access);
}

void tracing_entry_callback(instrumentr_tracer_t tracer,
                            instrumentr_callback_t callback,
                            instrumentr_state_t state) {
    TracingState::initialize(state);

    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();

    std::vector<instrumentr_environment_t> namespaces =
        instrumentr_state_get_namespaces(state);

    for (auto ns: namespaces) {
        analyze_package_environment(state, env_table, ns, "namespace");
    }

    std::vector<instrumentr_environment_t> packages =
        instrumentr_state_get_packages(state);

    for (auto packs: packages) {
        analyze_package_environment(state, env_table, packs, "package");
    }

    env_table.insert(instrumentr_state_get_global_env(state));
    env_table.insert(instrumentr_state_get_empty_env(state));
}

void tracing_exit_callback(instrumentr_tracer_t tracer,
                           instrumentr_callback_t callback,
                           instrumentr_state_t state) {
    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    FunctionTable& fun_table = tracing_state.get_function_table();

    fun_table.infer_qualified_names(env_table);

    TracingState::finalize(state);
}

void get_call_info(instrumentr_call_stack_t call_stack,
                   int position,
                   bool builtin,
                   bool special,
                   bool closure,
                   std::string& fun_name,
                   std::string& pack_name,
                   int& call_id) {
    pack_name = ENVTRACER_NA_STRING;
    fun_name = ENVTRACER_NA_STRING;
    call_id = NA_INTEGER;

    if (instrumentr_call_stack_get_size(call_stack) <= position) {
        return;
    }

    instrumentr_frame_t frame =
        instrumentr_call_stack_peek_frame(call_stack, position);

    if (!instrumentr_frame_is_call(frame)) {
        return;
    }

    instrumentr_call_t call = instrumentr_frame_as_call(frame);

    instrumentr_value_t function = instrumentr_call_get_function(call);

    if (builtin && instrumentr_value_is_builtin(function)) {
        instrumentr_builtin_t builtin = instrumentr_value_as_builtin(function);
        fun_name = charptr_to_string(instrumentr_builtin_get_name(builtin));
        pack_name = "base";
    }

    else if (special && instrumentr_value_is_special(function)) {
        instrumentr_special_t special = instrumentr_value_as_special(function);
        fun_name = charptr_to_string(instrumentr_special_get_name(special));
        pack_name = "base";
    }

    else if (closure && instrumentr_value_is_closure(function)) {
        instrumentr_closure_t closure = instrumentr_value_as_closure(function);
        fun_name = charptr_to_string(instrumentr_closure_get_name(closure));
        instrumentr_environment_t environment =
            instrumentr_closure_get_environment(closure);
        pack_name =
            charptr_to_string(instrumentr_environment_get_name(environment));
    }

    if (fun_name != ENVTRACER_NA_STRING) {
        call_id = instrumentr_call_get_id(call);
    }
}

void subset_or_subassign_callback(instrumentr_tracer_t tracer,
                                  instrumentr_callback_t callback,
                                  instrumentr_state_t state,
                                  instrumentr_application_t application,
                                  instrumentr_call_t call,
                                  instrumentr_value_t x,
                                  instrumentr_value_t index,
                                  instrumentr_value_t result) {
    if (!instrumentr_value_is_environment(x)) {
        return;
    }

    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    if (env_access_table.inside_library()) {
        return;
    }

    int time = instrumentr_state_get_time(state);

    int depth = NA_INTEGER;

    Environment* env = env_table.insert(instrumentr_value_as_environment(x));

    std::string varname = ENVTRACER_NA_STRING;

    if (instrumentr_value_is_pairlist(index)) {
        index =
            instrumentr_pairlist_get_car(instrumentr_value_as_pairlist(index));
    }

    if (instrumentr_value_is_symbol(index)) {
        varname = CHAR(PRINTNAME(instrumentr_value_get_sexp(index)));
    } else if (instrumentr_value_is_character(index)) {
        varname = CHAR(STRING_ELT(instrumentr_value_get_sexp(index), 0));
    } else if (instrumentr_value_is_char(index)) {
        varname = CHAR(instrumentr_value_get_sexp(index));
    }

    std::string value_type = get_sexp_type(instrumentr_value_get_sexp(result));

    int call_id = instrumentr_call_get_id(call);
    int env_id = env->get_id();

    std::string fun_name = ENVTRACER_NA_STRING;

    instrumentr_value_t function = instrumentr_call_get_function(call);

    if (instrumentr_value_is_special(function)) {
        fun_name = instrumentr_special_get_name(
            instrumentr_value_as_special(function));
    }

    else if (instrumentr_value_is_builtin(function)) {
        fun_name = instrumentr_builtin_get_name(
            instrumentr_value_as_builtin(function));
    }

    env->add_event(fun_name);

    int source_fun_id_1;
    int source_call_id_1;
    int source_fun_id_2;
    int source_call_id_2;
    int source_fun_id_3;
    int source_call_id_3;
    int source_fun_id_4;
    int source_call_id_4;
    int frame_index = 1;

    get_four_caller_info(call_stack,
                         source_fun_id_1,
                         source_call_id_1,
                         source_fun_id_2,
                         source_call_id_2,
                         source_fun_id_3,
                         source_call_id_3,
                         source_fun_id_4,
                         source_call_id_4,
                         frame_index);

    EnvironmentAccess* env_access =
        new EnvironmentAccess(time, depth, fun_name);

    env_access->set_backtrace(backtrace.to_string());

    env_access->set_source(source_fun_id_1,
                           source_call_id_1,
                           source_fun_id_2,
                           source_call_id_2,
                           source_fun_id_3,
                           source_call_id_3,
                           source_fun_id_4,
                           source_call_id_4);
    env_access->set_symbol(varname);

    env_access->set_side_effect(env->get_id(), value_type);

    env_access_table.insert(env_access);
}

void process_reads_and_writes(instrumentr_state_t state,
                              instrumentr_environment_t environment,
                              const std::string& event,
                              const std::string& varname,
                              const std::string& value_type,
                              EnvironmentTable& environment_table,
                              EnvironmentAccessTable& env_access_table,
                              Backtrace& backtrace) {
    if (env_access_table.inside_library()) {
        return;
    }

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    Environment* env = environment_table.insert(environment);

    bool record = false;

    std::string fun_name = ENVTRACER_NA_STRING;
    std::string pack_name = ENVTRACER_NA_STRING;
    int call_id = NA_INTEGER;
    int frame_index = 3;

    if (event == "ls") {
        frame_index = 7;
    } else if (event == "R") {
        frame_index = 5;
    }

    get_call_info(call_stack,
                  frame_index,
                  false,
                  false,
                  true,
                  fun_name,
                  pack_name,
                  call_id);

    if ((pack_name == "base") &&
        ((event == "L" &&
          (fun_name == "get" || fun_name == "get0" || fun_name == "mget")) ||
         (event == "A" && (fun_name == "assign")) ||
         (event == "D" && (fun_name == "assign")) ||
         (event == "E" && (fun_name == "exists")) ||
         (event == "R" && (fun_name == "remove" || fun_name == "rm")) ||
         (event == "ls" && (fun_name == "ls" || fun_name == "objects")))) {
        record = true;

        /* get0 is called by dynGet */
        if (fun_name == "get0") {
            std::string parent_pack_name("");
            std::string parent_fun_name("");
            int parent_call_id = NA_INTEGER;
            get_call_info(call_stack,
                          15,
                          false,
                          false,
                          true,
                          parent_fun_name,
                          parent_pack_name,
                          parent_call_id);

            if (parent_fun_name == "dynGet") {
                fun_name = parent_fun_name;
                frame_index = 15;
            }
        }
    }

    else if (env->inside_eval()) {
        record = true;
        fun_name = event;
        frame_index = 0;
    }

    if (record) {
        int time = instrumentr_state_get_time(state);

        int depth = NA_INTEGER;

        EnvironmentAccess* env_access =
            new EnvironmentAccess(time, depth, fun_name);

        env_access->set_backtrace(backtrace.to_string());

        env_access->set_side_effect(env->get_id(), value_type);

        env_access->set_symbol(varname);

        int source_fun_id_1;
        int source_call_id_1;
        int source_fun_id_2;
        int source_call_id_2;
        int source_fun_id_3;
        int source_call_id_3;
        int source_fun_id_4;
        int source_call_id_4;
        int frame_index = 1;

        get_four_caller_info(call_stack,
                             source_fun_id_1,
                             source_call_id_1,
                             source_fun_id_2,
                             source_call_id_2,
                             source_fun_id_3,
                             source_call_id_3,
                             source_fun_id_4,
                             source_call_id_4,
                             frame_index);

        env_access->set_source(source_fun_id_1,
                               source_call_id_1,
                               source_fun_id_2,
                               source_call_id_2,
                               source_fun_id_3,
                               source_call_id_3,
                               source_fun_id_4,
                               source_call_id_4);

        env_access_table.insert(env_access);

        env->add_event(fun_name);
    }
}

void variable_lookup(instrumentr_tracer_t tracer,
                     instrumentr_callback_t callback,
                     instrumentr_state_t state,
                     instrumentr_application_t application,
                     instrumentr_symbol_t symbol,
                     instrumentr_value_t value,
                     instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    ArgumentTable& arg_table = tracing_state.get_argument_table();
    EffectsTable& effects_table = tracing_state.get_effects_table();
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_char_t charval = instrumentr_symbol_get_element(symbol);
    std::string varname = instrumentr_char_get_element(charval);

    std::string value_type = get_sexp_type(instrumentr_value_get_sexp(value));

    process_reads_and_writes(state,
                             environment,
                             "L",
                             varname,
                             value_type,
                             env_table,
                             env_access_table,
                             backtrace);
}

void variable_exists(instrumentr_tracer_t tracer,
                     instrumentr_callback_t callback,
                     instrumentr_state_t state,
                     instrumentr_application_t application,
                     instrumentr_symbol_t symbol,
                     instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    ArgumentTable& arg_table = tracing_state.get_argument_table();
    EffectsTable& effects_table = tracing_state.get_effects_table();
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_char_t charval = instrumentr_symbol_get_element(symbol);
    std::string varname = instrumentr_char_get_element(charval);

    std::string value_type = ENVTRACER_NA_STRING;

    process_reads_and_writes(state,
                             environment,
                             "E",
                             varname,
                             value_type,
                             env_table,
                             env_access_table,
                             backtrace);
}

void function_context_lookup(instrumentr_tracer_t tracer,
                             instrumentr_callback_t callback,
                             instrumentr_state_t state,
                             instrumentr_application_t application,
                             instrumentr_symbol_t symbol,
                             instrumentr_value_t value,
                             instrumentr_environment_t environment) {
    if (!instrumentr_value_is_promise(value)) {
        return;
    }

    instrumentr_promise_t promise = instrumentr_value_as_promise(value);

    if (instrumentr_promise_get_type(promise) !=
        INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
        return;
    }

    TracingState& tracing_state = TracingState::lookup(state);

    ArgumentTable& arg_tab = tracing_state.get_argument_table();

    int promise_id = instrumentr_promise_get_id(promise);

    bool forced = instrumentr_promise_is_forced(promise);

    const std::vector<Argument*>& args = arg_tab.lookup(promise_id);

    for (auto arg: args) {
        arg->set_context_lookup();

        if (!forced) {
            arg->set_context_force();
        }
    }
}

void variable_assign(instrumentr_tracer_t tracer,
                     instrumentr_callback_t callback,
                     instrumentr_state_t state,
                     instrumentr_application_t application,
                     instrumentr_symbol_t symbol,
                     instrumentr_value_t value,
                     instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    ArgumentTable& arg_table = tracing_state.get_argument_table();
    EffectsTable& effects_table = tracing_state.get_effects_table();
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_char_t charval = instrumentr_symbol_get_element(symbol);
    std::string varname = instrumentr_char_get_element(charval);
    std::string value_type = get_sexp_type(instrumentr_value_get_sexp(value));

    process_reads_and_writes(state,
                             environment,
                             "A",
                             varname,
                             value_type,
                             env_table,
                             env_access_table,
                             backtrace);
}

void variable_define(instrumentr_tracer_t tracer,
                     instrumentr_callback_t callback,
                     instrumentr_state_t state,
                     instrumentr_application_t application,
                     instrumentr_symbol_t symbol,
                     instrumentr_value_t value,
                     instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    ArgumentTable& arg_table = tracing_state.get_argument_table();
    EffectsTable& effects_table = tracing_state.get_effects_table();
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_char_t charval = instrumentr_symbol_get_element(symbol);
    std::string varname = instrumentr_char_get_element(charval);
    std::string value_type = get_sexp_type(instrumentr_value_get_sexp(value));

    process_reads_and_writes(state,
                             environment,
                             "D",
                             varname,
                             value_type,
                             env_table,
                             env_access_table,
                             backtrace);
}

void variable_remove(instrumentr_tracer_t tracer,
                     instrumentr_callback_t callback,
                     instrumentr_state_t state,
                     instrumentr_application_t application,
                     instrumentr_symbol_t symbol,
                     instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    ArgumentTable& arg_table = tracing_state.get_argument_table();
    EffectsTable& effects_table = tracing_state.get_effects_table();
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_char_t charval = instrumentr_symbol_get_element(symbol);
    std::string varname = instrumentr_char_get_element(charval);
    std::string value_type = ENVTRACER_NA_STRING;

    process_reads_and_writes(state,
                             environment,
                             "R",
                             varname,
                             value_type,
                             env_table,
                             env_access_table,
                             backtrace);
}

void environment_ls(instrumentr_tracer_t tracer,
                    instrumentr_callback_t callback,
                    instrumentr_state_t state,
                    instrumentr_application_t application,
                    instrumentr_environment_t environment,
                    instrumentr_character_t result) {
    TracingState& tracing_state = TracingState::lookup(state);
    ArgumentTable& arg_table = tracing_state.get_argument_table();
    EffectsTable& effects_table = tracing_state.get_effects_table();
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();
    Backtrace& backtrace = tracing_state.get_backtrace();

    std::string varname = ENVTRACER_NA_STRING;
    std::string value_type = ENVTRACER_NA_STRING;

    process_reads_and_writes(state,
                             environment,
                             "ls",
                             varname,
                             value_type,
                             env_table,
                             env_access_table,
                             backtrace);
}

void value_finalize(instrumentr_tracer_t tracer,
                    instrumentr_callback_t callback,
                    instrumentr_state_t state,
                    instrumentr_application_t application,
                    instrumentr_value_t value) {
    TracingState& tracing_state = TracingState::lookup(state);

    if (instrumentr_value_is_closure(value)) {
        instrumentr_closure_t closure = instrumentr_value_as_closure(value);

        FunctionTable& function_table = tracing_state.get_function_table();

        int id = instrumentr_closure_get_id(closure);
        Function* fun = function_table.lookup(id);

        if (fun != NULL) {
            fun->set_name(instrumentr_closure_get_name(closure));
        }
    }

    else if (instrumentr_value_is_environment(value)) {
        instrumentr_environment_t environment =
            instrumentr_value_as_environment(value);

        EnvironmentTable& environment_table =
            tracing_state.get_environment_table();

        int id = instrumentr_environment_get_id(environment);
        Environment* env = environment_table.lookup(id);

        if (env != NULL) {
            env->set_name(instrumentr_environment_get_name(environment));

            instrumentr_environment_type_t type =
                instrumentr_environment_get_type(environment);
            const char* env_type = instrumentr_environment_type_to_string(type);
            env->set_type(env_type);

            if (type == INSTRUMENTR_ENVIRONMENT_TYPE_CALL) {
                instrumentr_call_t call =
                    instrumentr_environment_get_call(environment);
                int call_id = instrumentr_call_get_id(call);

                env->set_call_id(call_id);
            }

            const char* env_name =
                instrumentr_environment_get_name(environment);
            env->set_name(env_name);
        }
    }
}

void trace_error(instrumentr_tracer_t tracer,
                 instrumentr_callback_t callback,
                 instrumentr_state_t state,
                 instrumentr_application_t application,
                 instrumentr_value_t call_expr) {
    TracingState& tracing_state = TracingState::lookup(state);
    ArgumentTable& arg_tab = tracing_state.get_argument_table();
    EffectsTable& effects_tab = tracing_state.get_effects_table();
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    bool transitive = false;
    int source_fun_id = NA_INTEGER;
    int source_call_id = NA_INTEGER;
    int source_arg_id = NA_INTEGER;
    int source_formal_pos = NA_INTEGER;

    for (int i = 0; i < instrumentr_call_stack_get_size(call_stack); ++i) {
        instrumentr_frame_t frame =
            instrumentr_call_stack_peek_frame(call_stack, i);

        if (!instrumentr_frame_is_promise(frame)) {
            continue;
        }

        instrumentr_promise_t promise = instrumentr_frame_as_promise(frame);

        if (instrumentr_promise_get_type(promise) !=
            INSTRUMENTR_PROMISE_TYPE_ARGUMENT) {
            continue;
        }

        int promise_id = instrumentr_promise_get_id(promise);

        const std::vector<Argument*>& args = arg_tab.lookup(promise_id);

        Argument* arg = args.back();

        arg->side_effect('E', transitive);

        int arg_id = arg->get_id();
        int fun_id = arg->get_fun_id();
        int call_id = arg->get_call_id();
        int formal_pos = arg->get_formal_pos();

        effects_tab.insert('E',
                           ENVTRACER_NA_STRING,
                           transitive,
                           NA_INTEGER,
                           source_fun_id,
                           source_call_id,
                           source_arg_id,
                           source_formal_pos,
                           fun_id,
                           call_id,
                           arg_id,
                           formal_pos,
                           backtrace.to_string());

        if (!transitive) {
            source_fun_id = arg->get_fun_id();
            source_call_id = arg->get_call_id();
            source_arg_id = promise_id;
            source_formal_pos = arg->get_formal_pos();
        }
    }
}

void attribute_set_callback(instrumentr_tracer_t tracer,
                            instrumentr_callback_t callback,
                            instrumentr_state_t state,
                            instrumentr_application_t application,
                            instrumentr_value_t object,
                            instrumentr_symbol_t name,
                            instrumentr_value_t value) {
    if (!instrumentr_value_is_environment(object)) {
        return;
    }

    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();

    Environment* env =
        env_table.insert(instrumentr_value_as_environment(object));

    if (instrumentr_symbol_get_sexp(name) != R_ClassSymbol) {
        return;
    }

    if (!instrumentr_value_is_character(value)) {
        return;
    }

    env->add_event("@");

    instrumentr_character_t classes = instrumentr_value_as_character(value);

    for (int i = 0; i < instrumentr_character_get_size(classes); ++i) {
        instrumentr_char_t char_val =
            instrumentr_character_get_element(classes, i);
        if (!instrumentr_char_is_na(char_val)) {
            env->update_class(instrumentr_char_get_element(char_val));
        }
    }
}

void gc_allocation_callback(instrumentr_tracer_t tracer,
                            instrumentr_callback_t callback,
                            instrumentr_state_t state,
                            instrumentr_application_t application,
                            instrumentr_value_t value) {
    if (!instrumentr_value_is_environment(value)) {
        return;
    }

    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();

    instrumentr_environment_t environment =
        instrumentr_value_as_environment(value);

    Environment* env = env_table.insert(environment);

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    int source_fun_id_1 = NA_INTEGER;
    int source_call_id_1 = NA_INTEGER;
    int source_fun_id_2 = NA_INTEGER;
    int source_call_id_2 = NA_INTEGER;
    int source_fun_id_3 = NA_INTEGER;
    int source_call_id_3 = NA_INTEGER;
    int source_fun_id_4 = NA_INTEGER;
    int source_call_id_4 = NA_INTEGER;
    int frame_index = 0;

    get_four_caller_info(call_stack,
                         source_fun_id_1,
                         source_call_id_1,
                         source_fun_id_2,
                         source_call_id_2,
                         source_fun_id_3,
                         source_call_id_3,
                         source_fun_id_4,
                         source_call_id_4,
                         frame_index);

    env->set_source(ENVTRACER_NA_STRING,
                    source_fun_id_1,
                    source_call_id_1,
                    source_fun_id_2,
                    source_call_id_2,
                    source_fun_id_3,
                    source_call_id_3,
                    source_fun_id_4,
                    source_call_id_4);

    Backtrace& backtrace = tracing_state.get_backtrace();

    env->set_backtrace(backtrace.to_string());
}

void use_method_entry_callback(instrumentr_tracer_t tracer,
                               instrumentr_callback_t callback,
                               instrumentr_state_t state,
                               instrumentr_application_t application,
                               instrumentr_value_t object,
                               instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();

    Environment* env = env_table.insert(environment);

    env->set_dispatch();
}

void eval_call_entry(instrumentr_tracer_t tracer,
                     instrumentr_callback_t callback,
                     instrumentr_state_t state,
                     instrumentr_application_t application,
                     instrumentr_value_t expression,
                     instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    EvalTable& eval_table = tracing_state.get_eval_table();
    Backtrace& backtrace = tracing_state.get_backtrace();
    const std::string bt = backtrace.to_string();

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    instrumentr_value_t value = instrumentr_environment_as_value(environment);

    bool direct = true;

    int time = instrumentr_state_get_time(state);

    std::string expr =
        instrumentr_sexp_to_string(instrumentr_value_get_sexp(expression), true)
            .front();

    while (instrumentr_value_is_environment(value)) {
        instrumentr_environment_t envir =
            instrumentr_value_as_environment(value);

        auto type = instrumentr_environment_get_type(envir);

        if (type == INSTRUMENTR_ENVIRONMENT_TYPE_NAMESPACE ||
            type == INSTRUMENTR_ENVIRONMENT_TYPE_PACKAGE) {
            break;
        }

        Environment* env = env_table.insert(envir);
        env->push_eval();

        if (direct) {
            env->add_event("EvalEntryDirect");
        } else {
            env->add_event("EvalEntryIndirect");
        }

        int source_fun_id_1 = NA_INTEGER;
        int source_call_id_1 = NA_INTEGER;
        int source_fun_id_2 = NA_INTEGER;
        int source_call_id_2 = NA_INTEGER;
        int source_fun_id_3 = NA_INTEGER;
        int source_call_id_3 = NA_INTEGER;
        int source_fun_id_4 = NA_INTEGER;
        int source_call_id_4 = NA_INTEGER;
        int frame_index = 1;

        get_four_caller_info(call_stack,
                             source_fun_id_1,
                             source_call_id_1,
                             source_fun_id_2,
                             source_call_id_2,
                             source_fun_id_3,
                             source_call_id_3,
                             source_fun_id_4,
                             source_call_id_4,
                             frame_index);

        Eval* eval = new Eval(time,
                              env->get_id(),
                              direct,
                              expr,
                              source_fun_id_1,
                              source_call_id_1,
                              source_fun_id_2,
                              source_call_id_2,
                              source_fun_id_3,
                              source_call_id_3,
                              source_fun_id_4,
                              source_call_id_4,
                              bt);

        eval_table.insert(eval);

        value = instrumentr_environment_get_parent(envir);

        direct = false;
    }
}

void eval_call_exit(instrumentr_tracer_t tracer,
                    instrumentr_callback_t callback,
                    instrumentr_state_t state,
                    instrumentr_application_t application,
                    instrumentr_value_t expression,
                    instrumentr_environment_t environment,
                    instrumentr_value_t result) {
    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();

    instrumentr_value_t value = instrumentr_environment_as_value(environment);

    bool direct = true;

    while (instrumentr_value_is_environment(value)) {
        instrumentr_environment_t envir =
            instrumentr_value_as_environment(value);

        auto type = instrumentr_environment_get_type(envir);

        if (type == INSTRUMENTR_ENVIRONMENT_TYPE_NAMESPACE ||
            type == INSTRUMENTR_ENVIRONMENT_TYPE_PACKAGE) {
            break;
        }

        Environment* env = env_table.insert(envir);

        env->pop_eval();

        if (direct) {
            env->add_event("EvalExitDirect");
        } else {
            env->add_event("EvalExitIndirect");
        }

        value = instrumentr_environment_get_parent(envir);

        direct = false;
    }
}

void substitute_call_entry(instrumentr_tracer_t tracer,
                           instrumentr_callback_t callback,
                           instrumentr_state_t state,
                           instrumentr_application_t application,
                           instrumentr_value_t expression,
                           instrumentr_environment_t environment) {
    TracingState& tracing_state = TracingState::lookup(state);
    EnvironmentTable& env_table = tracing_state.get_environment_table();
    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();

    Backtrace& backtrace = tracing_state.get_backtrace();

    int time = instrumentr_state_get_time(state);
    int depth = NA_INTEGER;

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    int source_fun_id_1 = NA_INTEGER;
    int source_call_id_1 = NA_INTEGER;
    int source_fun_id_2 = NA_INTEGER;
    int source_call_id_2 = NA_INTEGER;
    int source_fun_id_3 = NA_INTEGER;
    int source_call_id_3 = NA_INTEGER;
    int source_fun_id_4 = NA_INTEGER;
    int source_call_id_4 = NA_INTEGER;
    int frame_index = 1;

    get_four_caller_info(call_stack,
                         source_fun_id_1,
                         source_call_id_1,
                         source_fun_id_2,
                         source_call_id_2,
                         source_fun_id_3,
                         source_call_id_3,
                         source_fun_id_4,
                         source_call_id_4,
                         frame_index);

    Environment* env = env_table.insert(environment);
    env->add_event("substitute");

    EnvironmentAccess* env_access =
        new EnvironmentAccess(time, depth, "substitute");

    env_access->set_result_env("environment", env->get_id());

    env_access->set_source(source_fun_id_1,
                           source_call_id_1,
                           source_fun_id_2,
                           source_call_id_2,
                           source_fun_id_3,
                           source_call_id_3,
                           source_fun_id_4,
                           source_call_id_4);

    env_access->set_backtrace(backtrace.to_string());

    env_access_table.insert(env_access);
}
