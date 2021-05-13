#include "tracer.h"
#include "callbacks.h"
#include "TracingState.h"
#include "utilities.h"
#include <instrumentr/instrumentr.h>
#include <vector>

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

instrumentr_call_t get_caller(instrumentr_call_stack_t call_stack) {
    int env_id = NA_INTEGER;

    for (int i = 1; i < instrumentr_call_stack_get_size(call_stack); ++i) {
        instrumentr_frame_t frame =
            instrumentr_call_stack_peek_frame(call_stack, i);

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

void update_source(instrumentr_call_stack_t call_stack,
                   EnvironmentAccess* env_access) {
    instrumentr_call_t call = get_caller(call_stack);

    if (call == nullptr) {
        return;
    }

    instrumentr_value_t fun = instrumentr_call_get_function(call);
    instrumentr_closure_t closure = instrumentr_value_as_closure(fun);

    env_access->set_source(instrumentr_closure_get_id(closure),
                           instrumentr_call_get_id(call));
}

void builtin_environment_access(instrumentr_call_stack_t call_stack,
                                instrumentr_call_t call,
                                instrumentr_builtin_t builtin,
                                EnvironmentAccessTable& env_access_table) {
    const char* name = instrumentr_builtin_get_name(builtin);

    if (name == NULL) {
        return;
    }

    std::string fun_name = name;

    if (fun_name != "as.environment" && fun_name != "pos.to.env") {
        return;
    }

    int call_id = instrumentr_call_get_id(call);

    instrumentr_value_t arguments = instrumentr_call_get_arguments(call);
    SEXP r_arguments = instrumentr_value_get_sexp(arguments);
    SEXP r_x = CAR(r_arguments);

    std::string x_type = get_sexp_type(r_x);

    EnvironmentAccess* env_access = nullptr;

    if (x_type == "integer") {
        env_access = EnvironmentAccess::XInt(
            call_id, fun_name, x_type, INTEGER_ELT(r_x, 0));
    } else if (x_type == "double") {
        env_access = EnvironmentAccess::XInt(
            call_id, fun_name, x_type, REAL_ELT(r_x, 0));
    } else if (x_type == "character") {
        env_access = EnvironmentAccess::XChar(
            call_id, fun_name, x_type, CHAR(STRING_ELT(r_x, 0)));
    } else {
        env_access = new EnvironmentAccess(call_id, fun_name, x_type);
    }

    if (env_access != nullptr) {
        update_source(call_stack, env_access);
        env_access_table.insert(env_access);
    }
}

void closure_environment_access(instrumentr_call_stack_t call_stack,
                                instrumentr_call_t call,
                                instrumentr_closure_t closure,
                                EnvironmentAccessTable& env_access_table) {
    const char* name = instrumentr_closure_get_name(closure);

    if (name == NULL) {
        return;
    }

    std::string fun_name = name;

    int call_id = instrumentr_call_get_id(call);

    EnvironmentAccess* env_access = nullptr;
    instrumentr_environment_t env = instrumentr_call_get_environment(call);
    SEXP r_env = instrumentr_environment_get_sexp(env);

    if (fun_name == "sys.call" || fun_name == "sys.frame" ||
        fun_name == "sys.function") {
        SEXP r_which = Rf_findVarInFrame(r_env, R_WhichSymbol);

        if (TYPEOF(r_which) == PROMSXP) {
            r_which = dyntrace_get_promise_value(r_which);
        }

        int which = NA_INTEGER;

        const std::string arg_type = get_sexp_type(r_which);

        if (TYPEOF(r_which) == REALSXP) {
            which = REAL_ELT(r_which, 0);
        }

        else if (TYPEOF(r_which) == INTSXP) {
            which = INTEGER_ELT(r_which, 0);
        }

        else {
            Rf_error("incorrect type of which: %s",
                     get_sexp_type(r_which).c_str());
        }

        env_access =
            EnvironmentAccess::Which(call_id, fun_name, arg_type, which);

    } else if (fun_name == "sys.parent" || fun_name == "parent.frame") {
        SEXP r_n = Rf_findVarInFrame(r_env, R_NSymbol);

        if (TYPEOF(r_n) == PROMSXP) {
            r_n = dyntrace_get_promise_value(r_n);
        }

        const std::string arg_type = get_sexp_type(r_n);

        int n = NA_INTEGER;

        if (TYPEOF(r_n) == REALSXP) {
            n = REAL_ELT(r_n, 0);
        }

        else if (TYPEOF(r_n) == INTSXP) {
            n = INTEGER_ELT(r_n, 0);
        }

        else {
            Rf_error("incorrect type of n: %s", get_sexp_type(r_n).c_str());
        }

        env_access = EnvironmentAccess::N(call_id, fun_name, arg_type, n);

    } else if (fun_name == "sys.calls" || fun_name == "sys.frames" ||
               fun_name == "sys.parents" || fun_name == "sys.on.exit" ||
               fun_name == "sys.status" || fun_name == "sys.nframe") {
        env_access =
            new EnvironmentAccess(call_id, fun_name, ENVTRACER_NA_STRING);
    }

    if (env_access != nullptr) {
        update_source(call_stack, env_access);
        env_access_table.insert(env_access);
    }
}

void builtin_call_exit_callback(instrumentr_tracer_t tracer,
                                instrumentr_callback_t callback,
                                instrumentr_state_t state,
                                instrumentr_application_t application,
                                instrumentr_builtin_t builtin,
                                instrumentr_call_t call) {
    TracingState& tracing_state = TracingState::lookup(state);

    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);

    builtin_environment_access(call_stack, call, builtin, env_access_table);

    std::string ref_type = instrumentr_builtin_get_name(builtin);
    int ref_call_id = instrumentr_call_get_id(call);

    /* NOTE: sys.status calls 3 of these functions so it is not added to the
     * list. */
    if (ref_type != "as.environment" && ref_type != "pos.to.env") {
        return;
    }

    if (!has_minus_one_argument(call)) {
        return;
    }

    ArgumentTable& arg_tab = tracing_state.get_argument_table();
    CallTable& call_tab = tracing_state.get_call_table();
    ArgumentReflectionTable& arg_ref_tab = tracing_state.get_arg_ref_tab();
    CallReflectionTable& call_ref_tab = tracing_state.get_call_ref_tab();
    Backtrace& backtrace = tracing_state.get_backtrace();

    mark_promises(
        ref_call_id, ref_type, arg_tab, arg_ref_tab, call_stack, backtrace);

    if (!instrumentr_call_has_result(call)) {
        return;
    }

    /* NOTE: sys.status calls 3 of these functions so it is not added to the
     * list. */
    if (ref_type == "as.environment" || ref_type == "pos.to.env") {
        instrumentr_value_t result = instrumentr_call_get_result(call);

        if (!instrumentr_value_is_environment(result)) {
            return;
        }

        instrumentr_environment_t environment =
            instrumentr_value_as_environment(result);

        mark_escaped_environment_call(ref_call_id,
                                      ref_type,
                                      arg_tab,
                                      call_tab,
                                      call_ref_tab,
                                      call_stack,
                                      environment);
    }

    /* NOTE: this case does not get affected by argument's evaluation position.
      this means that environments of all functions on the stack escape
    else if (ref_type == "sys.frames") {
        mark_escaped_environment_call(ref_call_id,
                                      ref_type,
                                      arg_tab,
                                      call_tab,
                                      call_ref_tab,
                                      call_stack,
                                      NULL);
    }
    */
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

    /* handle closure */

    FunctionTable& function_table = tracing_state.get_function_table();

    Function* function_data = function_table.insert(closure);

    function_data->call();

    /* handle call */

    CallTable& call_table = tracing_state.get_call_table();

    Call* call_data = call_table.insert(call, function_data);

    /* handle arguments */

    ArgumentTable& argument_table = tracing_state.get_argument_table();

    process_arguments(
        argument_table, call, closure, call_data, function_data, call_env_data);

    process_actuals(argument_table, call);

    /* handle backtrace */
    Backtrace& backtrace = tracing_state.get_backtrace();

    backtrace.push(call);
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

void inspect_environments(instrumentr_state_t state,
                          instrumentr_closure_t closure,
                          instrumentr_call_t call,
                          EnvironmentTable& env_table) {
    if (!instrumentr_closure_has_name(closure)) {
        return;
    }

    const std::string name(instrumentr_closure_get_name(closure));

    if (name == "eval" || name == "evalq") {
        instrumentr_environment_t call_env =
            instrumentr_call_get_environment(call);

        instrumentr_symbol_t envir_sym =
            instrumentr_state_get_symbol(state, "envir");

        instrumentr_value_t eval_env =
            instrumentr_environment_lookup(call_env, envir_sym);

        if (instrumentr_value_is_promise(eval_env)) {
            instrumentr_promise_t eval_env_prom =
                instrumentr_value_as_promise(eval_env);
            instrumentr_value_t val =
                instrumentr_promise_get_value(eval_env_prom);

            if (instrumentr_value_is_environment(val)) {
                Environment* env =
                    env_table.insert(instrumentr_value_as_environment(val));
                env->add_eval("eval");
            }
        }
    }

    if (name == "new.env" || name == "list2env") {
        if (instrumentr_call_has_result(call)) {
            instrumentr_value_t result = instrumentr_call_get_result(call);

            if (instrumentr_value_is_environment(result)) {
                instrumentr_environment_t environment =
                    instrumentr_value_as_environment(result);

                instrumentr_call_t caller =
                    get_caller(instrumentr_state_get_call_stack(state));

                int closure_id = NA_INTEGER;
                int call_id = NA_INTEGER;

                if (caller != nullptr) {
                    instrumentr_call_stack_t call_stack =
                        instrumentr_state_get_call_stack(state);
                    instrumentr_call_t call = get_caller(call_stack);

                    if (call == nullptr) {
                        return;
                    }

                    instrumentr_value_t fun =
                        instrumentr_call_get_function(call);
                    instrumentr_closure_t closure =
                        instrumentr_value_as_closure(fun);

                    closure_id = instrumentr_closure_get_id(closure),
                    call_id = instrumentr_call_get_id(call);
                }

                Environment* env = env_table.insert(environment);
                env->set_source(name, closure_id, call_id);
            }
        }
    }
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

    int call_id = instrumentr_call_get_id(call);

    bool has_result = instrumentr_call_has_result(call);

    std::string result_type = ENVTRACER_NA_STRING;
    if (has_result) {
        instrumentr_value_t value = instrumentr_call_get_result(call);
        instrumentr_value_type_t val_type = instrumentr_value_get_type(value);
        result_type = instrumentr_value_type_get_name(val_type);
    }

    Call* call_data = call_table.lookup(call_id);

    call_data->exit(result_type);

    /* handle backtrace */
    Backtrace& backtrace = tracing_state.get_backtrace();

    backtrace.pop();

    /* handle arguments */

    EnvironmentAccessTable& env_access_table =
        tracing_state.get_environment_access_table();

    instrumentr_call_stack_t call_stack =
        instrumentr_state_get_call_stack(state);
    closure_environment_access(call_stack, call, closure, env_access_table);

    EnvironmentTable& env_table = tracing_state.get_environment_table();
    inspect_environments(state, closure, call, env_table);
    //
    // handle_call_result(closure, call, env_table);
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

    TracingState& tracing_state = TracingState::lookup(state);
    CallTable& call_table = tracing_state.get_call_table();
    ArgumentTable& argument_table = tracing_state.get_argument_table();

    int promise_id = instrumentr_promise_get_id(promise);
    const std::vector<Argument*>& arguments = argument_table.lookup(promise_id);

    for (Argument* argument: arguments) {
        int call_id = argument->get_call_id();

        Call* call_data = call_table.lookup(call_id);

        /* NOTE: first check escaped */
        if (call_data->has_exited()) {
            argument->escaped();
        }

        std::string value_type = ENVTRACER_NA_STRING;
        if (instrumentr_promise_is_forced(promise)) {
            instrumentr_value_t value = instrumentr_promise_get_value(promise);
            value_type = instrumentr_value_type_get_name(
                instrumentr_value_get_type(value));
        }

        argument->set_value_type(value_type);
    }
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

void process_reads(instrumentr_state_t state,
                   instrumentr_environment_t environment,
                   const char type,
                   const std::string& varname,
                   ArgumentTable& argument_table,
                   EnvironmentTable& environment_table,
                   EffectsTable& effects_table,
                   Backtrace& backtrace) {
    /* don't process *tmp* as it is an implementation variable */
    if (varname == "*tmp*") {
        return;
    }

    Environment* env = environment_table.insert(environment);
    int t2 = instrumentr_environment_get_last_write_time(environment);
    int env_birth_time = instrumentr_environment_get_birth_time(environment);

    int env_id = instrumentr_environment_get_id(environment);

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

        int t1 = instrumentr_promise_get_birth_time(promise);
        int t3 = instrumentr_promise_get_force_entry_time(promise);

        /* if environment is born inside this promise's evaluation, then
           we stop the analysis here because the effect is contained inside
           this promise. */
        if (env_birth_time > t3) {
            return;
        }

        // This means the environment has been modified after the promise is
        // created but before it is forced and while forcing, this promise
        // is reading from the environment. Since this read can potentially
        // be from the modified location, we should mark it as non local and
        // make the argument lazy.
        if (!transitive && (t2 > t1 && t2 < t3)) {
            const std::vector<Argument*>& args =
                argument_table.lookup(promise_id);

            for (auto& arg: args) {
                arg->side_effect(type, transitive);
            }

            Argument* arg = args.back();

            /* NOTE: source_ids are NA because effect originates from this
             * promise. */

            effects_table.insert(type,
                                 varname,
                                 transitive,
                                 env_id,
                                 NA_INTEGER,
                                 NA_INTEGER,
                                 NA_INTEGER,
                                 NA_INTEGER,
                                 arg->get_fun_id(),
                                 arg->get_call_id(),
                                 promise_id,
                                 arg->get_formal_pos(),
                                 backtrace.to_string());

            source_fun_id = arg->get_fun_id();
            source_call_id = arg->get_call_id();
            source_arg_id = promise_id;
            source_formal_pos = arg->get_formal_pos();
            transitive = true;
        }

        /* if transitive is set, then the promise directly responsible for
         * the effect has already been found and handled. All other promises
         * on the stack are transitively responsible for forcing it and have
         * to be made lazy. */
        if (transitive) {
            const std::vector<Argument*>& args =
                argument_table.lookup(promise_id);

            for (auto& arg: args) {
                arg->side_effect(type, transitive);
            }

            // Take the most recent argument
            Argument* arg = args.back();

            effects_table.insert(type,
                                 varname,
                                 transitive,
                                 env_id,
                                 source_fun_id,
                                 source_call_id,
                                 source_arg_id,
                                 source_formal_pos,
                                 arg->get_fun_id(),
                                 arg->get_call_id(),
                                 promise_id,
                                 arg->get_formal_pos(),
                                 ENVTRACER_NA_STRING);

            /* loop back to next promise on the stack so it can be made
             * responsible for transitive read. */
            continue;
        }
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
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_char_t charval = instrumentr_symbol_get_element(symbol);
    std::string varname = instrumentr_char_get_element(charval);

    process_reads(state,
                  environment,
                  'L',
                  varname,
                  arg_table,
                  env_table,
                  effects_table,
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

void process_writes(instrumentr_state_t state,
                    instrumentr_environment_t environment,
                    const char type,
                    const std::string& varname,
                    ArgumentTable& argument_table,
                    EnvironmentTable& environment_table,
                    EffectsTable& effects_table,
                    Backtrace& backtrace) {
    /* don't process *tmp* as it is an implementation variable */
    if (varname == "*tmp*") {
        return;
    }

    Environment* env = environment_table.insert(environment);

    int t2 = instrumentr_environment_get_birth_time(environment);
    int env_id = instrumentr_environment_get_id(environment);

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
        int t1 = instrumentr_promise_get_force_entry_time(promise);

        /* if environment is born inside the the currently forced promise,
         * then effect is contained inside the promise and we can exit */
        if (t2 > t1) {
            return;
        }

        // if promise is forced after the environment it is writing to is
        // born then the write is a non-local side effect
        if (!transitive) {
            const std::vector<Argument*>& args =
                argument_table.lookup(promise_id);

            for (auto& arg: args) {
                arg->side_effect(type, transitive);
            }

            Argument* arg = args.back();

            /* NOTE: source_ids are NA because effect originates from this
             * promise. */

            effects_table.insert(type,
                                 varname,
                                 transitive,
                                 env_id,
                                 NA_INTEGER,
                                 NA_INTEGER,
                                 NA_INTEGER,
                                 NA_INTEGER,
                                 arg->get_fun_id(),
                                 arg->get_call_id(),
                                 promise_id,
                                 arg->get_formal_pos(),
                                 backtrace.to_string());

            source_fun_id = arg->get_fun_id();
            source_call_id = arg->get_call_id();
            source_arg_id = promise_id;
            source_formal_pos = arg->get_formal_pos();
            transitive = true;
        }

        /* if transitive is set, then the promise directly responsible for
         * the effect has already been found and handled. All other promises
         * on the stack satisfying this condition are transitively
         * responsible for forcing it and have to be made lazy. */
        if (transitive) {
            const std::vector<Argument*>& args =
                argument_table.lookup(promise_id);

            for (auto& arg: args) {
                arg->side_effect(type, transitive);
            }

            // Take the most recent argument
            Argument* arg = args.back();

            effects_table.insert(type,
                                 varname,
                                 transitive,
                                 env_id,
                                 source_fun_id,
                                 source_call_id,
                                 source_arg_id,
                                 source_formal_pos,
                                 arg->get_fun_id(),
                                 arg->get_call_id(),
                                 promise_id,
                                 arg->get_formal_pos(),
                                 ENVTRACER_NA_STRING);

            /* loop back to next promise on the stack so it can be made
             * responsible for transitive write. */
            continue;
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
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_char_t charval = instrumentr_symbol_get_element(symbol);
    std::string varname = instrumentr_char_get_element(charval);

    process_writes(state,
                   environment,
                   'A',
                   varname,
                   arg_table,
                   env_table,
                   effects_table,
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
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_char_t charval = instrumentr_symbol_get_element(symbol);
    std::string varname = instrumentr_char_get_element(charval);

    process_writes(state,
                   environment,
                   'D',
                   varname,
                   arg_table,
                   env_table,
                   effects_table,
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
    Backtrace& backtrace = tracing_state.get_backtrace();

    instrumentr_char_t charval = instrumentr_symbol_get_element(symbol);
    std::string varname = instrumentr_char_get_element(charval);

    process_writes(state,
                   environment,
                   'R',
                   varname,
                   arg_table,
                   env_table,
                   effects_table,
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

    instrumentr_character_t classes = instrumentr_value_as_character(value);

    for (int i = 0; i < instrumentr_character_get_size(classes); ++i) {
        instrumentr_char_t char_val =
            instrumentr_character_get_element(classes, i);
        if (!instrumentr_char_is_na(char_val)) {
            env->update_class(instrumentr_char_get_element(char_val));
        }
    }
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
